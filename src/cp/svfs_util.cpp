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

#include <sstream>

#include "svfs_util.h"

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

/**
 * Create a Python datetime.datetime from a struct std::tm with additional microseconds.
 *
 * @param bdt C++ time struct.
 * @param usecond Additional microseconds.
 * @return A Python datetime.datetime or NULL on failure in which case a Python Exception will have been set.
 */
PyObject *
datetime_from_struct_tm(const std::tm *bdt, int usecond) {
    PyObject * ret = NULL;

    IMPORT_DATETIME_IF_UNINITIALISED;
    assert(!PyErr_Occurred());
//    // This is calling new_datetime_ex() which increfs tz and sets hastzinfo
//    ret = PyDateTimeAPI->DateTime_FromDateAndTime(
//            bdt->tm_year + 1900,
//            bdt->tm_mon + 1,
//            bdt->tm_mday,
//            bdt->tm_hour,
//            bdt->tm_min,
//            bdt->tm_sec,
//            usecond,
//            NULL,
//            PyDateTimeAPI->DateTimeType
//    );
    ret = PyDateTime_FromDateAndTime(
            bdt->tm_year + 1900,
            bdt->tm_mon + 1,
            bdt->tm_mday,
            bdt->tm_hour,
            bdt->tm_min,
            bdt->tm_sec,
            usecond
    );
    if (!ret) {
        PyErr_Format(PyExc_RuntimeError, "%s: Can not create datetime.datetime", __FUNCTION__);
        goto except;
    }
    assert(!PyErr_Occurred());
    assert(ret);
    goto finally;
    except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
    finally:
    return ret;
}
