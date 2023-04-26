//
// Created by Paul Ross on 2020-01-22.
//

#ifndef CPPSVF_TEST_H
#define CPPSVF_TEST_H

#include <iostream>
#include <string>
#include <vector>

extern const char test_data_bytes_512[];

// Used in place of asserts so that checks are done in release version.
class ExceptionTest : public std::exception {
public:
    explicit ExceptionTest(const std::string &in_msg) : msg(in_msg) {}
    [[nodiscard]] const std::string &message() const { return msg; }
protected:
    std::string msg;
};

class ExceptionTestConfiguration : public ExceptionTest {
public:
    explicit ExceptionTestConfiguration(const std::string &in_msg) : ExceptionTest(in_msg) {}
};

class TestResult {
public:
    TestResult(const std::string &m_function, const std::string &m_test,
               int m_result, const std::string &m_errro_message, double m_exec_time,
               size_t m_work_bytes) : m_function(m_function), m_test(m_test),
                                      m_result(m_result),
                                      m_error_message(m_errro_message),
                                      m_exec_time(m_exec_time),
                                      m_work_bytes(m_work_bytes) {}
    const std::string &function() const { return m_function; }
    const std::string &test() const { return m_test; }
    int result() const { return m_result; }
    const std::string &error_message() const { return m_error_message; }
    double exec_time() const { return m_exec_time; }
    size_t work_bytes() const { return m_work_bytes; }
    bool has_error_message() const { return ! m_error_message.empty(); }
    // Returns the work rate in bytes/second.
    double work_rate() const {
        if (m_exec_time != 0.0) {
            return m_work_bytes / m_exec_time;
        }
        return 0.0;
    }
    // Returns the cost in ms/Mb
    double ms_per_mb() const {
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


class TestCount {
public:
    TestCount() : m_pass(0), m_fail(0) {}
    size_t pass() const { return m_pass; }
    size_t fail() const { return m_fail; }
    size_t total() const { return m_pass + m_fail; }
    void add_result(int result) {
        if (result) {
            m_fail += 1;
        } else {
            m_pass += 1;
        }
    }
    TestCount operator+=(const TestCount &rhs) {
        m_pass += rhs.m_pass;
        m_fail += rhs.m_fail;
        return *this;
    }
private:
    size_t m_pass;
    size_t m_fail;
};


#endif //CPPSVF_TEST_H
