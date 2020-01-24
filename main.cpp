#include <iostream>

#include "Python.h"

#include "svf.h"
#include "test.h"
#include "test_svf.h"


void test_write() {
    SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);
    svf.write(8, "ABCD", 4);
    svf.write(8, "ABCD", 4);
    t_test_results results;
    auto count = SparseVirtualFileSystem::test_all(results);
    write_test_results(results, std::cout);
}


int main() {
    std::cout << "Hello, World!" << std::endl;

//    PyObject *pobj = PyBytes_FromStringAndSize(NULL, 32);

//    std::cout << "pobj " << pobj << std::endl;

    SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);
    test_write();

    return 0;
}