//
// Created by Paul Ross on 2020-01-14.
//

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
#define SVF_ASSERT(x) assert(x)
#else
#define SVF_ASSERT(x)
#endif

namespace SVFS {

// Exception specialisation for the SparseVirtualFile.
    class ExceptionSparseVirtualFile : public std::exception {
    public:
        explicit ExceptionSparseVirtualFile(const std::string &in_msg) : msg(in_msg) {}
        [[nodiscard]] const std::string &message() const { return msg; }
    protected:
        std::string msg;
    };

// Might be thrown during a write operation which fails.
    class ExceptionSparseVirtualFileWrite : public ExceptionSparseVirtualFile {
    public:
        explicit ExceptionSparseVirtualFileWrite(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
    };

// Might be thrown during a write operation where the data differs.
    class ExceptionSparseVirtualFileDiff : public ExceptionSparseVirtualFileWrite {
    public:
        explicit ExceptionSparseVirtualFileDiff(const std::string &in_msg) : ExceptionSparseVirtualFileWrite(in_msg) {}
    };

// Might be thrown during a write operation where the data differs.
    class ExceptionSparseVirtualFileRead : public ExceptionSparseVirtualFile {
    public:
        explicit ExceptionSparseVirtualFileRead(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
    };

    class ExceptionSparseVirtualFileErase : public ExceptionSparseVirtualFile {
    public:
        explicit ExceptionSparseVirtualFileErase(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
    };

    typedef size_t t_fpos;
    typedef std::vector<std::pair<t_fpos, size_t>> t_seek_read;

    typedef struct SparseVirtualFileConfig {
        // TODO: Implement coalesce strategies or abandon that as unnecessary.
        // TODO: Implement cache limit and cache punting strategies? Random would be simplest.
        // TODO: Otherwise need map of fpos -> time or a list of fpos in time order, on access move the fpos to the
        // TODO: beginning then pop end on punt. Maybe this also needs a unordered_map<fpos, index_into_list>?
        // coalesce - The strategy to use for coalescing adjacent blocks. -1 is always, 0 is never and a
        // positive value means coalesce if the size of the result is less than this value.
        int coalesce = -1;
        // If true the memory is destructively overwritten when the Sparse Virtual File is destroyed.
        bool overwrite_on_exit = false;
        // If true compare with existing data on write and if there is a difference throw an exception.
        // This trades performance (if false) for correctness (if true).
        bool compare_for_diff = true;
    } tSparseVirtualFileConfig;

    class SparseVirtualFile {
    public:
        /*
         * Create a Sparse Virtual File
         * id - The unique identifier for this file.
         * mod_time - The modification time of the remote file in UNIX seconds, this is used for integrity checking.
         * config - See tSparseVirtualFileConfig above.
         */
        SparseVirtualFile(const std::string &id, double mod_time,
                const tSparseVirtualFileConfig &config = tSparseVirtualFileConfig()) : \
            m_id(id), \
            m_file_mod_time(mod_time), \
            m_config(config),
            m_time_write(std::chrono::time_point<std::chrono::system_clock>::min()), \
            m_time_read(std::chrono::time_point<std::chrono::system_clock>::min()) {
            if (m_config.coalesce != -1){
                throw std::runtime_error("Coalesce strategy not yet implemented.");
            }
        }
        // ---- Read and write etc. ----
        // Do I have the data?
        bool has(t_fpos fpos, size_t len) const noexcept;
        // Write data at file position.
        void write(t_fpos fpos, const char *data, size_t len);
        // Read data and write to the buffer provided by the caller.
        // Not const as we update m_bytes_read, m_count_read, m_time_read.
        void read(t_fpos fpos, size_t len, char *p);
        // Create a new fragmentation list of seek/read instructions.
        t_seek_read need(t_fpos fpos, size_t len) const noexcept;
        // Implements the data deletion strategy.
        void clear() noexcept;
        // Remove the block at the given file position which must be the start of the block.
        // This will raise an ExceptionSparseVirtualFile if the file position is not the start of the block.
        // Returns the length of the block erased.
        size_t erase(t_fpos fpos);
        // ---- Meta information about the SVF ----
        // The existing blocks as a list of (file_position, size) pairs.
        t_seek_read blocks() const noexcept;
        size_t block_size(t_fpos fpos) const;
        // Information about memory used:
        // size_of() gives best guess of total memory usage.
        size_t size_of() const noexcept;
        // Gives exact number of data bytes held.
        size_t num_bytes() const noexcept { return m_bytes_total; };
        // Gives exact number of blocks used.
        size_t num_blocks() const noexcept { return m_svf.size(); }
        // Uses mutex and checks integrity
        t_fpos last_file_position() const noexcept;
        // Check the clients file modification time has changed.
        // Caller has to decide what to do...
        bool file_mod_time_matches(const double &file_mod_time) const noexcept {
            return file_mod_time == m_file_mod_time;
        }
        // ---- Attribute access ----
        const std::string &id() const noexcept { return m_id; }
        double file_mod_time() const noexcept { return m_file_mod_time; }
        size_t count_write() const noexcept { return m_count_write; }
        size_t count_read() const noexcept { return m_count_read; }
        size_t bytes_write() const noexcept { return m_bytes_write; }
        size_t bytes_read() const noexcept { return m_bytes_read; }
        // These can be cast to std::chrono::time_point<double>
        std::chrono::time_point<std::chrono::system_clock> time_write() const noexcept { return m_time_write; }
        std::chrono::time_point<std::chrono::system_clock> time_read() const noexcept { return m_time_read; }
        // Eliminate copying.
        SparseVirtualFile(const SparseVirtualFile &rhs) = delete;
        SparseVirtualFile operator=(const SparseVirtualFile &rhs) = delete;
#ifdef SVF_THREAD_SAFE
        // Prohibit moving, the mutex has no move constructor.
        SparseVirtualFile(SparseVirtualFile &&other) = delete;
        SparseVirtualFile& operator=(SparseVirtualFile &&rhs) = delete;
#else
        // Allow moving
        SparseVirtualFile(SparseVirtualFile &&other) = default;
        SparseVirtualFile& operator=(SparseVirtualFile &&rhs) = default;
#endif
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
        // The actual SVF
        typedef std::vector<char> t_val;
        typedef std::map<t_fpos, t_val> t_map;
        t_map m_svf;
#ifdef SVF_THREAD_SAFE
        // This adds about 5-10% execution time compared with a single threaded version.
        mutable std::mutex m_mutex;
#endif
    private:
        // Write data at file position without checks.
        void _write(t_fpos fpos, const char *data, size_t len);
        void _write_new_append_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter);
        void _write_append_new_to_old(t_fpos fpos, const char *data, size_t len, t_map::iterator base_iter);
        // NOTE: This is const but read() is not as it updates metadata.
        void _read(t_fpos fpos, size_t len, char *p) const;
        void _throw_diff(t_fpos fpos, const char *data, t_map::const_iterator iter, size_t index_iter) const;
        // Does not use mutex or checks integrity
        t_fpos _last_file_position() const noexcept;
        t_fpos _last_file_pos_for_block(t_map::const_iterator iter) const noexcept;
        /* Check internal integrity. */
        enum ERROR_CONDITION {
            ERROR_NONE = 0,
            ERROR_EMPTY_BLOCK,
            ERROR_ADJACENT_BLOCKS,
            ERROR_BLOCKS_OVERLAP,
            ERROR_BYTE_COUNT_MISMATCH,
        };
        ERROR_CONDITION integrity() const noexcept;
    };

} // namespace SparseVirtualFileSystem

#endif //CPPSVF_SVF_H
