//
// Created by Paul Ross on 2020-01-14.
//
#include <iterator>
#include <strstream>

#include "svf.h"

namespace SparseVirtualFileSystem {

    bool SparseVirtualFile::has(t_fpos fpos, size_t len) const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
        if (m_svf.empty()) {
            return false;
        }
        t_map::const_iterator iter = m_svf.upper_bound(fpos);
        if (iter != m_svf.begin()) {
            --iter;
        }
        t_fpos fpos_end = _last_file_pos_for_block(iter);
        if (fpos >= iter->first && (fpos + len) <= fpos_end) {
            return true;
        }
        return false;
    }

    void SparseVirtualFile::_write(t_fpos fpos, const char *data, size_t len) {
        t_val new_vector;
        new_vector.reserve(len);
        while (len) {
            new_vector.push_back(*data);
            --len;
            ++data;
        }
        auto size_before_insert = m_svf.size();
        m_svf.insert(m_svf.end(), {fpos, std::move(new_vector)});
        if (m_svf.size() != 1 + size_before_insert) {
            std::ostrstream os;
            os << "SparseVirtualFile::write():";
            os << "Unable to insert new block at " << fpos;
            throw ExceptionSparseVirtualFileWrite(os.str());
        }
    }

    void
    SparseVirtualFile::_throw_diff(t_fpos fpos, const char *data, t_map::const_iterator iter, size_t index_iter) const {
        assert(data);
        assert(iter != m_svf.end());
        if (*data != iter->second[index_iter]) {
            std::ostrstream os;
            os << "SparseVirtualFile::write():";
            os << " Difference at position " << fpos;
            os << " " << *(data) << " != " << iter->second[index_iter];
            throw ExceptionSparseVirtualFileDiff(os.str());
        }
    }

    void SparseVirtualFile::write(t_fpos fpos, const char *data, size_t len) {
        SVF_ASSERT(integrity() == ERROR_NONE);
        // TODO: throw if !data, len == 0
        // Comments are structures like this where ==== are existing blocks and ++++ is the new block.
        // ^==== shows where the iterator is pointing to. fpos is the beginning of the ++++ bl.ock.
        //
        //       ^===========|    |=====|
        //  |+++++++++++++++++++++++++++++++++|
        //
        if (m_svf.empty()) {
            // Simple insert of new data into empty map.
            _write(fpos, data, len);
            m_bytes_total += len;
        } else {
            t_map::iterator iter = m_svf.upper_bound(fpos);
            if (iter != m_svf.begin()) {
                --iter;
            }
            if (iter->first > fpos) {
                // New comes earlier, copy existing block on to it.
                // Insert new block, possibly coalescing existing blocks.
                _write_new_append_old(fpos, data, len, iter);
            } else {
                // Append new to existing block, possibly coalescing existing blocks.
                _write_append_new_to_old(fpos, data, len, iter);
            }
        }
        m_count_write += 1;
        m_bytes_write += len;
        m_time_write = std::chrono::time_point<std::chrono::system_clock>::clock().now();
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    void SparseVirtualFile::_write_new_append_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter) {
        // We are in either of these situations.
        //       ^===========|  |=====|
        //  |+++++++++++++++++++++|
        // or:
        //       ^===========|
        //  |+++++++++++++++++++++|
        // or:
        //       ^===========|
        //  |+++++++++++++|
        SVF_ASSERT(integrity() == ERROR_NONE);
        assert(data);
        assert(len > 0);
        assert(iter != m_svf.end());
        assert(iter->first > fpos);

        size_t fpos_start = fpos;
        size_t fpos_end = fpos + len;
        t_val new_vector;
        while (true) {
            while (len && fpos < iter->first) {
                // Copy new data
                new_vector.push_back(*data);
                ++data;
                ++fpos;
                --len;
                ++m_bytes_total;
            }
            size_t index_iter = 0;
            while (len && _last_file_pos_for_block(iter) > fpos) {
                // Check overlapped data matches
                _throw_diff(fpos, data, iter, index_iter);
                ++index_iter;
                ++data;
                ++fpos;
                --len;
            }
            // Copy rest of iter.
            if (_last_file_pos_for_block(iter) > fpos_end) {
                assert(len == 0);
                //       ^===========|
                //  |+++++++++++++|
                // So append up to the end of iter and (maybe) go round again.
                while (index_iter < iter->second.size()) {
                    new_vector.push_back(iter->second[index_iter]);
                    ++index_iter;
                }
                break;
            }
            // Remove copied and checked old block and move on.
            iter = m_svf.erase(iter);
            if (len != 0 || iter == m_svf.end()) {
                // Copy rest of new and break
                while (len) {
                    new_vector.push_back(*data);
                    ++data;
                    ++fpos; // Could just set to fpos_end
                    --len;
                    ++m_bytes_total;
                }
                break;
            }
        }
        auto size_before_insert = m_svf.size();
        m_svf.insert(iter, {fpos_start, std::move(new_vector)});
        if (m_svf.size() != 1 + size_before_insert) {
            std::ostrstream os;
            os << "SparseVirtualFile::write():";
            os << "Unable to insert new block at " << fpos_start;
            throw ExceptionSparseVirtualFileWrite(os.str());
        }
        assert(len == 0);
        assert(fpos == fpos_end);
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    void SparseVirtualFile::_write_append_new_to_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter) {
        // We are in these situations:
        //  ^===========|    |=====|
        //  |+++++++++|
        //
        //  ^===========|    |=====|
        //     |++++++|
        //
        //  ^===========|    |=====|
        //     |+++++++++++|
        //
        //  ^===========|    |=====|
        //     |++++++++++++++++++|
        //
        SVF_ASSERT(integrity() == ERROR_NONE);
        assert(data);
        assert(len > 0);
        assert(iter != m_svf.end());
        assert(iter->first <= fpos);
        assert(_last_file_pos_for_block(iter) > fpos);

        size_t fpos_end = fpos + len;
        t_map::const_iterator next_iter = iter;
        while (true) {
            size_t index_iter = fpos - next_iter->first;
            while (len && index_iter < next_iter->second.size()) {
                // Check overlapped data matches
                _throw_diff(fpos, data, next_iter, index_iter);
                ++index_iter;
                ++data;
                ++fpos;
                --len;
            }
            // Write out the new data between the blocks
            while (len && (next_iter == m_svf.end() || fpos < next_iter->first)) {
                iter->second.push_back(*data);
                ++data;
                ++fpos;
                --len;
                ++m_bytes_total;
            }
            if (next_iter != iter) {
                next_iter = m_svf.erase(next_iter);
            } else {
                ++next_iter;
            }
            if (next_iter == m_svf.end()) {
                // Write out remainder from new and finish.
                while (len) {
                    iter->second.push_back(*data);
                    ++data;
                    ++fpos;
                    --len;
                    ++m_bytes_total;
                }
                break;
            }
        }
        assert(len == 0);
        assert(fpos == fpos_end);
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

// Read data and write to the buffer provided by the caller.
// It is up to the caller to make sure that p can contain len chars.
    void SparseVirtualFile::read(t_fpos fpos, size_t len, char *p) {
        SVF_ASSERT(integrity() == ERROR_NONE);
        t_map::const_iterator iter = m_svf.lower_bound(fpos);
        t_fpos offset = 0;

        if (iter == m_svf.end()) {
            std::ostrstream os;
            os << "SparseVirtualFile::read():";
            os << " Can not read length " << len << " at position " << fpos;
            os << " as at end()";
            throw ExceptionSparseVirtualFileRead(os.str());
        }
        if (iter->first == fpos) {
            if (len > iter->second.size()) {
                std::ostrstream os;
                os << "SparseVirtualFile::read():";
                os << " Can not read length " << len << " at position " << fpos;
                os << " as only have data length " << iter->second.size();
                throw ExceptionSparseVirtualFileRead(os.str());
            }
            // offset = 0
        } else {
            if (iter == m_svf.begin()) {
                std::ostrstream os;
                os << "SparseVirtualFile::read():";
                os << " Can not read length " << len << " at position " << fpos;
                os << " as at begin()";
                throw ExceptionSparseVirtualFileRead(os.str());
            }
            --iter;
            if (_last_file_pos_for_block(iter) > fpos + len) {
                std::ostrstream os;
                os << "SparseVirtualFile::read():";
                os << " Can not read length " << len << " at position " << fpos;
                os << " as it is past position " << iter->first;
                os << " that has length " << iter->second.size();
                throw ExceptionSparseVirtualFileRead(os.str());
            }
            offset = fpos - iter->first;
        }
        for (size_t i = offset; i < len + offset; ++i) {
            *p = iter->second[i];
            ++p;
        }
        m_bytes_read += len;
        m_count_read += 1;
        m_time_read = std::chrono::time_point<std::chrono::system_clock>::clock().now();
    }

    t_seek_read SparseVirtualFile::need(t_fpos fpos, size_t len) const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
        t_seek_read ret;
        t_map::const_iterator iter = m_svf.lower_bound(fpos);

        // TODO

        return ret;
    }

    t_seek_read SparseVirtualFile::blocks() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
        t_seek_read ret;
        for (const auto &iter: m_svf) {
            ret.push_back({ iter.first, iter.second.size() });
        }
        return ret;
    }

    size_t SparseVirtualFile::size_of() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
        size_t ret = sizeof(SparseVirtualFile);

        ret += m_id.size();
        for (auto &iter: m_svf) {
            ret += iter.second.size();
        }
        return ret;
    }

