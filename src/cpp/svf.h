/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2020-01-14.
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

/**
 * @mainpage


Introduction
=============

Somtimes you don't need the whole file.
Sometimes you don't *want* the whole file.
Especially if it is huge and on some remote server.
But, you might know what parts of the file that you want and @c svfsc can help you store them locally so it looks
<em>as if</em> you have access to the complete file but with just the pieces of interest.

@c svfsc is targeted at reading very large binary files such as TIFF, RP66V1, HDF5 where the structure is well known.
For example you might want to parse a TIFF file for its metadata or for a particular image tile or strip which is a tiny
fraction of the file itself.

@c svfsc implements a *Sparse Virtual File*, a specialised in-memory cache where a particular file might not be
available but <em>parts of it can be obtained</em> without reading the whole file.
A Sparse Virtual File (SVFS::SparseVirtualFile) is represented internally as a map of blocks of data with the key being
their file offsets.
Any write to an SVFS::SparseVirtualFile will coalesce these blocks where possible.

A Sparse Virtual File System (SVFS::SparseVirtualFileSystem) is an extension of this to provide a key/value store where
the key is a file ID and the value a SVFS::SparseVirtualFile.

@c svfsc is written in C++.
It is thread safe.

Usage
------

A SVFS::SparseVirtualFile might be used like this:

- The user requests some data (for example TIFF metadata) from a remote file using a Parser that knows the TIFF structure.
- The Parser consults the SVFS::SparseVirtualFile, if the SVFS::SparseVirtualFile has the data then the Parser parses it and gives the results to the user.
- If the SVFS::SparseVirtualFile does *not* have the data then the Parser consults the SVFS::SparseVirtualFile for what data is needed, then issues the appropriate GET request(s) to the remote server.
- That data is used to update the SVFS::SparseVirtualFile, then the parser can use it and give the results to the user.

Here is a conceptual example of a SVFS::SparseVirtualFile running on a local file system containing data from a
single file.

@code

                CLIENT SIDE           |             LOCAL FILE SYSTEM
                                      .
    /------\      /--------\          |              /-------------\
    | User | <--> | Parser | <-- read(fpos, len) --> | File System |
    \------/      \--------/          |              \-------------/
                       |              .
                       |              |
                   /-------\          .
                   |  SVF  |          |
                   \-------/          .

@endcode

Here is a conceptual example of a SVFS::SparseVirtualFile running with a remote file system.

@code

                CLIENT SIDE           |             SERVER SIDE
                                      .
    /------\      /--------\          |             /--------\
    | User | <--> | Parser | <-- GET(fpos, len) --> | Server |
    \------/      \--------/          |             \--------/
                       |              .                  |
                       |              |                  |
                   /-------\          .           /-------------\
                   |  SVF  |          |           | File System |
                   \-------/          .           \-------------/

@endcode

Example C++ Usage
-------------------

@c svfsc is written in C++ so can be used directly:

@code
    #include "svf.h"

    // Using an arbitrary modification time of 0.0
    SVFS::SparseVirtualFile svf("Some file ID", 0.0);

    // Write six char at file position 14
    svf.write(14, "ABCDEF", 6);

    // Read from it
    char read_buffer[2];
    svf.read(16, 2, read_buffer);

    // What do I have to do to read 24 bytes from file position 8?
    // This returns a std::vector<std::pair<size_t, size_t>>
    // as ((file_position, read_length), ...)
    auto need = svf.need(8, 24);
    // This prints ((8, 6), (20, 4),)
    std::cout << "(";
    for (auto &val: need) {
        std::cout << "(" << val.first << ", " << val.second << "),";
    }
    std::cout << ")" << std::endl;
@endcode

The basic operation is to check if the SVFS::SparseVirtualFile has data, if not then get it and write that data to the
SVFS::SparseVirtualFile.
Then read directly:

@code
        if (!svf.has_data(file_position, length)) {

            // Iterate through the minimal block set to read.
            for (auto &val: svf.need(file_position, length)) {
                // Somehow get the data at (val.first, val.second)...
                // This could be a GET request to a remote file.
                // Then...
                svf.write(val.first, data, val.second)
            }

        }

        // Now read directly
        svf.read(file_position, length)
@endcode
 *
 */

/**
 * @namespace SVFS
 *
 * @brief The namespace for all @c svfsc code.
 */

