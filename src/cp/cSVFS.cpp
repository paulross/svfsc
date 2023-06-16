/** @file
 *
 * A Sparse Virtual File implementation.
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

#include "cpp_svfs.h"

#include "_cSVFS.cpp"
#include "_cSVF.cpp"

// Module initialisation
#pragma mark Module initialisation

static PyModuleDef svfsmodule = {
        PyModuleDef_HEAD_INIT,
        .m_name = "svfsc",
        .m_doc = \
        "This module contains Sparse Virtual File System classes."
        "\n"
        "A Sparse Virtual File (SVF) is one where data from the actual file is held in memory at the relevant"
        " file locations as in the original file."
        " The original file is identified by a string ID."
        " Data can be written to an SVF, if the data differs from that existing an error might be raised."
        " Data can be read from an SVF, if the SVF does not have the data an error will be raised."
        " Before any ``read()`` the SVF can describe what, if any, data is missing and the user can obtain and write that"
        " data to the SVF before reading."
        "\n\n"
        "A Sparse Virtual File System (SVFS) is an extension of that concept where a file ID (string) is the key to the"
        " appropriate SVF.",
        .m_size = -1,
};

PyMODINIT_FUNC
PyInit_svfsc(void)
{
    PyObject *m = NULL;

    m = PyModule_Create(&svfsmodule);
    if (m == NULL) {
        return NULL;
    }
    if (PyModule_AddStringConstant(m, "SVFS_CPP_VERSION", SVFS_CPP_VERSION)) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "SVFS_CPP_VERSION_MAJOR", SVFS_CPP_VERSION_MAJOR)) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "SVFS_CPP_VERSION_MINOR", SVFS_CPP_VERSION_MINOR)) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "SVFS_CPP_VERSION_PATCH", SVFS_CPP_VERSION_PATCH)) {
        return NULL;
    }
    if (PyModule_AddStringConstant(m, "SVFS_CPP_VERSION_SUFFIX", SVFS_CPP_VERSION_SUFFIX)) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "SVF_THREAD_SAFE",
#ifdef SVF_THREAD_SAFE
    SVF_THREAD_SAFE
#else
    0
#endif
                                )) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "SVFS_THREAD_SAFE",
#ifdef SVFS_THREAD_SAFE
    SVFS_THREAD_SAFE
#else
    0
#endif
                                )) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "PY_THREAD_SAFE",
#ifdef PY_THREAD_SAFE
    1
#else
    0
#endif
                                )) {
        return NULL;
    }

    if (PyType_Ready(&svfsc_cSVF) < 0) {
        return NULL;
    }
    Py_INCREF(&svfsc_cSVF);
    PyModule_AddObject(m, "cSVF", (PyObject *) &svfsc_cSVF);

    if (PyType_Ready(&svfsc_cSVFS) < 0) {
        return NULL;
    }
    Py_INCREF(&svfsc_cSVF);
    PyModule_AddObject(m, "cSVFS", (PyObject *) &svfsc_cSVFS);


    PyDateTime_IMPORT;
    return m;
}

// END: Module initialisation
#pragma mark END: Module initialisation
