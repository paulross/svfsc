//
// Created by Paul Ross on 2020-01-22.
//

#ifndef CPPSVF_TEST_SCF_H
#define CPPSVF_TEST_SCF_H

#include <string>
#include "svf.h"
#include "test.h"


namespace SparseVirtualFileSystem {

    class TestCaseABC {
    public:
        TestCaseABC(const std::string &m_test_name, const t_seek_read &m_writes) : m_test_name(m_test_name),
                                                                                   m_writes(m_writes) {}
        virtual TestResult run() const = 0;
        // Load all the specified write seek/write blocks
        size_t load_writes(SparseVirtualFile &svf, const char *data) const {
            size_t bytes_written = 0;
            for (const auto &write_test: m_writes) {
                assert(write_test.first < 256);
                assert(write_test.first + write_test.second < 256);
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

    TestCount test_all(t_test_results &results);

} // namespace SparseVirtualFileSystem

#endif //CPPSVF_TEST_SCF_H
