#include <iostream>

#include "Python.h"

#include "svf.h"
#include "test.h"
#include "test_svf.h"


int main() {
    std::cout << "Testing SVF and SVFS..." << std::endl;

    TestCount pass_fail;
    t_test_results results;

    // Test section
    auto time_start = std::chrono::high_resolution_clock::now();

    pass_fail += SparseVirtualFileSystem::test_svf_all(results);

    std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

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
    return 0;
}
