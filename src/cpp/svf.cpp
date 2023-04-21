//
// Created by Paul Ross on 2020-01-14.
//
#include <iostream>
#include <iterator>
#include <sstream>

#include "svf.h"

namespace SVFS {
    // Used to overwrite the memory before discarding it.
    static const char OVERWRITE_CHAR = '0';

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
            os << " Unable to insert new block at " << fpos;
            throw ExceptionSparseVirtualFileWrite(os.str());
        }
    }

    void
    SparseVirtualFile::_throw_diff(t_fpos fpos, const char *data, t_map::const_iterator iter, size_t index_iter) const {
        assert(data);
        assert(iter != m_svf.end());
        assert(m_config.compare_for_diff);

        if (*data != iter->second[index_iter]) {
            std::ostringstream os;
            os << "SparseVirtualFile::write():";
            os << " Difference at position " << fpos;
            os << " '" << *(data) << "' != '" << iter->second[index_iter] << "'";
            os << " Ordinal " << static_cast<int>(*data) << " != " << static_cast<int>(iter->second[index_iter]);
            std::string str = os.str();
            throw ExceptionSparseVirtualFileDiff(str);
        }
        // Assert as this should now never be called is there is _not_ a diff.
        assert(0);
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
        if (m_svf.empty() || fpos > _last_file_position()) {
            // Simple insert of new data into empty map or a node beyond the end (common case).
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
                if (fpos > _last_file_pos_for_block(iter)) {
                    // No overlap so just write new block
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
            size_t delta = std::min(len, iter->second.size());
            // Check overlapped data matches 'Y'
            //       ^===========|  |=====|
            //  |++++YYYYYYYYYYYYY++++|
            if (m_config.compare_for_diff) {
                if (std::memcmp(iter->second.data(), data, delta) != 0) {
                    _throw_diff(fpos, data, iter, 0);
                }
            }
            for (size_t i = 0; i < delta; ++i) {
                new_vector.push_back(*data);
                ++data;
            }
            fpos += delta;
            len -= delta;
            index_iter += delta;
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
                if (m_config.overwrite_on_exit) {
                    iter->second.assign(iter->second.size(), OVERWRITE_CHAR);
                }
                m_svf.erase(iter);
                break;
            }
            // Remove copied and checked old block and move on.
            if (m_config.overwrite_on_exit) {
                iter->second.assign(iter->second.size(), OVERWRITE_CHAR);
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
            os << " Unable to insert new block at " << fpos_start;
            throw ExceptionSparseVirtualFileWrite(os.str());
        }
        assert(len == 0);
        assert(fpos == fpos_end);
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    /**
     * From file position, write the new_data to the block identified by base_block_iter.
     * This may involve coalescing existing blocks that follow base_block_iter.
     *
     * We are in these kind of situations. 1: means original, 2: is new_data to abe added, 3: is the result.
     * 'c' means checked equal (if required), 'A' means appended::
     *
     *      1: ^===========|    |=====|
     *      2: |+++++++++|
     *      3: ^cccccccccAA|    |=====|
     *
     *      1: ^===========|    |=====|
     *      2:    |++++++|
     *      3: ^===cccccc==|    |=====|
     *
     *      1: ^===========|    |=====|
     *      2:    |+++++++++++|
     *      3: ^===========AAA| |=====|
     *
     *      1: ^===========|    |=====|
     *      2:    |++++++++++++++++++|
     *      3: ^===ccccccccAAAAAAcccc=|
     *
     *      1: ^===========|    |=====|    |=====|
     *      2:    |++++++++++++++++++++++++++|
     *      3: ^===ccccccccAAAAAAcccccAAAAAAc====|
     *
     * @param fpos File position of the start of the new new_data.
     * @param new_data The new_data
     * @param new_data_len The length of the new data.
     * @param base_block_iter Block to write to.
     */
    void SparseVirtualFile::_write_append_new_to_old(t_fpos fpos, const char *new_data, size_t new_data_len,
                                                     t_map::iterator base_block_iter) {
        SVF_ASSERT(integrity() == ERROR_NONE);
        assert(new_data);
        assert(new_data_len > 0);
        assert(base_block_iter != m_svf.end());
        assert(fpos >= base_block_iter->first);
        assert(fpos <= _last_file_pos_for_block(base_block_iter));

#ifdef DEBUG
        size_t fpos_end = fpos + new_data_len;
#endif
        // Diff check against base_block_iter
        // Do the check to end of new_data_len or end of base_block_iter which ever comes first.
        // Do not increment m_bytes_total as this is existing new_data.
        // Diff check against base_block_iter
        size_t write_index_from_block_start = fpos - base_block_iter->first;
        // Do the check to end of new_data_len or end of base_block_iter which ever comes first.
        size_t len_check_or_copy = std::min(new_data_len, base_block_iter->second.size() - write_index_from_block_start);
        if (m_config.compare_for_diff) {
            if (std::memcmp(base_block_iter->second.data() + write_index_from_block_start, new_data, len_check_or_copy) != 0) {
                _throw_diff(fpos, new_data, base_block_iter, write_index_from_block_start);
            }
        }
        new_data += len_check_or_copy;
        fpos += len_check_or_copy;
        new_data_len -= len_check_or_copy;
        t_map::iterator next_block_iter = std::next(base_block_iter);
        while (new_data_len) {
            if (next_block_iter == m_svf.end()) {
                // Termination case, copy remainder
                while (new_data_len) {
                    base_block_iter->second.push_back(*new_data);
                    ++new_data;
                    ++fpos;
                    --new_data_len;
                    m_bytes_total += 1;
                }
                break; // Done. Needed because we are going to do erase(next_block_iter) otherwise.
            } else {
                // Copy the new_data up to start of next_block_iter or, we have exhausted the new_data.
                while (new_data_len && fpos < next_block_iter->first) {
                    base_block_iter->second.push_back(*new_data);
                    ++new_data;
                    ++fpos;
                    --new_data_len;
                    m_bytes_total += 1;
                }
            }
            if (new_data_len == 0 and fpos < next_block_iter->first) {
                // We have exhausted new data and not reached the next block so we are done
                break;
            }
            // Still data to copy, and we are up against the next block which we will coalesce into the base_block_iter.
            write_index_from_block_start = 0;
            // Diff check, but also append to base_block_iter.
            // Do not increment m_bytes_total as this is existing data.
            len_check_or_copy = std::min(new_data_len, _last_file_pos_for_block(next_block_iter) - fpos);
            if (len_check_or_copy) {
                if (m_config.compare_for_diff) {
                    if (std::memcmp(next_block_iter->second.data(), new_data, len_check_or_copy) != 0) {
                        _throw_diff(fpos, new_data, base_block_iter, 0);
                    }
                }
                // We could push_back either the new_data or the existing next block data. We choose the former.
                for (size_t i = 0; i < len_check_or_copy; ++i) {
                    base_block_iter->second.push_back(*new_data);
                    ++new_data;
                    ++fpos;
                    --new_data_len;
                    ++write_index_from_block_start;
                }
            }
            // Here either new_data is exhausted or next_block_iter is.
            // If new_data is exhausted then copy remaining from next_block_iter to base_block_iter.
            // Do not increment m_bytes_total as this is existing new_data.
            if (new_data_len == 0) {
                while (write_index_from_block_start < next_block_iter->second.size()) {
                    base_block_iter->second.push_back(next_block_iter->second[write_index_from_block_start]);
                    ++write_index_from_block_start;
                }
            }
            // New data is not exhausted so erase next_block_iter as we have copied it and move on to the next block.
            if (m_config.overwrite_on_exit) {
                next_block_iter->second.assign(next_block_iter->second.size(), OVERWRITE_CHAR);
            }
            next_block_iter = m_svf.erase(next_block_iter);
        }
        assert(new_data_len == 0);
#ifdef DEBUG
        assert(fpos == fpos_end);
#endif
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

// Read data and write to the buffer provided by the caller.
// It is up to the caller to make sure that p can contain len chars.
    void SparseVirtualFile::_read(t_fpos fpos, size_t len, char *p) const {
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
        /* TODO: memcpy()? */
        while (len) {
            *p = iter->second[offset];
            ++offset;
            ++p;
            --len;
        }
    }

    void SparseVirtualFile::read(t_fpos fpos, size_t len, char *p) {
        _read(fpos, len, p);
        // Adjust non-const members
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
                //        ^==|
                //   |+++|
                ret.push_back({fpos, len});
                fpos += len;
                len = 0;
                // We are done.
            }
        } else {
            // Otherwise check the previous node with std::prev.
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
        // Now walk through the remaining nodes.
        while (len) {
            if (iter == m_svf.end() || fpos + len <= iter->first) {
                // Either:
                //   |==|
                //         |++++++++++++|
                // or:
                //              |==|
                //   |+++++|
                ret.push_back({fpos, len});
                fpos += len;
                len = 0;
                break;
            }
            // We are in this state:
            //          ^=====|
            //     |++++++++++++++|
            // or:
            //          ^=====|
            //     |++++++++|
            if (fpos < iter->first) {
                assert(len >= iter->first - fpos);
                auto bytes_added = iter->first - fpos;
                ret.push_back({fpos, bytes_added});
                len -= bytes_added;
                fpos += bytes_added;
            }
            // We are now in this state:
            //          ^=====|
            //          |+++++++|
            // or:
            //          ^=====|
            //          |++++|
            assert(fpos == iter->first);
            if (fpos + len <= _last_file_pos_for_block(iter)) {
                //          ^======|
                //          |++++|
                fpos += len;
                len = 0;
                break;
            } else {
                //          ^======|
                //          |+++++++++|
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

    size_t SparseVirtualFile::block_size(t_fpos fpos) const {
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        SVF_ASSERT(integrity() == ERROR_NONE);

        if (m_svf.empty()) {
            throw ExceptionSparseVirtualFileRead("SparseVirtualFile::read(): Sparse virtual file is empty.");
        }
        t_map::const_iterator iter = m_svf.find(fpos);
        if (iter == m_svf.end()) {
            std::ostringstream os;
            os << "SparseVirtualFile::block_size():";
            os << " Requested file position " << fpos << " is not a block";
            throw ExceptionSparseVirtualFileRead(os.str());
        }
        return iter->second.size();
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
        // Maintain ID and constructor arguments.
//        m_time_write = std::chrono::time_point<std::chrono::system_clock>::min();
//        m_time_read = std::chrono::time_point<std::chrono::system_clock>::min();
        if (m_config.overwrite_on_exit) {
            for (auto &iter: m_svf) {
                iter.second.assign(iter.second.size(), OVERWRITE_CHAR);
            }
        }
        m_svf.clear();
        m_bytes_total = 0;
        m_count_write = 0;
        m_count_read = 0;
        m_bytes_write = 0;
        m_bytes_read = 0;
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    size_t SparseVirtualFile::erase(t_fpos fpos) {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        auto iter = m_svf.find(fpos);
        if (iter == m_svf.end()) {
            std::ostringstream os;
            os << "SparseVirtualFile::erase():";
            os << " Non-existent file position " << fpos << ".";
            throw ExceptionSparseVirtualFileErase(os.str());
        }
        size_t ret = iter->second.size();
        m_bytes_total -= ret;
        m_svf.erase(iter);
        return ret;
    }

    SparseVirtualFile::ERROR_CONDITION
    SparseVirtualFile::integrity() const noexcept {
        t_fpos prev_fpos;
        size_t prev_size;
        t_map::const_iterator iter = m_svf.begin();
        size_t byte_count = 0;

        while (iter != m_svf.end()) {
            t_fpos fpos = iter->first;
            size_t size = iter->second.size();
            if (iter->second.empty()) {
                return ERROR_EMPTY_BLOCK;
            }
            if (iter != m_svf.begin() && fpos == prev_fpos && size == prev_size) {
                return ERROR_DUPLICATE_BLOCK;
            }
            if (iter != m_svf.begin() && prev_fpos + prev_size == iter->first) {
                return ERROR_ADJACENT_BLOCKS;
            }
            if (iter != m_svf.begin() && prev_fpos + prev_size > iter->first) {
                return ERROR_BLOCKS_OVERLAP;
            }
            prev_fpos = iter->first;
            prev_size = iter->second.size();
            byte_count += prev_size;
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

    /// Returns the largest possible file position known so far.
    /// Of course this is not the EOF position as we may not have been offered that yet.
    t_fpos
    SparseVirtualFile::_last_file_position() const noexcept {
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
