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
    protected:
        std::string m_test_name;
        t_seek_read m_writes;
    };


    class TestCaseWrite : public TestCaseABC {
    public:
        TestCaseWrite(const std::string &m_test_name, const t_seek_read &m_writes,
                      const t_seek_read &m_expected_blocks);

        TestResult run() const override;
    private:
        t_seek_read m_expected_blocks;
    };

    size_t test_all(t_test_results &results);

} // namespace SparseVirtualFileSystem

#endif //CPPSVF_TEST_SCF_H
