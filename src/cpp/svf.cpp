//
// Created by Paul Ross on 2020-01-14.
//
#include <strstream>

#include "svf.h"

bool SparseVirtualFile::has(t_fpos fpos, size_t len) const noexcept {
    SVF_ASSERT(integrity() == 0);
    if (m_svf.empty()) {
        return false;
    }
    t_map::const_iterator iter = m_svf.upper_bound(fpos);
    if (iter != m_svf.begin()) {
        --iter;
    }
    t_fpos fpos_end = iter->first + iter->second.size();
    if (fpos >= iter->first && (fpos + len) <= fpos_end) {
        return true;
    }
    return false;
}

void SparseVirtualFile::_write(t_fpos fpos, const char *data, size_t len) {
    t_val vec;
    vec.reserve(len);
    while (len) {
        vec.push_back(*data);
        --len;
        ++data;
    }
    m_svf[fpos] = vec;
}

void SparseVirtualFile::write(t_fpos fpos, const char *data, size_t len) {
    SVF_ASSERT(integrity() == 0);
    // TODO: throw if len == 0
    // Comments are structures like this where ==== are existing blocks and ++++ is the new block.
    // ^==== shows where the iterator is pointing to. fpos is the beginning of the ++++ bl.ock.
    //
    //       ^===========|    |=====|
    //  |+++++++++++++++++++++++++++++++++|
    //
    if (m_svf.empty()) {
        // Simple insert of new data into empty map.
        _write(fpos, data, len);
    } else {
        t_map::const_iterator iter = m_svf.upper_bound(fpos);
        if (iter != m_svf.begin()) {
            --iter;
        }
        size_t index_new = 0;
        size_t index_iter = 0;
        if (iter->first > fpos)  {
            // New comes earlier, copy existing block on to it.
            // Insert new block, possibly coalescing existing blocks.
            if (fpos + len < iter->first) {
                // Simple insert of new data.
                //        ^===========|
                //  |++|
                _write(fpos, data, len);
            } else {
                // Insert the new vector possibly merging existing data.
                t_val vec;
                // Possibilities:
                //       ^===========|    |=====|
                //  |+++|
                //  |++++++++++++|
                //  |++++++++++++++++|
                //  |+++++++++++++++++++|
                // Need to increment iter for subsequent block:
                //  |+++++++++++++++++++++++++++++++++|
                while (iter != m_svf.end()) {
                    // Copy up to the block
                    while (index_new < iter->first - fpos) {
                        vec.push_back(*data);
                        --len;
                        ++data;
                        ++index_new;
                    }
                    index_iter = 0;
                    // Copy the existing block, checking the data is the same.
                    while (index_new < len && index_iter < iter->second.size()) {
                        if (*(data + index_new) != iter->second[index_iter]) {
                            std::ostrstream os;
                            os << "SparseVirtualFile::write():";
                            os << " Difference at position " << fpos + index_new;
                            os << " " << *(data + index_new) << " != " << iter->second[index_iter];
                            throw ExceptionSparseVirtualFileDiff(os.str());
                        }
                        vec.push_back(*data);
                        --len;
                        ++data;
                        ++index_new;
                        ++index_iter;
                    }
                    if (fpos + len > iter->first + iter->second.size()) {
                        // If new overlaps the block then copy from new:
                        //       ^===========|    |=====|
                        //  |++++++++++++++++++++++++|
                        iter = m_svf.erase(iter);
                        if (iter == m_svf.end()) {
                            while (len) {
                                vec.push_back(*data);
                                --len;
                                ++data;
                            }
                        }
                    } else {
                        // Copy the remaining existing block
                        //       ^===========|    |=====|
                        //  |++++++++++++|
                        while (index_iter < iter->second.size()) {
                            vec.push_back(iter->second[index_iter]);
                            ++index_iter;
                        }
                        iter = m_svf.erase(iter);
                    }
                    // Handle this situation by moving on to the next block:
                    //       |===========|    ^=====|
                    //  |++++++++++++++++++++++++|
                    iter = m_svf.erase(iter);
                }
                assert(len == 0);
                assert(! vec.empty());
                m_svf[fpos] = vec;
            }
        } else {
            if (iter->first + iter->second.size() < fpos) {
                //  ^=====|          |=====|
                //          |+++++|
                //          |++++++++++++++++++++|

            } else {
                // Add to existing block, possibly coalescing existing blocks.
                // New must be appended to existing block. Might merge.
                //  ^===========|    |=====|
                //         |++++++++++++++++++|
                index_iter = fpos - iter->first;
                while (index_new < len && index_iter < iter->second.size()) {
                    if (*(data + index_new) != iter->second[index_iter]) {
                        std::ostrstream os;
                        os << "SparseVirtualFile::write():";
                        os << " Difference at position " << fpos + index_new;
                        os << " " << *(data + index_new) << " != " << iter->second[index_iter];
                        throw ExceptionSparseVirtualFileDiff(os.str());
                    }
                }

        }
    }
    SVF_ASSERT(integrity() == 0);
}

// Read data and write to the buffer provided by the caller.
// It is up to the caller to make sure that p can contain len chars.
void SparseVirtualFile::read(t_fpos fpos, size_t len, char *p) const {
    SVF_ASSERT(integrity() == 0);
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
        if (iter->first + iter->second.size() > fpos + len) {
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
}

std::list<std::pair<t_fpos, size_t>>
SparseVirtualFile::need(t_fpos fpos, size_t len) const noexcept {
    SVF_ASSERT(integrity() == 0);
    std::list<std::pair<t_fpos, size_t>> ret;
    t_map::const_iterator iter = m_svf.lower_bound(fpos);
    
    

    return ret;
}

size_t SparseVirtualFile::size_of() const noexcept {
    SVF_ASSERT(integrity() == 0);
    size_t ret = sizeof(SparseVirtualFile);
    
    ret += m_id.size();
    for (auto &iter: m_svf) {
        ret += iter.second.size();
    }
    return ret;
}

// m_coalesce, m_file_mod_time are unchanged.
void SparseVirtualFile::clear() noexcept {
    m_id = "";
    m_file_mod_time = 0.0;
    m_total_bytes = 0;
    m_count_write = 0;
    m_count_read = 0;
    m_bytes_write = 0;
    m_bytes_read = 0;
    m_time_write = std::chrono::time_point<std::chrono::system_clock>::min();
    m_time_read = std::chrono::time_point<std::chrono::system_clock>::min();
    m_svf.clear();
}

SparseVirtualFile::ERROR_CONDITION
SparseVirtualFile::integrity() const noexcept {
    t_fpos prev_fpos;
    t_map::const_iterator iter = m_svf.begin();

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
        prev_fpos = iter->first + iter->second.size();
    }
    return ERROR_NONE;
}

