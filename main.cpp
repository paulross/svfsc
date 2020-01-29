#include <iostream>

#include "Python.h"

#include "svf.h"
#include "test.h"
#include "test_svf.h"


TestCount test_write() {
//    SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);
//    svf.write(8, "ABCD", 4);
//    svf.write(8, "ABCD", 4);
    // Count of pass/fail
    t_test_results results;

    auto test_count = SparseVirtualFileSystem::test_all(results);
//    test_count += SparseVirtualFileSystem::test_all(results);

    write_test_results(results, std::cout);
    return test_count;
}


int main() {
    std::cout << "Hello, World!" << std::endl;

//    PyObject *pobj = PyBytes_FromStringAndSize(NULL, 32);

//    std::cout << "pobj " << pobj << std::endl;

    SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);

    TestCount pass_fail;

    pass_fail += test_write();

    std::cout << "Test results [" << pass_fail.total() << "]";
    std::cout << " Pass: " << pass_fail.pass();
    std::cout << " Fail: " << pass_fail.fail();
    if (pass_fail.fail()) {
        std::cout << " - FAILED";
    } else {
        std::cout << " - PASSED";
    }
    std::cout << std::endl;

    return 0;
}