/**
 * @namespace SVFS::Exceptions
 *
 * @brief The namespace for all Exceptions in @c svfsc code.
 */

/**
 * @namespace SVFS::Test
 *
 * @brief The namespace for all the Test code in @c svfsc code.
 */

#ifndef CPPSVF_SVF_H
#define CPPSVF_SVF_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <cassert>

#ifdef SVF_THREAD_SAFE

#include <mutex>

#endif

#ifdef DEBUG
/** If set then SVF_ASSERT(integrity() == ERROR_NONE); in many places which can extend the runtime by x25 or even more. */
#define SVF_ASSERT(x) assert(x)
#else
/** Empty definition. */
#define SVF_ASSERT(x)
#endif

namespace SVFS {

#pragma mark - Exceptions

    namespace Exceptions {

        /// Exception specialisation for the SparseVirtualFile.
        class ExceptionSparseVirtualFile : public std::exception {
        public:
            explicit ExceptionSparseVirtualFile(const std::string &in_msg) : msg(in_msg) {}

            [[nodiscard]] const std::string &message() const { return msg; }

        protected:
            std::string msg;
        };

        /// Might be thrown during a write operation which fails.
        class ExceptionSparseVirtualFileWrite : public ExceptionSparseVirtualFile {
        public:
            explicit ExceptionSparseVirtualFileWrite(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
        };

        /// Might be thrown during a write operation where the data differs.
        class ExceptionSparseVirtualFileDiff : public ExceptionSparseVirtualFileWrite {
        public:
            explicit ExceptionSparseVirtualFileDiff(const std::string &in_msg) : ExceptionSparseVirtualFileWrite(
                    in_msg) {}
        };

        /// Might be thrown during a write operation where the data differs.
        class ExceptionSparseVirtualFileRead : public ExceptionSparseVirtualFile {
        public:
            explicit ExceptionSparseVirtualFileRead(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
        };

        /// Might be thrown during a erase operation where the file position is not at the exact beginning of a block.
        class ExceptionSparseVirtualFileErase : public ExceptionSparseVirtualFile {
        public:
            explicit ExceptionSparseVirtualFileErase(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
        };

    } //namespace Exceptions {

#pragma mark - typedefs

    /** Typedef for the file position. */
    typedef size_t t_fpos;
    /** Typedef for a \c seek() followed by a \c read() length. */
    typedef std::pair<t_fpos, size_t> t_seek_read;
    /** Typedef for a vector of (\c seek() followed by a \c read() ) lengths. */
    typedef std::vector<t_seek_read> t_seek_reads;
    /** Counter type that increments on every data 'touch' */
    typedef uint32_t t_block_touch;
    /** Map of block touch (smallest is younger) to file position block. */
    typedef std::map<t_block_touch, t_fpos> t_block_touches;

#pragma mark - SVF configuration

    /**
     * @brief Configuration for the Sparse Virtual File.
     */
    typedef struct SparseVirtualFileConfig {
        /**
         * If \c true the memory is destructively overwritten when the Sparse Virtual File is destroyed.
         * See \c test_perf_erase_overwrite_false() and \c test_perf_erase_overwrite_true() for performance comparison.
         * If \c true then \c clear() on a 1Mb SVF typically takes 35 us, if false 1.5 us.
         */
        bool overwrite_on_exit = false;
        /**
         * If \c true compare with existing data on write and if there is a difference throw an exception.
         * This trades performance (if \c false) for correctness (if \c true).
         * See \c test_perf_write_with_diff_check() and \c test_perf_write_without_diff_check()
         * If \c true writing is 0.321 ms, if \c false 0.264 m.
         */
        bool compare_for_diff = true;
    } tSparseVirtualFileConfig;

#pragma mark - The SVF class

    /**
     * @brief Implementation of a *Sparse Virtual File*.
     *
     * A *Sparse Virtual File*, a specialised in-memory cache where a particular
     * file might not be available but *parts of it can be obtained* without reading the whole file.
     * A Sparse Virtual File (SVF) is represented internally as a map of blocks of data with the
     * key being their file offsets. Any write to an SVF will coalesce those blocks where possible.
     */
    class SparseVirtualFile {
    public:
        /**
         * @brief Create a Sparse Virtual File
         *
         * @param id The identifier for this file.
         * @param mod_time The modification time of the remote file in UNIX seconds, this is used for integrity checking.
         * @param config See \c SVFS::SparseVirtualFileConfig.
         */
        explicit SparseVirtualFile(const std::string &id, double mod_time,
                                   const tSparseVirtualFileConfig &config = tSparseVirtualFileConfig()) :
                m_id(id),
                m_file_mod_time(mod_time),
                m_config(config),
                m_bytes_total(0),
                m_count_write(0),
                m_count_read(0),
                m_bytes_write(0),
                m_bytes_read(0),
                m_time_write(std::chrono::time_point<std::chrono::system_clock>::min()),
                m_time_read(std::chrono::time_point<std::chrono::system_clock>::min()),
                m_block_touch(0) {
        }

