//
// Created by Paul Ross on 2020-02-13.
//
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
