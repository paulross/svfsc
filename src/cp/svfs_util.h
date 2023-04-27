//
// Created by Paul Ross on 2020-02-13.
//

#ifndef CPPSVF_UTIL_H
#define CPPSVF_UTIL_H

#include <exception>
#include <string>

#include "cp_svfs.h"

#define IMPORT_DATETIME_IF_UNINITIALISED do {   \
    if (! PyDateTimeAPI) {                      \
        PyDateTime_IMPORT;                      \
    }                                           \
    assert(PyDateTimeAPI);                      \
} while(0)

PyObject *
datetime_from_struct_tm(const std::tm *bdt, int usecond);

#endif //CPPSVF_UTIL_H
