//
// Created by Paul Ross on 2020-02-10.
//

#ifndef CPPSVF_SVFS_H
#define CPPSVF_SVFS_H

#include <string>
#include <unordered_map>

#ifdef SVFS_THREAD_SAFE

#include <mutex>

#endif


#include "svf.h"

namespace SVFS {

    // Exception specialisation for the SparseVirtualFile.
    class ExceptionSparseVirtualFileSystem : public std::exception {
    public:
        explicit ExceptionSparseVirtualFileSystem(const std::string &in_msg) : msg(in_msg) {}

        [[nodiscard]] const std::string &message() const { return msg; }

    protected:
        std::string msg;
    };

    class ExceptionSparseVirtualFileSystemOutOfRange : public ExceptionSparseVirtualFileSystem {
    public:
        explicit ExceptionSparseVirtualFileSystemOutOfRange(const std::string &msg) : ExceptionSparseVirtualFileSystem(
                msg) {}
    };

    class ExceptionSparseVirtualFileSystemInsert : public ExceptionSparseVirtualFileSystem {
    public:
        explicit ExceptionSparseVirtualFileSystemInsert(const std::string &msg) : ExceptionSparseVirtualFileSystem(
                msg) {}
    };

    class ExceptionSparseVirtualFileSystemRemove : public ExceptionSparseVirtualFileSystem {
    public:
        explicit ExceptionSparseVirtualFileSystemRemove(const std::string &msg) : ExceptionSparseVirtualFileSystem(
                msg) {}
    };

    class SparseVirtualFileSystem {
    public:
        explicit SparseVirtualFileSystem(const tSparseVirtualFileConfig &config = tSparseVirtualFileConfig()) : \
            m_config(config) {}

        // Insert a new SVF
        void insert(const std::string &id, double mod_time);

        // Remove a specific SVF.
        void remove(const std::string &id);

        // May raise an ExceptionSparseVirtualFileSystemOutOfRange
        [[nodiscard]] const SparseVirtualFile &at(const std::string &id) const;

        // May raise an ExceptionSparseVirtualFileSystemOutOfRange
        SparseVirtualFile &at(const std::string &id);

        // Has an SVF
        [[nodiscard]] bool has(const std::string &id) const noexcept { return m_svfs.find(id) != m_svfs.end(); }

        // Number of SVFs
        [[nodiscard]] size_t size() const noexcept { return m_svfs.size(); }

        // Total estimated memory usage.
        [[nodiscard]] size_t size_of() const noexcept;

        // Total number of file bytes.
        [[nodiscard]] size_t num_bytes() const noexcept;

        // Total number of file blocks.
        [[nodiscard]] size_t num_blocks() const noexcept;

        // All the SVF IDs.
        [[nodiscard]] std::vector<std::string> keys() const noexcept;

        // Eliminate copying.
        SparseVirtualFileSystem(const SparseVirtualFileSystem &rhs) = delete;

        SparseVirtualFileSystem operator=(const SparseVirtualFileSystem &rhs) = delete;

        ~SparseVirtualFileSystem() noexcept;

    protected:
        std::unordered_map<std::string, SparseVirtualFile> m_svfs;
        tSparseVirtualFileConfig m_config;
#ifdef SVFS_THREAD_SAFE
        mutable std::mutex m_mutex;
#endif
    };
}

#endif //CPPSVF_SVFS_H
