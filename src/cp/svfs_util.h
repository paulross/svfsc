/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2020-02-13.
 *
 * @verbatim
    MIT License

    Copyright (c) 2020-2023 Paul Ross

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

#ifndef CPPSVF_UTIL_H
#define CPPSVF_UTIL_H

#include <exception>
#include <string>

#include "cp_svfs.h"

/**
 * Import the datetime capsule if necessary.
 */
#define IMPORT_DATETIME_IF_UNINITIALISED do {   \
    if (! PyDateTimeAPI) {                      \
        PyDateTime_IMPORT;                      \
    }                                           \
    assert(PyDateTimeAPI);                      \
} while(0)

/** If you are interested this is a way that you can trace the input. */
#define TRACE_SELF_ARGS_KWARGS do {                         \
    fprintf(stdout, "self, args, kwargs:\n");               \
    fprintf(stdout, "  self: ");                            \
    PyObject_Print((PyObject *)self, stdout, Py_PRINT_RAW); \
    fprintf(stdout, "\n");                                  \
    fprintf(stdout, "  args: ");                            \
    PyObject_Print(args, stdout, Py_PRINT_RAW);             \
    fprintf(stdout, "\n");                                  \
    fprintf(stdout, "kwargs: ");                            \
    PyObject_Print(kwargs, stdout, Py_PRINT_RAW);           \
    fprintf(stdout, "\n");                                  \
} while(0)

PyObject *
datetime_from_struct_tm(const std::tm *bdt, int usecond);

#endif //CPPSVF_UTIL_H
