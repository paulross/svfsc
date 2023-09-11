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

#ifndef CPPSVF_TEST_H
#define CPPSVF_TEST_H

#include <iostream>
#include <string>
#include <vector>

namespace SVFS {
    namespace Test {

        extern const char test_data_bytes_512[];

/**
 * @brief Exception used in place of asserts so that checks are done in release version.
 */
        class ExceptionTest : public std::exception {
        public:
            explicit ExceptionTest(const std::string &in_msg) : msg(in_msg) {}

            [[nodiscard]] const std::string &message() const { return msg; }

        protected:
            std::string msg;
        };

/**
 * @brief Exception used where a test case is miss-configured.
 */
        class ExceptionTestConfiguration : public ExceptionTest {
        public:
            explicit ExceptionTestConfiguration(const std::string &in_msg) : ExceptionTest(in_msg) {}
        };

/**
 * @brief Result of a test.
 */
        class TestResult {
        public:
            /**
             * Create a test result.
             *
             * @param m_function The test function.
             * @param m_test The tests name.
             * @param m_result The test result, 0 passes non-zero fails.
             * @param m_errro_message Any error message.
             * @param m_exec_time The relevant execution time (non necessarily the complete tests time).
             * @param m_work_bytes Size of the work done.
             */
            TestResult(const std::string &m_function, const std::string &m_test,
                       int m_result, const std::string &m_errro_message, double m_exec_time,
                       size_t m_work_bytes) : m_function(m_function), m_test(m_test),
                                              m_result(m_result),
                                              m_error_message(m_errro_message),
                                              m_exec_time(m_exec_time),
                                              m_work_bytes(m_work_bytes) {}

            [[nodiscard]] const std::string &function() const { return m_function; }

            [[nodiscard]] const std::string &test() const { return m_test; }

            [[nodiscard]] int result() const { return m_result; }

            [[nodiscard]] const std::string &error_message() const { return m_error_message; }

            [[nodiscard]] double exec_time() const { return m_exec_time; }

            [[nodiscard]] size_t work_bytes() const { return m_work_bytes; }

            [[nodiscard]] bool has_error_message() const { return !m_error_message.empty(); }

            /// Returns the work rate in bytes/second.
            [[nodiscard]] double work_rate() const {
                if (m_exec_time != 0.0) {
                    return m_work_bytes / m_exec_time;
                }
                return 0.0;
            }

            /// Returns the cost in ms/Mb
            [[nodiscard]] double ms_per_mb() const {
                if (m_work_bytes != 0.0) {
                    return m_exec_time * 1000 * (1 << 20) / m_work_bytes;
                }
                return 0.0;
            }

        private:
            std::string m_function;
            std::string m_test;
            int m_result;
            std::string m_error_message;
            double m_exec_time;
            size_t m_work_bytes;
        };


        typedef std::vector<TestResult> t_test_results;


        void write_test_results(const t_test_results &results, std::ostream &os);


/**
 * @brief Count of tests taht pass and fail.
 */
        class TestCount {
        public:
            TestCount() : m_pass(0), m_fail(0) {}

            [[nodiscard]] size_t pass() const { return m_pass; }

            [[nodiscard]] size_t fail() const { return m_fail; }

            [[nodiscard]] size_t total() const { return m_pass + m_fail; }

            /// Add a tests result.
            void add_result(int result) {
                if (result) {
                    m_fail += 1;
                } else {
                    m_pass += 1;
                }
            }

            /// Append a test result.
            TestCount operator+=(const TestCount &rhs) {
                m_pass += rhs.m_pass;
                m_fail += rhs.m_fail;
                return *this;
            }

        private:
            size_t m_pass;
            size_t m_fail;
        };

    } // namespace Test
} // namespace SVFS

#endif //CPPSVF_TEST_H
