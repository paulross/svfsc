#include <iostream>

#include "Python.h"

#include "src/cpp/svf.h"


void test_write() {
    SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);
    svf.write(8, "ABCD", 4);
    svf.write(8, "ABCD", 4);
}


int main() {
    std::cout << "Hello, World!" << std::endl;

//    PyObject *pobj = PyBytes_FromStringAndSize(NULL, 32);

//    std::cout << "pobj " << pobj << std::endl;

    SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);
    test_write();

    return 0;
}