        // ---- Read and write etc. ----
        /// Do I have the data at the given file position and length?
        [[nodiscard]] bool has(t_fpos fpos, size_t len) const noexcept;

        void write(t_fpos fpos, const char *data, size_t len);

        /** Read data and write to the buffer provided by the caller.
         * Not const as we update m_bytes_read, m_count_read, m_time_read. */
        void read(t_fpos fpos, size_t len, char *p);

        /// Create a new fragmentation list of seek/read instructions.
        [[nodiscard]] t_seek_reads need(t_fpos fpos, size_t len, size_t greedy_length = 0) const noexcept;

        /// Executes the data deletion strategy.
        void clear() noexcept;

        /** Remove the block at the given file position which must be the start of the block.
         * This will raise an ExceptionSparseVirtualFile if the file position is not the start of the block.
         * Returns the length of the block erased. */
        size_t erase(t_fpos fpos);

        // ---- Meta information about the SVF ----
        /// The existing blocks as a list of (file_position, size) pairs.
        [[nodiscard]] t_seek_reads blocks() const noexcept;

        size_t block_size(t_fpos fpos) const;

        // Information about memory used:
        /// size_of() gives best guess of total memory usage.
        [[nodiscard]] size_t size_of() const noexcept;

        /// Gives exact number of data bytes held.
        [[nodiscard]] size_t num_bytes() const noexcept { return m_bytes_total; };

        /// Number of blocks used.
        [[nodiscard]] size_t num_blocks() const noexcept { return m_svf.size(); }

        /// The position of the last byte.
        [[nodiscard]] t_fpos last_file_position() const noexcept;

        /** Check the clients file modification time has changed.
         * Caller has to decide what to do... */
        [[nodiscard]] bool file_mod_time_matches(const double &file_mod_time) const noexcept {
            return file_mod_time == m_file_mod_time;
        }

        // ---- Attribute access ----
        /// The ID of the file.
        [[nodiscard]] const std::string &id() const noexcept { return m_id; }

        /// The file modification time as a double representing UNIX seconds.
        [[nodiscard]] double file_mod_time() const noexcept { return m_file_mod_time; }

        /// The configuration.
        [[nodiscard]] const tSparseVirtualFileConfig &config() const noexcept { return m_config; }

        /// Count of \c write() operations.
        [[nodiscard]] size_t count_write() const noexcept { return m_count_write; }

        /// Count of \c read() operations.
        [[nodiscard]] size_t count_read() const noexcept { return m_count_read; }

        /// Count of total bytes written with \c write() operations.
        [[nodiscard]] size_t bytes_write() const noexcept { return m_bytes_write; }

        /// Count of total bytes read with \c read() operations.
        [[nodiscard]] size_t bytes_read() const noexcept { return m_bytes_read; }

        /// Time of the last \c write() operation.
        /// This can be cast to \c std::chrono::time_point<double>
        [[nodiscard]] std::chrono::time_point<std::chrono::system_clock> time_write() const noexcept {
            return m_time_write;
        }

        /// Time of the last \c read() operation.
        /// This can be cast to \c std::chrono::time_point<double>
        [[nodiscard]] std::chrono::time_point<std::chrono::system_clock> time_read() const noexcept {
            return m_time_read;
        }

        /// Return the latest value of the monotonically increasing block_touch value.
        [[nodiscard]] t_block_touch block_touch() const noexcept { return m_block_touch; }
        [[nodiscard]] t_block_touches block_touches() const noexcept;
        size_t lru_punt(size_t cache_size_upper_bound);

        /// Eliminate copying.
        SparseVirtualFile(const SparseVirtualFile &rhs) = delete;

        /// Eliminate copying.
        SparseVirtualFile operator=(const SparseVirtualFile &rhs) = delete;

#ifdef SVF_THREAD_SAFE

