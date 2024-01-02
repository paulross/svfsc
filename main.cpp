/** @file
 *
 * A Sparse Virtual File System implementation.
 *
 * Created by Paul Ross on 2020-02-10.
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

#include <iostream>

#include "Python.h"

#include "svf.h"
#include "test.h"
#include "test_svf.h"
#include "test_svfs.h"
#include "test_cpp_svfs.h"


/**
 * @brief Main entry point.
 *
 * This runs all the C++ tests.
 * @return 0 on success, non-zero on failure.
 */
int main() {
    std::cout << "Testing SVF and SVFS..." << std::endl;

    SVFS::Test::TestCount pass_fail;
    SVFS::Test::t_test_results results;

    // Test section
    auto time_start = std::chrono::high_resolution_clock::now();

    std::cout << "Testing SVF all..." << std::endl;
    pass_fail += SVFS::Test::test_svf_all(results);
    pass_fail += SVFS::Test::test_cpp_svfs_all(results);
#if 1
    std::cout << "Testing SVFS all..." << std::endl;
    pass_fail += SVFS::Test::test_svfs_all(results);
#endif
    std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
    auto result = SVFS::Test::TestResult(__PRETTY_FUNCTION__, "All tests", results.size() != 166,
                                         "Hard coded test count to make sure some tests haven't been omitted.",
                                         time_exec.count(), 0);
    pass_fail.add_result(result.result());
    results.push_back(result);


    // Results
    write_test_results(results, std::cout);
    std::cout << "Test results [" << pass_fail.total() << "]";
    std::cout << " Pass: " << pass_fail.pass();
    std::cout << " Fail: " << pass_fail.fail();
    if (pass_fail.fail()) {
        std::cout << " - FAILED";
    } else {
        std::cout << " - PASSED";
    }
    std::cout << std::endl;
    std::cout << "main() execution time: " << time_exec.count() << " (s)" << std::endl;

    std::cout << "Bye bye!" << std::endl;
    return pass_fail.fail() != 0;
}
