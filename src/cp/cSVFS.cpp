#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"

#include "_cSVFS.cpp"
#include "_cSVF.cpp"

// Module initialisation
#pragma mark Module initialisation

static PyModuleDef svfsmodule = {
        PyModuleDef_HEAD_INIT,
        .m_name = "svfs",
        .m_doc = \
        "This module contains Sparse Virtual File System classes."
        "\n"
        "A Sparse Virtual File (SVF) is one where some data from the actual file is held in memory at the specific"
        " file locations as the original file."
        " The original file is identified by a string ID."
        " Data can be written to an SVF, if the data differs from that existing an error will be raised."
        " Data can be read from an SVF, if the SVF does not have the data an error will be raised."
        " Before any read() the SVF can describe what, if any, data is missing and the user can obtain and write that"
        " data to the SVF before reading."
        "\n"
        "A Sparse Virtual File System is an extension of that concept where a file ID (string) is the key to the appropriate SVF.",
        .m_size = -1,
};

PyMODINIT_FUNC
PyInit_svfs(void)
{
    PyObject *m = NULL;

    m = PyModule_Create(&svfsmodule);
    if (m == NULL) {
        return NULL;
    }

    if (PyType_Ready(&svfs_SVF) < 0) {
        return NULL;
    }
    Py_INCREF(&svfs_SVF);
    PyModule_AddObject(m, "SVF", (PyObject *) &svfs_SVF);

    if (PyType_Ready(&svfs_SVFS) < 0) {
        return NULL;
    }
    Py_INCREF(&svfs_SVFS);
    PyModule_AddObject(m, "SVFS", (PyObject *) &svfs_SVFS);

    PyDateTime_IMPORT;
    return m;
}

// END: Module initialisation
#pragma mark END: Module initialisation
