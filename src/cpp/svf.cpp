//
// Created by Paul Ross on 2020-01-14.
//
#include <iostream>
#include <iterator>
#include <sstream>

#include "svf.h"

namespace SparseVirtualFileSystem {

    bool SparseVirtualFile::has(t_fpos fpos, size_t len) const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif

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
            std::ostringstream os;
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
            std::ostringstream os;
            os << "SparseVirtualFile::write():";
            os << " Difference at position " << fpos;
            os << " '" << *(data) << "' != '" << iter->second[index_iter] << "'";
            os << " Ordinal " << static_cast<int>(*data) << " != " << static_cast<int>(iter->second[index_iter]);
            throw ExceptionSparseVirtualFileDiff(os.str());
        }
    }

    void SparseVirtualFile::write(t_fpos fpos, const char *data, size_t len) {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        // TODO: throw if !data, len == 0
        // Comments are structures like this where ==== are existing blocks and ++++ is the new block.
        // ^==== shows where the iterator is pointing to. fpos is the beginning of the ++++ bl.ock.
        //
        //       ^===========|    |=====|
        //  |+++++++++++++++++++++++++++++++++|
        //
//        std::cout << "TRACE: write(): fpos = " << fpos << " len = " << len << std::endl;

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
                if (fpos > iter->first + iter->second.size()) {
                    _write(fpos, data, len);
                    m_bytes_total += len;
                } else {
                    // Append new to existing block, possibly coalescing existing blocks.
                    _write_append_new_to_old(fpos, data, len, iter);
                }
            }
        }
        m_count_write += 1;
        m_bytes_write += len;
        m_time_write = std::chrono::system_clock::now();
        SVF_ASSERT(integrity() == ERROR_NONE);
