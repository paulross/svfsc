//
// Created by Paul Ross on 2020-02-13.
//

#ifndef CPPSVF_UTIL_H
#define CPPSVF_UTIL_H

#include <exception>
#include <string>

#include <Python.h>
#include "datetime.h"

//class ExceptionUtil : public std::exception {
//public:
//    explicit ExceptionUtil(const std::string &in_msg) : msg(in_msg) {}
//    const std::string &message() const { return msg; }
//protected:
//    std::string msg;
//};
//
//
//PyObject *
//py_unicode_from_std_string(const std::string &s);
//std::string
//py_unicode_to_std_string(PyObject *op);


#define IMPORT_DATETIME_IF_UNINITIALISED do {   \
    if (! PyDateTimeAPI) {                      \
        PyDateTime_IMPORT;                      \
    }                                           \
    assert(PyDateTimeAPI);                      \
} while(0)

PyObject *
datetime_from_struct_tm(const std::tm *bdt, int usecond);

#endif //CPPSVF_UTIL_H
