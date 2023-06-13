/** @file
 *
 * A Sparse Virtual File System implementation.
 *
 * Created by Paul Ross on 2020-02-10.
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

#ifndef CPPSVF_SVFS_H
#define CPPSVF_SVFS_H

#include <string>
#include <unordered_map>

#ifdef SVFS_THREAD_SAFE

#include <mutex>

#endif


#include "svf.h"

namespace SVFS {

#pragma mark - Exceptions

    /** Exception specialisation for the SparseVirtualFileSystem. */
    class ExceptionSparseVirtualFileSystem : public std::exception {
    public:
        explicit ExceptionSparseVirtualFileSystem(const std::string &in_msg) : msg(in_msg) {}

        [[nodiscard]] const std::string &message() const { return msg; }

    protected:
        std::string msg;
    };

    /** Exception specialisation for out of range error. */
    class ExceptionSparseVirtualFileSystemOutOfRange : public ExceptionSparseVirtualFileSystem {
    public:
        explicit ExceptionSparseVirtualFileSystemOutOfRange(const std::string &msg) : ExceptionSparseVirtualFileSystem(
                msg) {}
    };

    /** Exception specialisation on insert error. */
    class ExceptionSparseVirtualFileSystemInsert : public ExceptionSparseVirtualFileSystem {
    public:
        explicit ExceptionSparseVirtualFileSystemInsert(const std::string &msg) : ExceptionSparseVirtualFileSystem(
                msg) {}
    };

    /** Exception specialisation on remove error. */
    class ExceptionSparseVirtualFileSystemRemove : public ExceptionSparseVirtualFileSystem {
    public:
        explicit ExceptionSparseVirtualFileSystemRemove(const std::string &msg) : ExceptionSparseVirtualFileSystem(
                msg) {}
    };

#pragma mark - The SVFS class

    /**
     * A SparseVirtualFileSystem is a key/value stor where the key is a file ID as a string and the value is a
     * SparseVirtualFile.
     */
    class SparseVirtualFileSystem {
    public:
        /** Constructor takes a tSparseVirtualFileConfig that is passed to every new SparseVirtualFile */
        explicit SparseVirtualFileSystem(const tSparseVirtualFileConfig &config = tSparseVirtualFileConfig()) : \
            m_config(config) {}

        // Insert a new SVF
        void insert(const std::string &id, double mod_time);

        // Remove a specific SVF.
        void remove(const std::string &id);

        // May raise an ExceptionSparseVirtualFileSystemOutOfRange
        [[nodiscard]] const SparseVirtualFile &at(const std::string &id) const;

        // May raise an ExceptionSparseVirtualFileSystemOutOfRange
        [[nodiscard]] SparseVirtualFile &at(const std::string &id);

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

        /// Eliminate copying.
        SparseVirtualFileSystem(const SparseVirtualFileSystem &rhs) = delete;

        /// Eliminate copying.
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
