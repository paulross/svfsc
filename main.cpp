#include <iostream>

#include "Python.h"

#include "src/cpp/svf.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

//    PyObject *pobj = PyBytes_FromStringAndSize(NULL, 32);

//    std::cout << "pobj " << pobj << std::endl;

    SparseVirtualFile svf("", 0.0);
    svf.write(8, "ABCD", 4);

    return 0;
}