//        std::cout << "size now " << m_svf.size() << std::endl;
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
                // Copy new data 'X' up to start of iter.
                //       ^===========|  |=====|
                //  |XXXX+++++++++++++XX++|
                new_vector.push_back(*data);
                ++data;
                ++fpos;
                --len;
                ++m_bytes_total;
            }
            size_t index_iter = 0;
            while (len && _last_file_pos_for_block(iter) > fpos) {
                // Check overlapped data matches 'Y'
                //       ^===========|  |=====|
                //  |++++YYYYYYYYYYYYY++++|
                _throw_diff(fpos, data, iter, index_iter);
                new_vector.push_back(*data);
                ++data;
                ++fpos;
                --len;
                ++index_iter;
            }
            if (_last_file_pos_for_block(iter) > fpos_end) {
                // Copy rest of iter 'Z'
                assert(len == 0);
                //       ^=========ZZZ
                //  |+++++++++++++|
                // So append up to the end of iter and (maybe) go round again.
                while (index_iter < iter->second.size()) {
                    new_vector.push_back(iter->second[index_iter]);
                    ++index_iter;
                }
                m_svf.erase(iter);
                break;
            }
            // Remove copied and checked old block and move on.
            if (m_overwrite) {
                iter->second.assign(iter->second.size(), '0');
            }
            iter = m_svf.erase(iter);
            if (iter == m_svf.end() || iter->first > fpos + len) {
                // Copy rest of new and break
                while (len) {
                    new_vector.push_back(*data);
                    ++data;
                    ++fpos;
                    --len;
                    ++m_bytes_total;
                }
                break;
            }
        }
        auto size_before_insert = m_svf.size();
        m_svf.insert({fpos_start, std::move(new_vector)});
        if (m_svf.size() != 1 + size_before_insert) {
            std::ostringstream os;
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
        assert(fpos >= iter->first);
        assert(fpos <= _last_file_pos_for_block(iter));

        size_t fpos_end = fpos + len;

        t_map::iterator next_iter = iter;
        while (true) {
            if (len == 0) {
                // Break condition: Coalesce next_iter if necessary
                if (fpos == next_iter->first) {
                    for (size_t i = 0; i < next_iter->second.size(); ++i) {
                        iter->second.push_back(next_iter->second[i]);
                    }
                    m_svf.erase(next_iter);
                }
                break;
            }
            size_t index_iter = fpos - next_iter->first;
            while (len && index_iter < next_iter->second.size()) {
                // Check overlapped data matches, copying to iter
                _throw_diff(fpos, data, next_iter, index_iter);
                ++index_iter;
                ++data;
                ++fpos;
                --len;
            }
            if (next_iter != iter) {
                // Coalesce by copying all of next_iter to iter.
                for (size_t i = 0; i < next_iter->second.size(); ++i) {
                    iter->second.push_back(next_iter->second[i]);
                }
                // Erase next_iter as we have copied it all.
                if (m_overwrite) {
                    next_iter->second.assign(next_iter->second.size(), '0');
                }
                next_iter = m_svf.erase(next_iter);
            } else {
                ++next_iter;
            }
            // Write out the new data between the blocks
            while (len && (next_iter == m_svf.end() || fpos < next_iter->first)) {
                iter->second.push_back(*data);
                ++data;
                ++fpos;
                --len;
                ++m_bytes_total;
            }
        }
        assert(len == 0);
        assert(fpos == fpos_end);
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

// Read data and write to the buffer provided by the caller.
// It is up to the caller to make sure that p can contain len chars.
    void SparseVirtualFile::read(t_fpos fpos, size_t len, char *p) {
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        SVF_ASSERT(integrity() == ERROR_NONE);

        if (m_svf.empty()) {
            throw ExceptionSparseVirtualFileRead("SparseVirtualFile::read(): Sparse virtual file is empty.");
        }
        t_map::const_iterator iter = m_svf.lower_bound(fpos);
        if (iter == m_svf.begin() && iter->first != fpos) {
            std::ostringstream os;
            os << "SparseVirtualFile::read():";
            os << " Requested file position " << fpos << " precedes first block at " << iter->first;
            throw ExceptionSparseVirtualFileRead(os.str());
        }
        size_t offset = 0;
        if (iter == m_svf.end() || iter->first != fpos) {
            --iter;
            offset = fpos - iter->first;
        }
        if (offset + len > iter->second.size()) {
            std::ostringstream os;
            os << "SparseVirtualFile::read():";
            os << " Requested position " << fpos << " and length " << len;
            os << " overruns block at " << iter->first << " of size " << iter->second.size();
            throw ExceptionSparseVirtualFileRead(os.str());
        }
        while (len) {
            *p = iter->second[offset];
            ++offset;
            ++p;
            --len;
        }
        m_bytes_read += len;
        m_count_read += 1;
        m_time_read = std::chrono::system_clock::now();
    }

    t_seek_read SparseVirtualFile::need(t_fpos fpos, size_t len) const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif

        if (m_svf.empty()) {
            return {{fpos, len}};
        }
        t_fpos fpos_to = fpos + len;
        t_seek_read ret;
        t_map::const_iterator iter = m_svf.upper_bound(fpos);
        if (iter == m_svf.begin()) {
            if (fpos + len <= iter->first) {
                ret.push_back({fpos, len});
                fpos += len;
                len = 0;
            } else {
                ret.push_back({fpos, iter->first - fpos});
                len -= iter->first - fpos;
                fpos = iter->first;
            }
        } else {
            auto last_fpos = _last_file_pos_for_block(std::prev(iter));
            if (fpos < last_fpos) {
                // Example, change:
                //        |==|    ^==|
                //         |++++++++++++|
                // to:
                //            |+++++++++|
                len -= std::min(len, last_fpos - fpos);
                fpos = std::min(fpos_to, last_fpos);
            }
        }
        while (len) {
            if (iter == m_svf.end() || fpos + len <= iter->first) {
                //   ^==|
                //         |++++++++++++|
                assert(len);
                ret.push_back({fpos, len});
                fpos += len;
                len = 0;
                break;
            }
            if (fpos < iter->first) {
                if (len < iter->first - fpos) {
                    //          ^==|
                    //   |++++|
                    ret.push_back({fpos, len});
                    fpos += len;
                    len = 0;
                    break;
                } else {
                    auto bytes_added = iter->first - fpos;
                    ret.push_back({fpos, bytes_added});
                    len -= bytes_added;
                    fpos += bytes_added;

                    auto last_fpos = _last_file_pos_for_block(std::prev(iter));
                    if (len <= last_fpos - fpos) {
                        //          ^==|
                        //   |++++++++|
                        fpos += len;
                        len = 0;
                        break;
                    } else {
                        //          ^==|
                        //   |++++++++++++|
                        len -= last_fpos - fpos;
                        fpos = last_fpos;
                        // Go round again.
                    }
                }
            } else if (fpos + len <= _last_file_pos_for_block(iter)) {
                //          ^======|
                //          |+++++|
                fpos += len;
                len = 0;
                break;
            } else {
                assert(fpos == iter->first);
                assert(len > iter->second.size());
                fpos += iter->second.size();
                len -= iter->second.size();
            }
            ++iter;
        }
        assert(fpos == fpos_to);
        assert(len == 0);
        return ret;
    }

    t_seek_read SparseVirtualFile::blocks() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif

        t_seek_read ret;
        for (t_map::const_iterator iter = m_svf.cbegin(); iter != m_svf.cend(); ++iter) {
            ret.push_back({ iter->first, iter->second.size() });
        }
        return ret;
    }

    size_t SparseVirtualFile::size_of() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        size_t ret = sizeof(SparseVirtualFile);

        // Add heap referenced data sizes.
        ret += m_id.size();
        for (auto &iter: m_svf) {
            ret += sizeof(iter.first);
            ret += sizeof(iter.second);
            ret += iter.second.size();
        }
        return ret;
    }

// m_coalesce, m_file_mod_time are unchanged.
    void SparseVirtualFile::clear() noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        m_id = "";
        m_bytes_total = 0;
        m_count_write = 0;
        m_count_read = 0;
        m_bytes_write = 0;
        m_bytes_read = 0;
        m_time_write = std::chrono::time_point<std::chrono::system_clock>::min();
        m_time_read = std::chrono::time_point<std::chrono::system_clock>::min();
        if (m_overwrite) {
            for (auto &iter: m_svf) {
                iter.second.assign(iter.second.size(), '0');
            }
        }
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

/// Returns the largest possible file position known so far.
/// Of course this is not the EOF position as we may not have been offered that yet.
    t_fpos
    SparseVirtualFile::last_file_position() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
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

        auto ret = iter->first + iter->second.size();
        return ret;
    }

} // namespace SparseVirtualFileSystem
