//
// Created by Paul Ross on 2020-02-10.
//

#include <sstream>
#include <utility>

#include "svfs.h"

namespace SVFS {

    /**
     * Inserts a new SparseVirtualFile corresponding to the given ID and file modification timestamp.
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
            throw ExceptionSparseVirtualFileSystemInsert(os.str());
        }
    }

    /**
     * Remove the SparseVirtualFile corresponding to the given ID.
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
            throw ExceptionSparseVirtualFileSystemRemove(os.str());
        } else {
            m_svfs.erase(iter);
        }
    }

    /**
     * Return the const SparseVirtualFile at the given ID.
     * Will raise a ExceptionSparseVirtualFileSystemOutOfRange is the ID does not exist.
     *
     * @param id The SparseVirtualFile ID.
     * @return The SparseVirtualFile.
     */
    const SparseVirtualFile &SparseVirtualFileSystem::at(const std::string &id) const {
        try {
            return m_svfs.at(id);
        } catch (std::out_of_range &err) {
            throw ExceptionSparseVirtualFileSystemOutOfRange(err.what());
        }
    }

    /**
     * Return the non-const SparseVirtualFile at the given ID.
     * Will raise a ExceptionSparseVirtualFileSystemOutOfRange is the ID does not exist.
     *
     * @param id The SparseVirtualFile ID.
     * @return The SparseVirtualFile.
     */
    SparseVirtualFile &SparseVirtualFileSystem::at(const std::string &id) {
        try {
            return m_svfs.at(id);
        } catch (std::out_of_range &err) {
            throw ExceptionSparseVirtualFileSystemOutOfRange(err.what());
        }
    }

    /**
     * Returns the total in-memory size of the SparseVirtualFileSystem structure in bytes.
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

    /**
     * Returns the total number of readable bytes in the SparseVirtualFileSystem.
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

    /**
     * Returns the total number of blocks in the SparseVirtualFileSystem.
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

    /**
     * Return all the SVF IDs (unordered).
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

    /**
     * Destructor.
     */
    SparseVirtualFileSystem::~SparseVirtualFileSystem() noexcept {
        for (auto &iter: m_svfs) {
            iter.second.clear();
        }
        m_svfs.clear();
    }

}
