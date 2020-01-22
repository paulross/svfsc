//
// Created by Paul Ross on 2020-01-14.
//

#ifndef CPPSVF_SVF_H
#define CPPSVF_SVF_H


#include <string>
#include <vector>
#include <list>
#include <map>
#include <chrono>
#include <cassert>

#ifdef DEBUG
#define SVF_ASSERT(x) assert(x)
#else
#define SVF_ASSERT(x)
#endif

namespace SparseVirtualFileSystem {

// Exception specialisation for the SparseVirtualFile.
    class ExceptionSparseVirtualFile : public std::exception {
    public:
        explicit ExceptionSparseVirtualFile(const std::string &in_msg) : msg(in_msg) {}

        const std::string &message() const { return msg; }

    protected:
        std::string msg;
    };

// Might be thrown during a write operation where the data differs.
    class ExceptionSparseVirtualFileDiff : public ExceptionSparseVirtualFile {
    public:
        explicit ExceptionSparseVirtualFileDiff(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
    };

// Might be thrown during a write operation which fails.
    class ExceptionSparseVirtualFileWrite : public ExceptionSparseVirtualFile {
    public:
        explicit ExceptionSparseVirtualFileWrite(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
    };

// Might be thrown during a write operation where the data differs.
    class ExceptionSparseVirtualFileRead : public ExceptionSparseVirtualFile {
    public:
        explicit ExceptionSparseVirtualFileRead(const std::string &in_msg) : ExceptionSparseVirtualFile(in_msg) {}
    };

//typedef fpos_t t_fpos;
    typedef size_t t_fpos;

    class SparseVirtualFile {
    public:
        // TODO: Implement coalesce strategies.
        // TODO: Implement cache limit and cache punting strategies.
        SparseVirtualFile(const std::string &id, double mod_time, int coalesce = -1) : \
        m_id(id), \
        m_file_mod_time(mod_time), \
        m_coalesce(coalesce), \
        m_time_read(std::chrono::time_point<std::chrono::system_clock>::min()), \
        m_time_write(std::chrono::time_point<std::chrono::system_clock>::min()) {}

        // Check the clients file modification time has changed.
        // Caller has to decide what to do...
        bool file_mod_time_matches(const double &file_mod_time) const noexcept {
            return file_mod_time == m_file_mod_time;
        }

        // Do I have the data?
        bool has(t_fpos fpos, size_t len) const noexcept;

        // Write data at file position.
        void write(t_fpos, const char *data, size_t len);

        // Read data and write to the buffer provided by the caller.
        // Not const as we update m_bytes_read, m_count_read, m_time_read.
        void read(t_fpos fpos, size_t len, char *p);

        // Create a new fragmentation list of seek/read instructions.
        std::list<std::pair<t_fpos, size_t>> need(t_fpos fpos, size_t len) const noexcept;

        // Information about memory used:
        // size_of() gives best guess of total memory usage.
        size_t size_of() const noexcept;

        // Gives exact number of data bytes held.
        size_t num_bytes() const noexcept { return m_bytes_total; };

        // Gives exact number of blocks used.
        size_t num_blocks() const noexcept { return m_svf.size(); }

        t_fpos last_file_position() const noexcept;

        // Attribute access
        const std::string id() const noexcept { return m_id; }

        void clear() noexcept;

        // Eliminate copying.
        SparseVirtualFile(const SparseVirtualFile &rhs) = delete;

        SparseVirtualFile operator=(const SparseVirtualFile &rhs) = delete;

    private:
        std::string m_id;
        double m_file_mod_time;
        // TODO: Implement this strategy.
        // -1 Always coalesce
        // 0 Never coalesce
        // >0 Only coalesce if the result is < this value, say 2048 (bytes).
        int m_coalesce;
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
    private:
        // Write data at file position without checks.
        void _write(t_fpos fpos, const char *data, size_t len);

        void _write_new_append_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter);

        void _write_append_new_to_old(t_fpos fpos, const char *data, size_t len, t_map::iterator iter);

        void _throw_diff(t_fpos fpos, const char *data, t_map::const_iterator iter, size_t index_iter) const;

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
