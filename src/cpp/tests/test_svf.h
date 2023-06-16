/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2020-01-22.
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

#ifndef CPPSVF_TEST_SVF_H
#define CPPSVF_TEST_SVF_H

#include <string>
#include <sstream>

#include "svf.h"
#include "test.h"


namespace SVFS {

    class TestCaseABC {
    public:
        TestCaseABC(const std::string &m_test_name, const t_seek_reads &m_writes) : m_test_name(m_test_name),
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
        t_seek_reads m_writes;
    };


    class TestCaseWrite : public TestCaseABC {
    public:
        TestCaseWrite(const std::string &m_test_name, const t_seek_reads &m_writes,
                      const t_seek_reads &m_expected_blocks);

        TestResult run() const override;
        virtual ~TestCaseWrite() =  default;
    private:
        t_seek_reads m_expected_blocks;
    };

    class TestCaseWriteThrows : public TestCaseABC {
    public:
        TestCaseWriteThrows(const std::string &m_test_name, const t_seek_reads &m_writes,
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
        TestCaseRead(const std::string &m_test_name, const t_seek_reads &m_writes,
                     t_fpos fpos, size_t len);

        TestResult run() const override;

        virtual ~TestCaseRead() = default;

    protected:
        t_fpos m_fpos;
        size_t m_len;
    };

    class TestCaseReadThrows : public TestCaseRead {
    public:
        TestCaseReadThrows(const std::string &m_test_name, const t_seek_reads &m_writes,
                           t_fpos fpos, size_t len, const std::string &message);

        TestResult run() const override;

        virtual ~TestCaseReadThrows() = default;

    protected:
        std::string m_message;
    };

    class TestCaseHas : public TestCaseABC {
    public:
        TestCaseHas(const std::string &m_test_name, const t_seek_reads &m_writes,
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
        TestCaseNeed(const std::string &m_test_name, const t_seek_reads &m_writes,
                     t_fpos fpos, size_t len, const t_seek_reads &m_need);

        TestResult run() const override;

        virtual ~TestCaseNeed() = default;

    protected:
        t_fpos m_fpos;
        size_t m_len;
        t_seek_reads m_need;
    };

    class TestCaseNeedGreedy : public TestCaseABC {
    public:
        explicit TestCaseNeedGreedy(const std::string &m_test_name, const t_seek_reads &m_writes,
                     t_fpos fpos, size_t len, size_t greedy_length, const t_seek_reads &m_need);
        TestResult run() const override;
        virtual ~TestCaseNeedGreedy() = default;
    protected:
        t_fpos m_fpos;
        size_t m_len;
        size_t m_greedy_length;
        t_seek_reads m_need;
    };

    class TestCaseErase : public TestCaseABC {
    public:
        TestCaseErase(const std::string &m_test_name, const t_seek_reads &m_writes, t_fpos fpos);

        TestResult run() const override;

        virtual ~TestCaseErase() = default;

    protected:
        t_fpos m_fpos;
//        size_t m_len;
    };

    class TestCaseEraseThrows : public TestCaseErase {
    public:
        TestCaseEraseThrows(const std::string &m_test_name, const t_seek_reads &m_writes,
                           t_fpos fpos, const std::string &message);

        TestResult run() const override;

        virtual ~TestCaseEraseThrows() = default;

    protected:
        std::string m_message;
    };

    TestCount test_svf_all(t_test_results &results);

} // namespace SVFS

#endif //CPPSVF_TEST_SVF_H
