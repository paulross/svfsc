/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2020-01-14.
 *
 * @verbatim
    MIT License

    Copyright (c) 2020-2023 Paul Ross

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 @endverbatim
 */

#include <iostream>
#include <iterator>
#include <sstream>
#include <set>

#include "svf.h"

namespace SVFS {
    /**
     * @brief Used to overwrite the memory before discarding it (if required).
     */
    static const char OVERWRITE_CHAR = '0';

    /**
     * @brief Returns \c true if this SVF already contains this data.
     *
     * If \c false then \c need() can say what exactly is required.
     *
     * @param fpos File position.
     * @param len Read length.
     * @return \c true if this SVF already contains this data, \c false otherwise.
     */
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
        t_fpos fpos_end = _file_position_immediatly_after_block(iter);
        if (fpos >= iter->first && (fpos + len) <= fpos_end) {
            return true;
        }
        return false;
    }

    /**
     * @brief Throws a ExceptionSparseVirtualFileDiff with an explanation of the data difference.
     *
     * @param fpos File position.
     * @param data The data.
     * @param iter The iterator to the block which has different data.
     * @param index_iter The index into that block where the first data difference starts.
     */
    void
    SparseVirtualFile::_throw_diff(t_fpos fpos, const char *data, t_map::const_iterator iter, size_t index_iter) const {
        assert(data);
        assert(iter != m_svf.end());
        assert(m_config.compare_for_diff);

        if (*data != iter->second.data[index_iter]) {
            std::ostringstream os;
            os << "SparseVirtualFile::write():";
            os << " Difference at position " << fpos;
            os << " '" << *(data) << "' != '" << iter->second.data[index_iter] << "'";
            os << " Ordinal " << static_cast<int>(*data) << " != " << static_cast<int>(iter->second.data[index_iter]);
            std::string str = os.str();
            throw Exceptions::ExceptionSparseVirtualFileDiff(str);
        }
        // Assert as this should now never be called is there is _not_ a diff.
        assert(0);
    }

    /**
     * @brief Write a brand new block into either an empty SVF or beyond the current blocks.
     *
     * Will raise an ExceptionSparseVirtualFileWrite if the write fails.
     * This also updates \c m_bytes_total
     *
     * @param fpos The file position.
     * @param data The data.
     * @param len The length of the data.
     */
    void SparseVirtualFile::_write_new_block(t_fpos fpos, const char *data, size_t len, t_map::const_iterator hint) {
        assert(m_svf.count(fpos) == 0);

        t_val new_value;
        new_value.data.reserve(len);
        new_value.block_touch = m_block_touch++;
        while (len) {
            new_value.data.push_back(*data);
            --len;
            ++data;
            ++m_bytes_total;
        }
        auto size_before_insert = m_svf.size();
        m_svf.insert(hint, {fpos, std::move(new_value)});
        // Sanity check that we really have added a new block (rather than replacing one).
        if (m_svf.size() != 1 + size_before_insert) {
            std::ostringstream os;
            os << "SparseVirtualFile::_write_new_block():";
            os << " Unable to insert new block at " << fpos;
            throw Exceptions::ExceptionSparseVirtualFileWrite(os.str());
        }
    }

    /**
     * @brief Write a new block and append existing blocks to it.
     *
     * We are in either of these situations.
     *
     * Notation:
     *
     *  -# Means original blocks.
     *  -# Is new data to be added
     *  -# Is the result.
     *
     *  And the characters mean:
     *
     * - \c % Is the new file position to write to, argument \c fpos
     * - \c ^ Is the iterator file position, argument \c iter.first
     * - \c = Means original data, argument \c iter.second
     * - \c + Means new data, argument \c data.
     * - \c c Means data checked equal (if required).
     * - \c A Means new data appended or added.
     *
     * @code
     *      1:     ^===========|  |=====|
     *      2: %+++++++++++++++++++++|
     *      3: %AAAAcccccccccccAAAAcc===|
     * @endcode
     * or:
     * @code
     *      1:       ^===========|
     *      2: %+++++++++++++++++++++|
     *      3: %AAAAAAcccccccccccAAAA|
     * @endcode
     * or:
     * @code
     *      1:      ^===========|
     *      2: %+++++++++++++|
     *      3: %AAAAAcccccccc===|
     * @endcode
     *
     * @param fpos The file position of the start of the new block.
     * @param data The data to write.
     * @param len The length of the data.
     * @param iter The iterator of the existing block (\c '^' above).
     */
    void SparseVirtualFile::_write_new_append_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter) {
        SVF_ASSERT(integrity() == ERROR_NONE);
        assert(data);
        assert(len > 0);
        assert(iter != m_svf.end());
        assert(iter->first > fpos);

        size_t fpos_start = fpos;
        size_t fpos_end = fpos + len;
        t_val new_value;
        new_value.block_touch = m_block_touch++;

        while (true) {
            while (len && fpos < iter->first) {
                // Copy new data 'X' up to start of iter.
                //       ^===========|  |=====|
                //  %XXXX+++++++++++++XX++|
                new_value.data.push_back(*data);
                ++data;
                ++fpos;
                --len;
                ++m_bytes_total;
            }
            size_t index_iter = 0;
            size_t delta = std::min(len, iter->second.data.size());
            // Check overlapped data matches 'Y'
            //       ^===========|  |=====|
            //  %++++YYYYYYYYYYYYY++++|
            if (m_config.compare_for_diff) {
                if (std::memcmp(iter->second.data.data(), data, delta) != 0) {
                    _throw_diff(fpos, data, iter, 0);
                }
            }
            for (size_t i = 0; i < delta; ++i) {
                new_value.data.push_back(*data);
                ++data;
            }
            fpos += delta;
            len -= delta;
            index_iter += delta;
            if (_file_position_immediatly_after_block(iter) > fpos_end) {
                // Copy rest of iter 'Z'
                assert(len == 0);
                //       ^=========ZZZ
                //  %+++++++++++++|
                // So append up to the end of iter and (maybe) go round again.
                while (index_iter < iter->second.data.size()) {
                    new_value.data.push_back(iter->second.data[index_iter]);
                    ++index_iter;
                }
                if (m_config.overwrite_on_exit) {
                    iter->second.data.assign(iter->second.data.size(), OVERWRITE_CHAR);
                }
                m_svf.erase(iter);
                break;
            }
            // Remove copied and checked old block and move on.
            if (m_config.overwrite_on_exit) {
                iter->second.data.assign(iter->second.data.size(), OVERWRITE_CHAR);
            }
            iter = m_svf.erase(iter);
            if (iter == m_svf.end() || iter->first > fpos + len) {
                // Copy rest of new and break
                while (len) {
                    new_value.data.push_back(*data);
                    ++data;
                    ++fpos;
                    --len;
                    ++m_bytes_total;
                }
                break;
            }
        }
        auto size_before_insert = m_svf.size();
        m_svf.insert({fpos_start, std::move(new_value)});
        if (m_svf.size() != 1 + size_before_insert) {
            std::ostringstream os;
            os << "SparseVirtualFile::write():";
            os << " Unable to insert new block at " << fpos_start;
            throw Exceptions::ExceptionSparseVirtualFileWrite(os.str());
        }
        assert(len == 0);
        assert(fpos == fpos_end);
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    /**
     * @brief From file position, write the new_data to the block identified by base_block_iter.
     * This may involve coalescing existing blocks that follow base_block_iter.
     *
     * We are in these kind of situations:
     *
     * Notation:
     *
     *  -# Means original blocks.
     *  -# Is new data to be added
     *  -# Is the result.
     *
     *  And the characters mean:
     *
     * - \c % Is the new file position to write to, argument \c fpos
     * - \c ^ Is the iterator file position, argument \c base_block_iter.first
     * - \c = Means original data, argument \c base_block_iter.second
     * - \c + Means new data, argument \c new_data.
     * - \c c Means data checked equal (if required).
     * - \c A Means new data appended or added.
     *
     * @code
     *      1: ^===========|    |=====|
     *      2: %+++++++++|
     *      3: ^ccccccccc==|    |=====|
     *
     *      1: ^===========|    |=====|
     *      2:    %++++++|
     *      3: ^===cccccc==|    |=====|
     *
     *      1: ^===========|    |=====|
     *      2:    %+++++++++++|
     *      3: ^===========AAA| |=====|
     *
     *      1: ^===========|    |=====|
     *      2:    %++++++++++++++++++|
     *      3: ^===ccccccccAAAAAAcccc=|
     *
     *      1: ^===========|    |=====|    |=====|
     *      2:    |++++++++++++++++++++++++++|
     *      3: ^===ccccccccAAAAAAcccccAAAAAAc====|
     * @endcode
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
        assert(fpos <= _file_position_immediatly_after_block(base_block_iter));

#ifdef DEBUG
        size_t fpos_end = fpos + new_data_len;
#endif
        ++m_block_touch;
        // Diff check against base_block_iter
        // Do the check to end of new_data_len or end of base_block_iter which ever comes first.
        // Do not increment m_bytes_total as this is existing new_data.
        // Diff check against base_block_iter
        size_t write_index_from_block_start = fpos - base_block_iter->first;
        // Do the check to end of new_data_len or end of base_block_iter which ever comes first.
        size_t len_check_or_copy = std::min(new_data_len,
                                            base_block_iter->second.data.size() - write_index_from_block_start);
        if (m_config.compare_for_diff) {
            if (std::memcmp(base_block_iter->second.data.data() + write_index_from_block_start, new_data,
                            len_check_or_copy) != 0) {
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
                    base_block_iter->second.data.push_back(*new_data);
                    ++new_data;
                    ++fpos;
                    --new_data_len;
                    m_bytes_total += 1;
                }
                break; // Done. Needed because we are going to do erase(next_block_iter) otherwise.
            } else {
                // Copy the new_data up to start of next_block_iter or, we have exhausted the new_data.
                while (new_data_len && fpos < next_block_iter->first) {
                    base_block_iter->second.data.push_back(*new_data);
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
            len_check_or_copy = std::min(new_data_len, _file_position_immediatly_after_block(next_block_iter) - fpos);
            if (len_check_or_copy) {
                if (m_config.compare_for_diff) {
                    if (std::memcmp(next_block_iter->second.data.data(), new_data, len_check_or_copy) != 0) {
                        _throw_diff(fpos, new_data, base_block_iter, 0);
                    }
                }
                // We could push_back either the new_data or the existing next block data. We choose the former.
                for (size_t i = 0; i < len_check_or_copy; ++i) {
                    base_block_iter->second.data.push_back(*new_data);
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
                while (write_index_from_block_start < next_block_iter->second.data.size()) {
                    base_block_iter->second.data.push_back(next_block_iter->second.data[write_index_from_block_start]);
                    ++write_index_from_block_start;
                }
            }
            // New data is not exhausted so erase next_block_iter as we have copied it and move on to the next block.
            if (m_config.overwrite_on_exit) {
                next_block_iter->second.data.assign(next_block_iter->second.data.size(), OVERWRITE_CHAR);
            }
            next_block_iter = m_svf.erase(next_block_iter);
        }
        base_block_iter->second.block_touch = m_block_touch;
        assert(new_data_len == 0);
#ifdef DEBUG
        assert(fpos == fpos_end);
#endif
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    /**
     * @brief Write the data a the given file position.
     *
     * This will either:
     *
     * - Write a brand new block independent of all the others.
     * - Write a new block and coalesce other blocks onto its end.
     * - Coalesce the new block onto an existing block and possibly others.
     *
     * Comments are structures like this where:
     *
     * - \c ==== are existing blocks.
     * - \c ++++ is the new block. File position is the beginning of the \c ++++ block.
     * - \c ^==== shows where the iterator is pointing to.
     *
     * Notation:
     *
     *  -# Means original blocks.
     *  -# Is new data to be added
     *  -# Is the result.
     *
     *  And the characters mean:
     *
     * - \c = Means original data.
     * - \c + Means new data.
     * - \c c Means data checked equal (if required).
     * - \c A Means new data appended or added.
     *
     * @code
     *      1:       ^===========|    |=====|
     *      2: |+++++++++++++++++++++++++++++++++|
     *      3: |AAAAAAcccccccccccAAAAAAcccccAAAAA|
     * @endcode
     *
     * This also updates the write count, the number of bytes written and the last write time.
     *
     * If ``SVF_THREAD_SAFE`` is defined then this will acquire a lock on this ``SparseVirtualFile``.
     *
     * @param fpos The file position to write to.
     * @param data The data, assumed to be of the given length.
     * @param len The length to the data to write.
     */
    void SparseVirtualFile::write(t_fpos fpos, const char *data, size_t len) {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        // TODO: throw if !data, len == 0
        if (m_svf.empty() || fpos > _file_position_immediatly_after_end()) {
            // Simple insert of new data into empty map or a node beyond the end (common case).
            _write_new_block(fpos, data, len, m_svf.begin());
        } else {
            t_map::iterator iter = m_svf.upper_bound(fpos);
            if (iter != m_svf.begin()) {
                --iter;
            }
            if (iter->first > fpos) {
                // Insert new block, possibly coalescing existing blocks.
                // New comes earlier so either create a new block or copy existing block on to it.
                if (iter->first <= fpos + len) {
                    // Need to coalesce
                    _write_new_append_old(fpos, data, len, iter);
                } else {
                    // The new block precedes the old one
                    _write_new_block(fpos, data, len, iter);
                }
            } else {
                // Existing block.first is <= fpos
                if (fpos > _file_position_immediatly_after_block(iter)) {
                    // No overlap so just write new block
                    _write_new_block(fpos, data, len, m_svf.end());
                } else {
                    // Append new to existing block, possibly coalescing existing blocks.
                    _write_append_new_to_old(fpos, data, len, iter);
                }
            }
        }
        // Update internals.
        m_count_write += 1;
        m_bytes_write += len;
        m_time_write = std::chrono::system_clock::now();
        SVF_ASSERT(integrity() == ERROR_NONE);
    }

    /**
     * @brief Read data and write to the buffer provided by the caller.
     * This is the const method as it does not update the internals.
     *
     * @param fpos File position to start the read.
     * @param len Length of the read.
     * @param p Buffer to copy the data into. It is up to the caller to make sure that p can contain len chars.
     */
    void SparseVirtualFile::_read(t_fpos fpos, size_t len, char *p) const {
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        SVF_ASSERT(integrity() == ERROR_NONE);

        if (m_svf.empty()) {
            throw Exceptions::ExceptionSparseVirtualFileRead("SparseVirtualFile::read(): Sparse virtual file is empty.");
        }
        t_map::const_iterator iter = m_svf.lower_bound(fpos);
        if (iter == m_svf.begin() && iter->first != fpos) {
            std::ostringstream os;
            os << "SparseVirtualFile::read():";
            os << " Requested file position " << fpos << " precedes first block at " << iter->first;
            throw Exceptions::ExceptionSparseVirtualFileRead(os.str());
        }
        size_t offset_into_block = 0;
        if (iter == m_svf.end() || iter->first != fpos) {
            --iter;
            offset_into_block = fpos - iter->first;
        }
        if (offset_into_block + len > iter->second.data.size()) {
            std::ostringstream os;
            os << "SparseVirtualFile::read():";
            os << " Requested position " << fpos << " length " << len;
            os << " (end " << fpos + len << ")";
            os << " overruns block that starts at " << iter->first << " has size " << iter->second.data.size();
            os << " (end " << iter->first + iter->second.data.size() << ").";
            os << " Offset into block is " << offset_into_block;
            os << " overrun is " << offset_into_block + len - iter->second.data.size() << " bytes";
            throw Exceptions::ExceptionSparseVirtualFileRead(os.str());
        }
        if (memcpy(p, iter->second.data.data() + offset_into_block, len) != p) {
            std::ostringstream os;
            os << "SparseVirtualFile::read():";
            os << " memcpy failed " << fpos << " length " << len;
            throw Exceptions::ExceptionSparseVirtualFileRead(os.str());

        }
    }

    /**
     * @brief Read data and write to the buffer provided by the caller.
     * This also updated the non-const members.
     *
     * @param fpos File position to start the read.
     * @param len Length of the read.
     * @param p Buffer to copy the data into. It is up to the caller to make sure that p can contain len chars.
     */
    void SparseVirtualFile::read(t_fpos fpos, size_t len, char *p) {
        _read(fpos, len, p);
        // Adjust non-const members
        m_bytes_read += len;
        m_count_read += 1;
        m_time_read = std::chrono::system_clock::now();
    }

    /**
     * @brief Given a file position and a length what data do I need that I don't yet have?
     *
     * This takes an optional argument @c greedy_length that can be useful when dealing with remote files on high
     * latency networks.
     * The @c greedy_length will determine the then minimum read length and this may well change a series of small
     * reads into a shorter series of larger reads.
     * This might well improve the read performance at the expense of cacheing extra unused data.
     *
     * @warning The SVF has no knowledge of the the actual file size so when using a greedy length the need list
     * might include positions beyond EOF.
     * For example a file 1024 bytes long and a greedy length of 256 then <tt>need(1000, 24, 256)</tt> will create
     * a need list of [(1000, 256),].
     * This should generate a <tt>write(1000, 24)</tt> not a <tt>write(1000, 256)</tt>.
     * It is up to the caller to handle this, however, @c reads() in C/C++/Python will ignore read lengths past EOF
     * so the caller does not have to do anything.
     *
     * @param fpos File position at the start of the attempted read.
     * @param len Length of the attempted read.
     * @param greedy_length If greater than zero this makes greedy, fewer but larger, reads.
     * @return A vector of pairs (file_position, length) that this SVF needs.
     */
    t_seek_reads SparseVirtualFile::need(t_fpos fpos, size_t len, size_t greedy_length) const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif

        if (m_svf.empty()) {
            return {{fpos, greedy_length > len ? greedy_length : len}};
        }
        size_t original_len = len;
        t_fpos fpos_to = fpos + len;
        t_seek_reads ret;
        t_map::const_iterator iter = m_svf.upper_bound(fpos);
        if (iter == m_svf.begin()) {
            if (fpos + len <= iter->first) {
                //        ^==|
                //   |+++|
                ret.push_back({fpos, len});
                fpos += len;
                // Mark that we are done.
                len = 0;
            }
        } else {
            // Otherwise check the previous node with std::prev.
            auto last_fpos = _file_position_immediatly_after_block(std::prev(iter));
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
        // Now walk through the remaining nodes until we have exhausted the read.
        while (len) {
            if (iter == m_svf.end() || fpos + len <= iter->first) {
                // Either:
                //   |==|
                //         |++++++++++++|
                // or:
                //              |==|
                //   |+++++|
                ret.emplace_back(fpos, len);
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
                ret.emplace_back(fpos, bytes_added);
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
            if (fpos + len <= _file_position_immediatly_after_block(iter)) {
                //          ^======|
                //          |++++|
                fpos += len;
                len = 0;
                break;
            } else {
                //          ^======|
                //          |+++++++++|
                fpos += iter->second.data.size();
                len -= iter->second.data.size();
            }
            ++iter;
        }
        assert(fpos == fpos_to);
        assert(len == 0);
        if (greedy_length && greedy_length > original_len && !ret.empty()) {
            ret = _minimise_seek_reads(ret, greedy_length);
        }
        return ret;
    }

    /**
     * @brief Returns the maximal length to read given a greedy length.
     *
     * @param iter The current block.
     * @param greedy_length The greed length read.
     * @return Maximal read value.
     */
    size_t SparseVirtualFile::_amount_to_read(t_seek_read iter, size_t greedy_length) noexcept {
        return iter.second > greedy_length ? iter.second : greedy_length;
    }

    /**
     * @brief May reduce the list of file position/lengths by coalescing them if possible up to a limit @c greedy_length.
     *
     * @param seek_reads Vector of minimal seek/reads.
     * @param greedy_length Maximal length that allows coalescing.
     * @return New vector of maximal seek/reads.
     */
    t_seek_reads
    SparseVirtualFile::_minimise_seek_reads(const t_seek_reads &seek_reads, size_t greedy_length) noexcept {
        assert (greedy_length != 0);

        t_seek_reads new_seek_reads;
        for (const t_seek_read &seek_read: seek_reads) {
            if (new_seek_reads.empty()) {
                // Add the first greedy block
                new_seek_reads.emplace_back(seek_read.first, _amount_to_read(seek_read, greedy_length));
            } else {
                // Compare with last new block
                auto last_iter_of_new = new_seek_reads.end();
                --last_iter_of_new;
                if (seek_read.first > last_iter_of_new->first + last_iter_of_new->second) {
                    // Add a new greedy block
                    new_seek_reads.emplace_back(seek_read.first, _amount_to_read(seek_read, greedy_length));
                } else if (seek_read.first + seek_read.second > last_iter_of_new->first + last_iter_of_new->second) {
                    // Extend last block
                    last_iter_of_new->second +=
                            (seek_read.first + seek_read.second) - (last_iter_of_new->first + last_iter_of_new->second);
                } // Otherwise do nothing, it is covered by greedy.
            }
        }
        return new_seek_reads;
    }

    /**
     * @brief Returns a description of the current blocks as a vector of (file_position, length).
     *
     * @return The currently held blocks.
     */
    t_seek_reads SparseVirtualFile::blocks() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif

        t_seek_reads ret;
        for (const auto &iter: m_svf) {
            ret.emplace_back(iter.first, iter.second.data.size());
        }
        return ret;
    }

    /**
     * @brief The length of the block at a specific file position.
     *
     * This will throw a \c ExceptionSparseVirtualFileRead if the file position is not in the block entries.
     *
     * @param fpos File position, this must be at the start of a block.
     * @return The block size.
     */
    size_t SparseVirtualFile::block_size(t_fpos fpos) const {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif

        if (m_svf.empty()) {
            throw Exceptions::ExceptionSparseVirtualFileRead("SparseVirtualFile::block_size(): Sparse virtual file is empty.");
        }
        t_map::const_iterator iter = m_svf.find(fpos);
        if (iter == m_svf.end()) {
            std::ostringstream os;
            os << "SparseVirtualFile::block_size():";
            os << " Requested file position " << fpos << " is not at the start of a block";
            throw Exceptions::ExceptionSparseVirtualFileRead(os.str());
        }
        return iter->second.data.size();
    }

    /**
     * @brief Returns the total memory usage of this SVF.
     *
     * @return Memory used.
     */
    size_t SparseVirtualFile::size_of() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        size_t ret = sizeof(SparseVirtualFile);

        // Add heap referenced data sizes.
        ret += m_id.size();
        for (const auto &iter: m_svf) {
            ret += sizeof(iter.first);
            ret += sizeof(iter.second);
            ret += iter.second.data.size();
        }
        return ret;
    }

    /**
     * @brief Clears this Sparse Virtual File.
     *
     * This removes all data and resets the internal counters.
     *
     * @note m_coalesce, m_file_mod_time are maintained.
     *
     * @note m_time_write, m_time_read are maintained.
     */
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
                iter.second.data.assign(iter.second.data.size(), OVERWRITE_CHAR);
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

    /**
     * @brief Remove a particular block.
     *
     * This will raise an ExceptionSparseVirtualFileErase if the file position is not exactly at the start of a block.
     *
     * @param fpos File position of the start of the block.
     * @return Size of the block that was removed.
     */
    size_t SparseVirtualFile::_erase_no_lock(t_fpos fpos) {
        SVF_ASSERT(integrity() == ERROR_NONE);

        auto iter = m_svf.find(fpos);
        if (iter == m_svf.end()) {
            std::ostringstream os;
            os << "SparseVirtualFile::erase():";
            os << " Non-existent file position " << fpos << " at start of block.";
            throw Exceptions::ExceptionSparseVirtualFileErase(os.str());
        }
        size_t ret = iter->second.data.size();
        m_bytes_total -= ret;
        m_svf.erase(iter);
        return ret;
    }

    /**
     * @brief Remove a particular block.
     *
     * This will raise an ExceptionSparseVirtualFileErase if the file position is not exactly at the start of a block.
     *
     * @param fpos File position of the start of the block.
     * @return Size of the block that was removed.
     */
    size_t SparseVirtualFile::erase(t_fpos fpos) {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        return _erase_no_lock(fpos);
    }

    /**
     * @brief Internal integrity check.
     *
     * This checks for these error conditions:
     *
     * - Empty block.
     * - Duplicate block.
     * - Adjacent blocks.
     * - Overlapping blocks.
     * - Byte count missmatch.
     *
     * @return An error condition or \c ERROR_NONE if the integrity is correct.
     */
    SparseVirtualFile::ERROR_CONDITION
    SparseVirtualFile::integrity() const noexcept {
        t_fpos prev_fpos;
        size_t prev_size;
        t_map::const_iterator iter = m_svf.begin();
        size_t byte_count = 0;
        std::set<t_block_touch> block_touches;

        while (iter != m_svf.end()) {
            t_fpos fpos = iter->first;
            size_t size = iter->second.data.size();
            if (iter->second.data.empty()) {
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
            if (block_touches.find(iter->second.block_touch) != block_touches.end()) {
                // Duplicate block_touches value
                return ERROR_DUPLICATE_BLOCK_TOUCH;
            }
            prev_fpos = iter->first;
            prev_size = iter->second.data.size();
            byte_count += prev_size;
            ++iter;
        }
        if (byte_count != m_bytes_total) {
            return ERROR_BYTE_COUNT_MISMATCH;
        }
        return ERROR_NONE;
    }

    /**
     * @brief Returns the largest possible file position known so far.
     *
     * Of course this is not the EOF position as we may not have been offered that yet.
     *
     * @return The last known file position.
     */
    t_fpos
    SparseVirtualFile::last_file_position() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        return _file_position_immediatly_after_end();
    }

    /**
     * @brief Returns a \c std::map of latest touch value key and file position value.
     *
     * Callers can use this to make informed decisions about punting older blocks.
     *
     * @return A map std::map<t_block_touch, t_fpos>.
     */
    [[nodiscard]] t_block_touches SparseVirtualFile::_block_touches_no_lock() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
        t_block_touches ret;
        for (const auto &iter: m_svf) {
            // The block_touch should not be in the return value, yet.
            assert(ret.find(iter.second.block_touch) == ret.end());
            ret[iter.second.block_touch] = iter.first;
        }
        return ret;
    }

    /**
     * @brief Returns a \c std::map of latest touch value key and file position value.
     *
     * Callers can use this to make informed decisions about punting older blocks.
     *
     * @return A map std::map<t_block_touch, t_fpos>.
     */
    [[nodiscard]] t_block_touches SparseVirtualFile::block_touches() const noexcept {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        return _block_touches_no_lock();
    }

    /**
     * Implements a simple punting strategy based a Last Recently Used blocks.
     * This brings the cache size to < cache_size_upper_bound but leaving at least one block in place.
     *
     * This will block in a multi-threaded environment.
     *
     * @param cache_size_upper_bound The upper bound of the final cache size.
     */
    size_t SparseVirtualFile::lru_punt(size_t cache_size_upper_bound) {
        SVF_ASSERT(integrity() == ERROR_NONE);
#ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        size_t ret = 0;
        if (m_svf.size() > 1 and m_bytes_total >= cache_size_upper_bound) {
            auto touch_fpos_map = _block_touches_no_lock();
            for (const auto &iter: touch_fpos_map) {
                if (m_svf.size() > 1 and m_bytes_total >= cache_size_upper_bound) {
                    ret += _erase_no_lock(iter.second);
                } else {
                    break;
                }
            }
        }
        return ret;
    }

    /**
     * @brief Returns the file position immediately after the last block.
     *
     * Of course this is not the EOF position as we may not have been offered that yet.
     * This does not have use a lock.
     *
     * @return The last know file position.
     */
    t_fpos
    SparseVirtualFile::_file_position_immediatly_after_end() const noexcept {
        if (m_svf.empty()) {
            return 0;
        } else {
            auto iter = m_svf.end();
            --iter;
            return _file_position_immediatly_after_block(iter);
        }
    }

    /**
     * @brief Returns the file position immediately after the particular block.
     * 
     * Example file position 4, block length 2 this returns 6.
     *
     * @param iter The iterator to the block.
     * @return Last known file position.
     */
    t_fpos
    SparseVirtualFile::_file_position_immediatly_after_block(t_map::const_iterator iter) const noexcept {
        // NOTE: do not SVF_ASSERT(integrity() == ERROR_NONE); as integrity() calls this so infinite recursion.
        assert(iter != m_svf.end());

        auto ret = iter->first + iter->second.data.size();
        return ret;
    }

} // namespace SVFS
