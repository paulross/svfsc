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
        explicit ExceptionSparseVirtualFileDiff(const std::string &in_msg) : ExceptionSparseVirtualFileWrite(in_msg) {}
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

    /** Typedef for the file position. */
    typedef size_t t_fpos;
    /** Typedef for a \c seek() followed by a \c read() length. */
    typedef std::pair<t_fpos, size_t> t_seek_read;
    /** Typedef for a vector of (\c seek() followed by a \c read() ) lengths. */
    typedef std::vector<t_seek_read> t_seek_reads;

    /**
     * Configuration for the Sparse Virtual File.
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

    class SparseVirtualFile {
    public:
        /**
         * Create a Sparse Virtual File
         * @param id The unique identifier for this file.
         * @param mod_time The modification time of the remote file in UNIX seconds, this is used for integrity checking.
         * @param config See \c tSparseVirtualFileConfig above.
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
                m_time_read(std::chrono::time_point<std::chrono::system_clock>::min()) {
        }

        // ---- Read and write etc. ----
        /// Do I have the data at the given file position and length?
        bool has(t_fpos fpos, size_t len) const noexcept;

        void write(t_fpos fpos, const char *data, size_t len);

        /** Read data and write to the buffer provided by the caller.
         * Not const as we update m_bytes_read, m_count_read, m_time_read. */
        void read(t_fpos fpos, size_t len, char *p);

        /// Create a new fragmentation list of seek/read instructions.
        t_seek_reads need(t_fpos fpos, size_t len, size_t greedy_length = 0) const noexcept;

        /// Implements the data deletion strategy.
        void clear() noexcept;

        /** Remove the block at the given file position which must be the start of the block.
         * This will raise an ExceptionSparseVirtualFile if the file position is not the start of the block.
         * Returns the length of the block erased. */
        size_t erase(t_fpos fpos);

        // ---- Meta information about the SVF ----
        /// The existing blocks as a list of (file_position, size) pairs.
        t_seek_reads blocks() const noexcept;

        size_t block_size(t_fpos fpos) const;

        // Information about memory used:
        /// size_of() gives best guess of total memory usage.
        size_t size_of() const noexcept;

        /// Gives exact number of data bytes held.
        size_t num_bytes() const noexcept { return m_bytes_total; };

        /// Gives exact number of blocks used.
        size_t num_blocks() const noexcept { return m_svf.size(); }

        /// The position of the last byte.
        t_fpos last_file_position() const noexcept;

        /** Check the clients file modification time has changed.
         * Caller has to decide what to do... */
        bool file_mod_time_matches(const double &file_mod_time) const noexcept {
            return file_mod_time == m_file_mod_time;
        }

        // ---- Attribute access ----
        /// The ID of the file.
        const std::string &id() const noexcept { return m_id; }

        /// The file modification time as a double representing UNIX seconds.
        double file_mod_time() const noexcept { return m_file_mod_time; }

        /// Count of \c write() operations.
        size_t count_write() const noexcept { return m_count_write; }

        /// Count of \c read() operations.
        size_t count_read() const noexcept { return m_count_read; }

        /// Count of total bytes written with \c write() operations.
        size_t bytes_write() const noexcept { return m_bytes_write; }

        /// Count of total bytes read with \c read() operations.
        size_t bytes_read() const noexcept { return m_bytes_read; }

        /// Time of the last \c write() operation.
        /// This can be cast to \c std::chrono::time_point<double>
        std::chrono::time_point<std::chrono::system_clock> time_write() const noexcept { return m_time_write; }

        /// Time of the last \c read() operation.
        /// This can be cast to \c std::chrono::time_point<double>
        std::chrono::time_point<std::chrono::system_clock> time_read() const noexcept { return m_time_read; }

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
        std::string m_id;
        double m_file_mod_time;
        tSparseVirtualFileConfig m_config;
        // Total number of bytes in this SVF
        size_t m_bytes_total = 0;
        // Access statistics
        size_t m_count_write = 0;
        size_t m_count_read = 0;
        // NOTE: These include any duplicate reads/writes.
        size_t m_bytes_write = 0;
        size_t m_bytes_read = 0;
        // Last access times
        std::chrono::time_point<std::chrono::system_clock> m_time_write;
        std::chrono::time_point<std::chrono::system_clock> m_time_read;
        /// Typedef for the data.
        typedef std::vector<char> t_val;
        /// Typedef for the map of file blocks <file_position, data>.
        typedef std::map<t_fpos, t_val> t_map;
        /// The actual SVF.
        t_map m_svf;
#ifdef SVF_THREAD_SAFE
        // This adds about 5-10% execution time compared with a single threaded version.
        mutable std::mutex m_mutex;
#endif
    private:
        // Write data at file position without checks.
        void _write_new_block_after_end(t_fpos fpos, const char *data, size_t len);

        void _write_new_append_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter);

        void _write_append_new_to_old(t_fpos fpos, const char *new_data, size_t new_data_len,
                                      t_map::iterator base_block_iter);

        // NOTE: This is const but read() is not as it updates metadata.
        void _read(t_fpos fpos, size_t len, char *p) const;

        void _throw_diff(t_fpos fpos, const char *data, t_map::const_iterator iter, size_t index_iter) const;

        // Does not use mutex or checks integrity
        t_fpos _file_position_immediatly_after_end() const noexcept;

        t_fpos _file_position_immediatly_after_block(t_map::const_iterator iter) const noexcept;

        static size_t _amount_to_read(t_seek_read iter, size_t greedy_length) noexcept;

        static t_seek_reads _minimise_seek_reads(t_seek_reads seek_reads, size_t greedy_length) noexcept;

        /* Check internal integrity. */
        enum ERROR_CONDITION {
            ERROR_NONE = 0,
            ERROR_EMPTY_BLOCK,
            ERROR_ADJACENT_BLOCKS,
            ERROR_BLOCKS_OVERLAP,
            ERROR_BYTE_COUNT_MISMATCH,
            ERROR_DUPLICATE_BLOCK,
        };

        ERROR_CONDITION integrity() const noexcept;
    };

} // namespace SparseVirtualFileSystem

#endif //CPPSVF_SVF_H