        /// Prohibit moving, the mutex has no move constructor.
        SparseVirtualFile(SparseVirtualFile &&other) = delete;

        SparseVirtualFile &operator=(SparseVirtualFile &&rhs) = delete;

#else
        /// Allow moving
        SparseVirtualFile(SparseVirtualFile &&other) = default;
        SparseVirtualFile& operator=(SparseVirtualFile &&rhs) = default;
#endif

        /// Destruction just clears the internal map.
        ~SparseVirtualFile() { clear(); }

    private:
        /// The SVF ID
        std::string m_id;
        /// The original file modification data a UNIX time. This is used for consistency checking.
        double m_file_mod_time;
        /// The SVF configuration.
        tSparseVirtualFileConfig m_config;
        /// Total number of bytes in this SVF
        size_t m_bytes_total = 0;
        /// Access statistics: count of write operations.
        size_t m_count_write = 0;
        /// Access statistics: count of read operations.
        size_t m_count_read = 0;
        /// Access statistics: total bytes written.
        /// @note These include any duplicate writes.
        size_t m_bytes_write = 0;
        /// Access statistics: total bytes read.
        /// @note These include any duplicate reads.
        size_t m_bytes_read = 0;
        /// Last access real-time timestamp for a write.
        std::chrono::time_point<std::chrono::system_clock> m_time_write;
        /// Last access real-time timestamp for a read.
        std::chrono::time_point<std::chrono::system_clock> m_time_read;
        /// Typedef for the data. This allows for extra per-block fields in the future.
        typedef struct {
            std::vector<char> data;
            // Potentially more fields here such as time of access.
            t_block_touch block_touch;
        } t_val;
        /// Typedef for the map of file blocks <file_position, data>.
        typedef std::map<t_fpos, t_val> t_map;
        /// The actual SVF.
        t_map m_svf;
        /// A monotonically increasing integer that indicates the age of a block, smaller is older.
        t_block_touch m_block_touch;
#ifdef SVF_THREAD_SAFE
        /// Thread mutex. This adds about 5-10% execution time compared with a single threaded version.
        mutable std::mutex m_mutex;
#endif
    private:
        void _throw_diff(t_fpos fpos, const char *data, t_map::const_iterator iter, size_t index_iter) const;

        // Write data at file position without checks.
        void _write_new_block(t_fpos fpos, const char *data, size_t len, t_map::const_iterator hint);

        void _write_new_append_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter);

        void _write_append_new_to_old(t_fpos fpos, const char *new_data, size_t new_data_len,
                                      t_map::iterator base_block_iter);

        // NOTE: This is const but read() is not as it updates metadata.
        void _read(t_fpos fpos, size_t len, char *p) const;

        // Does not use mutex or checks integrity
        [[nodiscard]] t_fpos _file_position_immediatly_after_end() const noexcept;

        [[nodiscard]] t_fpos _file_position_immediatly_after_block(t_map::const_iterator iter) const noexcept;

        [[nodiscard]] static size_t _amount_to_read(t_seek_read iter, size_t greedy_length) noexcept;

        [[nodiscard]] static t_seek_reads
        _minimise_seek_reads(const t_seek_reads &seek_reads, size_t greedy_length) noexcept;

        [[nodiscard]] size_t _erase_no_lock(t_fpos fpos);
        [[nodiscard]] t_block_touches _block_touches_no_lock() const noexcept;

        /** @brief Check result of internal integrity. */
        enum ERROR_CONDITION {
            /// No error.
            ERROR_NONE = 0,
            /// A block is empty.
            ERROR_EMPTY_BLOCK,
            /// Blocks are adjacent and have not been coalesced.
            ERROR_ADJACENT_BLOCKS,
            /// Blocks overlap.
            ERROR_BLOCKS_OVERLAP,
            /// Missmatch in byte count where the count of the bytes in all the blocks does not match @c m_bytes_total.
            ERROR_BYTE_COUNT_MISMATCH,
            /// Duplicate blocks of the same length and at the same file positions.
            ERROR_DUPLICATE_BLOCK,
            /// Two or more blocks have the same block touch value.
            ERROR_DUPLICATE_BLOCK_TOUCH,
        };

        [[nodiscard]] ERROR_CONDITION integrity() const noexcept;
    };

} // namespace SVFS

#endif //CPPSVF_SVF_H
