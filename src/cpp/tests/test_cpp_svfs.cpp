/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 14/06/2023.
 *
 * @verbatim
    MIT License

    Copyright (c) 2020-2024 Paul Ross

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

#include "test_svf.h"
#include "cpp_svfs.h"

namespace SVFS {
    namespace Test {

        TestCount test_cpp_version(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;

            auto time_start = std::chrono::high_resolution_clock::now();
            std::string version = SVFS_CPP_VERSION;
            result |= version != "0.2.1";
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(), 1);
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_cpp_version_major(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;

            auto time_start = std::chrono::high_resolution_clock::now();
            result |= SVFS_CPP_VERSION_MAJOR != 0;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(), 1);
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_cpp_version_minor(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;

            auto time_start = std::chrono::high_resolution_clock::now();
            result |= SVFS_CPP_VERSION_MINOR != 2;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(), 1);
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_cpp_version_patch(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;

            auto time_start = std::chrono::high_resolution_clock::now();
            result |= SVFS_CPP_VERSION_PATCH != 2;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(), 1);
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_cpp_version_suffix(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;

            auto time_start = std::chrono::high_resolution_clock::now();
            result |= std::string(SVFS_CPP_VERSION_SUFFIX) != "";
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(), 1);
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_cpp_svfs_all(t_test_results &results) {
            TestCount count;
            count += test_cpp_version(results);
            count += test_cpp_version_major(results);
            count += test_cpp_version_minor(results);
            count += test_cpp_version_patch(results);
            count += test_cpp_version_suffix(results);
            return count;
        }

    } // namespace Test
} // namespace SVFS