// m_coalesce, m_file_mod_time are unchanged.
    void SparseVirtualFile::clear() noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
        m_id = "";
        m_file_mod_time = 0.0;
        m_bytes_total = 0;
        m_count_write = 0;
        m_count_read = 0;
        m_bytes_write = 0;
        m_bytes_read = 0;
        m_time_write = std::chrono::time_point<std::chrono::system_clock>::min();
        m_time_read = std::chrono::time_point<std::chrono::system_clock>::min();
        m_svf.clear();
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    SparseVirtualFile::ERROR_CONDITION
    SparseVirtualFile::integrity() const noexcept {
        t_fpos prev_fpos;
        t_map::const_iterator iter = m_svf.begin();
        size_t byte_count = 0;

        while (iter != m_svf.end()) {
            if (iter->second.empty()) {
                return ERROR_EMPTY_BLOCK;
            }
            if (iter != m_svf.begin() && prev_fpos == iter->first) {
                return ERROR_ADJACENT_BLOCKS;
            }
            if (iter != m_svf.begin() && prev_fpos >= iter->first) {
                return ERROR_BLOCKS_OVERLAP;
            }
            prev_fpos = _last_file_pos_for_block(iter);
            byte_count += iter->second.size();
            ++iter;
        }
        if (byte_count != m_bytes_total) {
            return ERROR_BYTE_COUNT_MISMATCH;
        }
        return ERROR_NONE;
    }

/// Returns the largest possible file postion known so far.
/// Of course this is not the EOF position as we maay not have been offered that yet.
    t_fpos
    SparseVirtualFile::last_file_position() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
        if (m_svf.empty()) {
            return 0;
        } else {
            auto iter = m_svf.end();
            --iter;
            return _last_file_pos_for_block(iter);
        }
    }

    t_fpos
    SparseVirtualFile::_last_file_pos_for_block(t_map::const_iterator iter) const noexcept {
        // NOTE: do not SVF_ASSERT(integrity() == ERROR_NONE); as integrity() calls this so infinite recursion.
        assert(iter != m_svf.end());

        return iter->first + iter->second.size();
    }

} // namespace SparseVirtualFileSystem
