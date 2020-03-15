//
// Created by Paul Ross on 2020-02-13.
//

#include "svfs_util.h"
#include <sstream>

//PyObject *py_unicode_from_std_string(const std::string &s) {
//    return PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, s.c_str(), s.size());
//}
//
//std::string py_unicode_to_std_string(PyObject *op) {
//    assert(op);
//    fprintf(stdout, "py_unicode_to_std_string() op = %p\n", (void *)op);
//    if (! PyUnicode_Check(op)) {
//        std::ostringstream os;
//        os << "py_unicode_to_std_string(): not a unicode object but type \"";
//        os << Py_TYPE(op) << "\"";
//        throw ExceptionUtil(os.str());
//    }
//    assert(PyUnicode_AsUTF8(op));
////    PyObject_Print(PyUnicode_AsUTF8(op), stdout);
//    std::string ret(PyUnicode_AsUTF8(op), static_cast<unsigned long>(PyUnicode_GET_LENGTH(op)));
//    return ret;
//}

