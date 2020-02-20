//
// Created by Paul Ross on 2020-02-10.
//

#ifndef CPPSVF_SVFS_H
#define CPPSVF_SVFS_H

#include <string>
#include <unordered_map>
#ifdef SVFS_THREAD_SAFE
#include <mutex>
#error "SparseVirtualFileSystem is not (yet) thread safe."
#endif


#include "svf.h"

namespace SVFS {

    // Exception specialisation for the SparseVirtualFile.
    class ExceptionSparseVirtualFileSystem : public std::exception {
    public:
        explicit ExceptionSparseVirtualFileSystem(const std::string &in_msg) : msg(in_msg) {}
        const std::string &message() const { return msg; }
    protected:
        std::string msg;
    };

    class ExceptionSparseVirtualFileSystemOutOfRange : public ExceptionSparseVirtualFileSystem {
    public:
        ExceptionSparseVirtualFileSystemOutOfRange(const std::string &msg) : ExceptionSparseVirtualFileSystem(msg) {}
    };

    class ExceptionSparseVirtualFileSystemInsert : public ExceptionSparseVirtualFileSystem {
    public:
        ExceptionSparseVirtualFileSystemInsert(const std::string &msg) : ExceptionSparseVirtualFileSystem(msg) {}
    };

    class ExceptionSparseVirtualFileSystemRemove : public ExceptionSparseVirtualFileSystem {
    public:
        ExceptionSparseVirtualFileSystemRemove(const std::string &msg) : ExceptionSparseVirtualFileSystem(msg) {}
    };

    class SparseVirtualFileSystem {
    public:
        SparseVirtualFileSystem(int coalesce = -1, bool overwrite = false) : \
            m_coalesce(coalesce), \
            m_overwrite(overwrite) {}

        void insert(const std::string &id, double mod_time);
        void remove(const std::string &id);

        // May raise an ExceptionSparseVirtualFileSystemOutOfRange
        const SparseVirtualFile &at(const std::string &id) const;
        // May raise an ExceptionSparseVirtualFileSystemOutOfRange
        SparseVirtualFile &at(const std::string &id);


        // Has an SVF
        bool has(const std::string &id) const noexcept { return m_svfs.find(id) != m_svfs.end(); }
//        // The SVF has data. This might raise an ExceptionSparseVirtualFileSystemOutOfRange if the id does not exist in
//        // the SVFS.
//        bool has(const std::string &id, t_fpos fpos, size_t len) const noexcept { return at(id).has(fpos, len); }
//        // Write data to the underlying SVF. This might raise an ExceptionSparseVirtualFileSystemOutOfRange if the
//        // id does not exist in the SVFS or anything that the SVF.write() raises such as
//        // ExceptionSparseVirtualFile or children notably ExceptionSparseVirtualFileDiff.
//        void write(const std::string &id, t_fpos fpos, const char *data, size_t len) { return at(id).write(fpos, data, len); }
//        // Read data to the underlying SVF. This might raise an ExceptionSparseVirtualFileSystemOutOfRange if the
//        // id does not exist in the SVFS or anything that the SVF.read() raises such as
//        // ExceptionSparseVirtualFile or children notably ExceptionSparseVirtualFileRead.
//        void read(const std::string &id, t_fpos fpos, size_t len, char *p) { return at(id).read(fpos, len, p); }
//        t_seek_read
//        need(const std::string &id, t_fpos fpos, size_t len) const noexcept { return at(id).need(fpos, len); }

        size_t size() const noexcept { return m_svfs.size(); }
        size_t size_of() const noexcept;
        size_t num_bytes() const noexcept;
        size_t num_blocks() const noexcept;

        std::vector<std::string> keys()const noexcept {
            std::vector<std::string> ret;
            for(const auto &iter: m_svfs) {
                ret.push_back(iter.first);
            }
            return ret;
        }

        // Eliminate copying.
        SparseVirtualFileSystem(const SparseVirtualFileSystem &rhs) = delete;
        SparseVirtualFileSystem operator=(const SparseVirtualFileSystem &rhs) = delete;
        ~SparseVirtualFileSystem() noexcept;
    protected:
        std::unordered_map<std::string, SparseVirtualFile> m_svfs;
        // TODO: Implement the coalesce strategy.
        // -1 Always coalesce
        // 0 Never coalesce
        // >0 Only coalesce if the result is < this value, say 2048 (bytes).
        int m_coalesce;
        bool m_overwrite;
    };
}

#endif //CPPSVF_SVFS_H
