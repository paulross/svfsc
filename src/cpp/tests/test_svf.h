//
// Created by Paul Ross on 2020-01-22.
//

#ifndef CPPSVF_TEST_SVF_H
#define CPPSVF_TEST_SVF_H

#include <string>
#include <sstream>

#include "svf.h"
#include "test.h"


namespace SVFS {

    class TestCaseABC {
    public:
        TestCaseABC(const std::string &m_test_name, const t_seek_read &m_writes) : m_test_name(m_test_name),
                                                                                   m_writes(m_writes) {}
        virtual TestResult run() const = 0;
        // Load all the specified write seek/write blocks
        size_t load_writes(SparseVirtualFile &svf, const char *data) const {
            size_t bytes_written = 0;
            for (const auto &write_test: m_writes) {
                /* We limit ourselves to only half the 512 byte data and then a further offset of 256 maximum. */
                assert(write_test.first < 256);
                if (write_test.first >= 256) {
                    std::ostringstream os;
                    os << "Test file position " << write_test.first;
                    os << " >= 256";
                    throw ExceptionTestConfiguration(os.str());
                }
                assert(write_test.first + write_test.second < 256);
                if (write_test.first + write_test.second >= 256) {
                    std::ostringstream os;
                    os << "Test file position + size " << write_test.first;
                    os << " >= 256";
                    throw ExceptionTestConfiguration(os.str());
                }
                svf.write(write_test.first, data + write_test.first, write_test.second);
                bytes_written += write_test.second;
            }
            return bytes_written;
        }
        virtual ~TestCaseABC() = default;
        const std::string & test_name() const noexcept { return m_test_name; }
    protected:
        std::string m_test_name;
        t_seek_read m_writes;
    };


    class TestCaseWrite : public TestCaseABC {
    public:
        TestCaseWrite(const std::string &m_test_name, const t_seek_read &m_writes,
                      const t_seek_read &m_expected_blocks);

        TestResult run() const override;
        virtual ~TestCaseWrite() =  default;
    private:
        t_seek_read m_expected_blocks;
    };

    class TestCaseWriteThrows : public TestCaseABC {
    public:
        TestCaseWriteThrows(const std::string &m_test_name, const t_seek_read &m_writes,
                            t_fpos fpos, size_t len, const char *data, const std::string &message);

        TestResult run() const override;

        virtual ~TestCaseWriteThrows() = default;

    protected:
        t_fpos m_fpos;
        const char *m_data;
        size_t m_len;
        std::string m_message;
    };

    class TestCaseRead : public TestCaseABC {
    public:
        TestCaseRead(const std::string &m_test_name, const t_seek_read &m_writes,
                     t_fpos fpos, size_t len);

        TestResult run() const override;

        virtual ~TestCaseRead() = default;

    protected:
        t_fpos m_fpos;
        size_t m_len;
    };

    class TestCaseReadThrows : public TestCaseRead {
    public:
        TestCaseReadThrows(const std::string &m_test_name, const t_seek_read &m_writes,
                           t_fpos fpos, size_t len, const std::string &message);

        TestResult run() const override;

        virtual ~TestCaseReadThrows() = default;

    protected:
        std::string m_message;
    };

    class TestCaseHas : public TestCaseABC {
    public:
        TestCaseHas(const std::string &m_test_name, const t_seek_read &m_writes,
                     t_fpos fpos, size_t len, bool expected);

        TestResult run() const override;

        virtual ~TestCaseHas() = default;

    protected:
        t_fpos m_fpos;
        size_t m_len;
        bool m_expected;
    };

    class TestCaseNeed : public TestCaseABC {
    public:
        TestCaseNeed(const std::string &m_test_name, const t_seek_read &m_writes,
                     t_fpos fpos, size_t len, const t_seek_read &m_need);

        TestResult run() const override;

        virtual ~TestCaseNeed() = default;

    protected:
        t_fpos m_fpos;
        size_t m_len;
        t_seek_read m_need;
    };

    class TestCaseErase : public TestCaseABC {
    public:
        TestCaseErase(const std::string &m_test_name, const t_seek_read &m_writes, t_fpos fpos);

        TestResult run() const override;

        virtual ~TestCaseErase() = default;

    protected:
        t_fpos m_fpos;
//        size_t m_len;
    };

    class TestCaseEraseThrows : public TestCaseErase {
    public:
        TestCaseEraseThrows(const std::string &m_test_name, const t_seek_read &m_writes,
                           t_fpos fpos, const std::string &message);

        TestResult run() const override;

        virtual ~TestCaseEraseThrows() = default;

    protected:
        std::string m_message;
    };

    TestCount test_svf_all(t_test_results &results);

} // namespace SparseVirtualFileSystem

#endif //CPPSVF_TEST_SVF_H
