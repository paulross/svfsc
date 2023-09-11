/** @file
 *
 * A Sparse Virtual File System implementation.
 *
 * Created by Paul Ross on 2020-02-10.
 *
 * @verbatim
    MIT License

    Copyright (c) 2023 Paul Ross

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

#include <sstream>
#include <utility>

#include "svfs.h"

namespace SVFS {

    /** @brief Inserts a new SparseVirtualFile corresponding to the given ID and file modification timestamp.
     *
     * @param id The file ID.
     * @param mod_time The file modification time.
     */
    void SparseVirtualFileSystem::insert(const std::string &id, double mod_time) {
#ifdef SVFS_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        auto result = m_svfs.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(id),
                                     std::forward_as_tuple(id, mod_time, m_config));
        if (! result.second) {
            // Error, insertion failed
            std::ostringstream os;
            os << "SparseVirtualFileSystem::insert():";
            os << " can not insert \"" << id << "\"";
            throw Exceptions::ExceptionSparseVirtualFileSystemInsert(os.str());
        }
    }

    /** @brief Remove the SparseVirtualFile corresponding to the given ID.
     *
     * This will raise an ExceptionSparseVirtualFileSystemRemove if the given ID does not exist.
     *
     * @param id The SparseVirtualFile ID.
     */
    void SparseVirtualFileSystem::remove(const std::string &id) {
#ifdef SVFS_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        auto iter = m_svfs.find(id);
        if (iter == m_svfs.end()) {
            std::ostringstream os;
            os << "SparseVirtualFileSystem::remove():";
            os << " id \"" << id << "\" not found.";
            throw Exceptions::ExceptionSparseVirtualFileSystemRemove(os.str());
        } else {
            m_svfs.erase(iter);
        }
    }

    /** @brief Return the const SparseVirtualFile at the given ID.
     *
     * Will raise a ExceptionSparseVirtualFileSystemOutOfRange is the ID does not exist.
     *
     * @param id The SparseVirtualFile ID.
     * @return The SparseVirtualFile.
     */
    const SparseVirtualFile &SparseVirtualFileSystem::at(const std::string &id) const {
        try {
            return m_svfs.at(id);
        } catch (std::out_of_range &err) {
            throw Exceptions::ExceptionSparseVirtualFileSystemOutOfRange(err.what());
        }
    }

    /** @brief Return the non-const SparseVirtualFile at the given ID.
     *
     * Will raise a ExceptionSparseVirtualFileSystemOutOfRange is the ID does not exist.
     *
     * @param id The SparseVirtualFile ID.
     * @return The SparseVirtualFile.
     */
    SparseVirtualFile &SparseVirtualFileSystem::at(const std::string &id) {
        try {
            return m_svfs.at(id);
        } catch (std::out_of_range &err) {
            throw Exceptions::ExceptionSparseVirtualFileSystemOutOfRange(err.what());
        }
    }

    /** @brief Returns the total in-memory size of the SparseVirtualFileSystem structure in bytes.
     *
     * @return Memory size.
     */
    size_t SparseVirtualFileSystem::size_of() const noexcept {
        size_t ret = sizeof(SparseVirtualFileSystem);
        for (auto &iter: m_svfs) {
            ret += iter.second.size_of();
        }
        return ret;
    }

    /** @brief Returns the total number of readable bytes in the SparseVirtualFileSystem.
     *
     * @return Readable size.
     */
    size_t SparseVirtualFileSystem::num_bytes() const noexcept {
        size_t ret = 0;
        for (auto &iter: m_svfs) {
            ret += iter.second.num_bytes();
        }
        return ret;
    }

    /** @brief Returns the total number of blocks in the SparseVirtualFileSystem.
     *
     * @return Total number of blocks.
     */
    size_t SparseVirtualFileSystem::num_blocks() const noexcept {
        size_t ret = 0;
        for (auto &iter: m_svfs) {
            ret += iter.second.num_blocks();
        }
        return ret;
    }

    /** @brief Return all the SVF IDs (unordered).
     *
     * @return All the SVF IDs.
     */
    std::vector<std::string> SparseVirtualFileSystem::keys() const noexcept {
        std::vector<std::string> ret;
        ret.reserve(m_svfs.size());
        for(const auto &iter: m_svfs) {
            ret.push_back(iter.first);
        }
        return ret;
    }

    /** @brief Destructor. */
    SparseVirtualFileSystem::~SparseVirtualFileSystem() noexcept {
        for (auto &iter: m_svfs) {
            iter.second.clear();
        }
        m_svfs.clear();
    }

}
