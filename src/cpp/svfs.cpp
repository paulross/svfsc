//
// Created by Paul Ross on 2020-02-10.
//

#include <sstream>
#include <utility>

#include "svfs.h"

namespace SVFS {

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

    void SparseVirtualFileSystem::remove(const std::string &id) {
#ifdef SVFS_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
#endif
        auto iter = m_svfs.find(id);
        if (iter == m_svfs.end()) {
            std::ostringstream os;
            os << "SparseVirtualFileSystem::rfemove():";
            os << " id \"" << id << "\" not found.";
            throw ExceptionSparseVirtualFileSystemRemove(os.str());
        } else {
            m_svfs.erase(iter);
        }
    }

    const SparseVirtualFile &SparseVirtualFileSystem::at(const std::string &id) const {
        try {
            return m_svfs.at(id);
        } catch (std::out_of_range &err) {
            throw ExceptionSparseVirtualFileSystemOutOfRange(err.what());
        }
    }

    SparseVirtualFile &SparseVirtualFileSystem::at(const std::string &id) {
        try {
            return m_svfs.at(id);
        } catch (std::out_of_range &err) {
            throw ExceptionSparseVirtualFileSystemOutOfRange(err.what());
        }
    }

    size_t SparseVirtualFileSystem::size_of() const noexcept {
        size_t ret = sizeof(SparseVirtualFileSystem);
        for (auto &iter: m_svfs) {
            ret += iter.second.size_of();
        }
        return ret;
    }

    size_t SparseVirtualFileSystem::num_bytes() const noexcept {
        size_t ret = 0;
        for (auto &iter: m_svfs) {
            ret += iter.second.num_bytes();
        }
        return ret;
    }

    size_t SparseVirtualFileSystem::num_blocks() const noexcept {
        size_t ret = 0;
        for (auto &iter: m_svfs) {
            ret += iter.second.num_blocks();
        }
        return ret;
    }

    SparseVirtualFileSystem::~SparseVirtualFileSystem() noexcept {
        for (auto &iter: m_svfs) {
            iter.second.clear();
        }
        m_svfs.clear();
    }

}
