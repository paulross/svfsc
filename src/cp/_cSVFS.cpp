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
 *
 * Naming convention:
 *
 * SVFS functions are named cp_SparseVirtualFileSystem_...
 * SVF functions are named cp_SparseVirtualFileSystem_svf_...
 */

#include "cp_svfs.h"

#include <ctime>
#include <memory>

#include "svfs.h"
#include "svfs_util.h"

/* TODO: Implement pickling an SVFS?
 * TODO: Implement the Buffer Protocol rather than returning a copy of the bytes? Look for PyBytes_FromStringAndSize().
 * */

/**
 * @brief Python wrapper around a C++ SparseVirtualFile.
 *
 * If \c PY_THREAD_SAFE is defined then this also contains a mutex.
 */
typedef struct {
    PyObject_HEAD
    SVFS::SparseVirtualFileSystem *p_svfs;
#ifdef PY_THREAD_SAFE
    PyThread_type_lock lock;
#endif
} cp_SparseVirtualFileSystem;

#ifdef PY_THREAD_SAFE

/** @brief A RAII wrapper around the PyThread_type_lock for the CPython SVFS.
 *
 * See https://pythonextensionpatterns.readthedocs.io/en/latest/thread_safety.html
 * */
class AcquireLockSVFS {
public:
    AcquireLockSVFS(cp_SparseVirtualFileSystem *pSVFS) : _pSVFS(pSVFS) {
        assert(_pSVFS);
        assert(_pSVFS->lock);
        if (!PyThread_acquire_lock(_pSVFS->lock, NOWAIT_LOCK)) {
            Py_BEGIN_ALLOW_THREADS
                PyThread_acquire_lock(_pSVFS->lock, WAIT_LOCK);
            Py_END_ALLOW_THREADS
        }
    }

    ~AcquireLockSVFS() {
        assert(_pSVFS);
        assert(_pSVFS->lock);
        PyThread_release_lock(_pSVFS->lock);
    }

private:
    cp_SparseVirtualFileSystem *_pSVFS;
};

#else
/* Make the class a NOP which should get optimised out. */
class AcquireLockSVFS {
public:
    AcquireLockSVFS(cp_SparseVirtualFileSystem *) {}
};
#endif

// Function entry point test macro.
// After construction, we expect this invariant at the entry to each function.
// The cast is necessary when used with functions that take a SVFS as a PyObject* such as
// cp_SparseVirtualFileSystem_mapping_length
#define ASSERT_FUNCTION_ENTRY_SVFS(member) do { \
    assert(self); \
    assert(((cp_SparseVirtualFileSystem *)self)->member); \
    assert(! PyErr_Occurred()); \
} while (0)


// Construction and destruction
#pragma mark Construction and destruction

static PyObject *
cp_SparseVirtualFileSystem_new(PyTypeObject *type, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds)) {
    assert(!PyErr_Occurred());
    cp_SparseVirtualFileSystem *self;
    self = (cp_SparseVirtualFileSystem *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->p_svfs = nullptr;
#ifdef PY_THREAD_SAFE
        self->lock = NULL;
#endif
    }
//    PyObject_Print((PyObject *)self, stdout);
//    fprintf(stdout, "cp_SparseVirtualFileSystem_new() self %p\n", (void *)self);
    assert(!PyErr_Occurred());
    return (PyObject *) self;
}

static int
cp_SparseVirtualFileSystem_init(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    assert(!PyErr_Occurred());
    static const char *kwlist[] = {"overwrite_on_exit", "compare_for_diff", NULL};
    SVFS::tSparseVirtualFileConfig config;

//    TRACE_SELF_ARGS_KWARGS;
//    fprintf(stdout, "Config was compare_for_diff=%d overwrite_on_exit=%d\n", config.compare_for_diff,
//            config.overwrite_on_exit);

    // NOTE: With format unit 'p' we need to pass in an int.
    int overwrite_on_exit = config.overwrite_on_exit ? 1 : 0;
    int compare_for_diff = config.compare_for_diff ? 1 : 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|pp", (char **) kwlist, &overwrite_on_exit, &compare_for_diff)) {
        assert(PyErr_Occurred());
        return -1;
    }
    config.overwrite_on_exit = overwrite_on_exit != 0;
    config.compare_for_diff = compare_for_diff != 0;

//    fprintf(stdout, "Config now compare_for_diff=%d overwrite_on_exit=%d\n", config.compare_for_diff,
//            config.overwrite_on_exit);

    self->p_svfs = new SVFS::SparseVirtualFileSystem(config);
#ifdef PY_THREAD_SAFE
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        delete self->p_svfs;
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate thread lock.");
        return -2;
    }
#endif
//    fprintf(stdout, "cp_SparseVirtualFileSystem_init() self->p_svfs %p\n", (void *)self->p_svfs);
    assert(!PyErr_Occurred());
    return 0;
}

static void
cp_SparseVirtualFileSystem_dealloc(cp_SparseVirtualFileSystem *self) {
#ifdef PY_THREAD_SAFE
    if (self->lock) {
        PyThread_free_lock(self->lock);
        self->lock = NULL;
    }
#endif
    delete self->p_svfs;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

// END: Construction and destruction
#pragma mark END: Construction and destruction

// SVFS functions
#pragma mark SVFS functions

static const char *cp_SparseVirtualFileSystem_keys_docstring = (
        "Returns the IDs of all the Sparse Virtual Files in the Sparse Virtual File System."
        "\n\nSignature: ``keys() -> typing.List[str]:``"
);

static PyObject *
cp_SparseVirtualFileSystem_keys(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    Py_ssize_t index = 0;
    PyObject * key = NULL;
    // All items set to NULL on construction.
    PyObject * ret = PyList_New(self->p_svfs->size());
    AcquireLockSVFS _lock(self);

    if (!ret) {
        PyErr_Format(PyExc_RuntimeError, "%s: Can create list of size %d", __FUNCTION__, self->p_svfs->size());
        goto except;
    }
    try {
        for (const auto &cpp_key: self->p_svfs->keys()) {
            key = PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, cpp_key.c_str(), cpp_key.size());
            if (!key) {
                PyErr_Format(PyExc_RuntimeError, "%s: Can create key for %s", __FUNCTION__, cpp_key.c_str());
                goto except;
            }
            // No error checking for this line. Steals reference.
            PyList_SET_ITEM(ret, index, key);
            key = NULL; // Safety habit with stolen reference.
            ++index;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        goto except;
    }
    assert(!PyErr_Occurred());
    assert(ret);
    goto finally;
    except:
    assert(PyErr_Occurred());
    if (ret) {
        for (Py_ssize_t i = 0; i < PyList_Size(ret); ++i) {
            Py_XDECREF(PyList_GET_ITEM(ret, i));
        }
    }
    Py_XDECREF(ret);
    ret = NULL;
    finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_insert_docstring = (
        "Inserts a Sparse Virtual File of ID and Unix file modification time as a float."
        "\n\nSignature: ``insert(id: str) -> None:``"
);

static PyObject *
cp_SparseVirtualFileSystem_insert(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    double mod_time;
    static const char *kwlist[] = {"id", "mod_time", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sd", (char **) kwlist, &c_id, &mod_time)) {
        goto except;
    }
    try {
        self->p_svfs->insert(c_id, mod_time);
    } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileSystemInsert &err) {
        PyErr_Format(
                PyExc_RuntimeError, "%s: Can not insert a new Sparse Virtual File ID = \"%s\". ERROR: %s",
                __FUNCTION__, c_id, err.message().c_str()
        );
        goto except;
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        goto except;
    }
    Py_INCREF(Py_None);
    ret = Py_None;
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

static const char *cp_SparseVirtualFileSystem_remove_docstring = (
        "Removes a Sparse Virtual File of ID freeing that file's memory. Will raise an ``IndexError`` if the ID is absent."
        "\n\nSignature: ``remove(id: str) -> None:``"
);

static PyObject *
cp_SparseVirtualFileSystem_remove(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    static const char *kwlist[] = {"id", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **) kwlist, &c_id)) {
        goto except;
    }
    try {
        self->p_svfs->remove(c_id);
    } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileSystemRemove &err) {
        PyErr_Format(PyExc_IndexError, "%s: Can not remove a Sparse Virtual File. ERROR: %s",
                     __FUNCTION__, err.message().c_str()
        );
        goto except;
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        goto except;
    }
    Py_INCREF(Py_None);
    ret = Py_None;
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

static const char *cp_SparseVirtualFileSystem_has_docstring = (
        "Returns True if the Sparse Virtual File for the ID is in the Sparse Virtual File System."
        "\n\nSignature: ``has(id: str) -> bool:``"
);

static PyObject *
cp_SparseVirtualFileSystem_has(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    static const char *kwlist[] = {"id", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **) kwlist, &c_id)) {
        goto except;
    }
    try {
        if (self->p_svfs->has(c_id)) {
            Py_INCREF(Py_True);
            ret = Py_True;
        } else {
            Py_INCREF(Py_False);
            ret = Py_False;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
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

static const char *cp_SparseVirtualFileSystem_total_size_of_docstring = (
        "Returns the estimate of total memory usage of the Sparse Virtual File System."
        "\n\nSignature: ``total_size_of() -> int:``"
);

static PyObject *
cp_SparseVirtualFileSystem_total_size_of(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    try {
        return PyLong_FromLong(self->p_svfs->size_of());
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return NULL;
    }
}

static const char *cp_SparseVirtualFileSystem_total_bytes_docstring = (
        "Returns the total number of file bytes held by the Sparse Virtual File System."
        "\n\nSignature: ``total_bytes() -> int:``"
);

static PyObject *
cp_SparseVirtualFileSystem_total_bytes(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    try {
        return PyLong_FromLong(self->p_svfs->num_bytes());
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return NULL;
    }
}

static const char *cp_SparseVirtualFileSystem_total_blocks_docstring = (
        "Returns the total number of blocks of data held by the Sparse Virtual File System."
        "\n\nSignature: ``total_blocks() -> int:``"
);

static PyObject *
cp_SparseVirtualFileSystem_total_blocks(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    try {
        return PyLong_FromLong(self->p_svfs->num_blocks());
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return NULL;
    }
}

static const char *cp_SparseVirtualFileSystem_svf_has_data_docstring = (
        "Checks if the Sparse Virtual File of the ID has data at the given file_position and length."
        " This takes a string as an id, a file position and a length."
        " This returns True if the Sparse Virtual File of that id has the data, False otherwise."
        " This will raise an ``IndexError`` if the SVF of that id does not exist."
        "\n\nSignature: ``has_data(id: str, file_position: int, length: int) -> bool:``"
);

static PyObject *
cp_SparseVirtualFileSystem_svf_has_data(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    std::string cpp_id;
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = {"id", "file_position", "length", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sKK", (char **) kwlist, &c_id, &fpos, &len)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    if (self->p_svfs->has(cpp_id)) {
        try {
            const SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            if (svf.has(fpos, len)) {
                Py_INCREF(Py_True);
                ret = Py_True;
            } else {
                Py_INCREF(Py_False);
                ret = Py_False;
            }
        } catch (const std::exception &err) {
            PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
            goto except;
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id);
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

static const char *cp_SparseVirtualFileSystem_svf_write_docstring = (
        "Writes the data to the Sparse Virtual File of the given ID at file_position and length."
        " This takes a string as an id, a file position and data as a bytes object."
        " This will raise an ``IndexError`` if the SVF of that id does not exist."
        " This will raise an ``IOError`` if the given data is different than that seen before and only"
        " new data up to this point will be written."
        " This will raise a ``RuntimeError`` if the data can not be written for any other reason"
        "\n\nSignature: ``write(id: str, file_position: int, data: bytes) -> None:``"
);

static PyObject *
cp_SparseVirtualFileSystem_svf_write(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    std::string cpp_id;
    unsigned long long fpos = 0;
    PyObject * py_bytes_data = NULL;
    static const char *kwlist[] = {"id", "file_position", "data", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sKS", (char **) kwlist, &c_id, &fpos, &py_bytes_data)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            try {
                svf.write(fpos, PyBytes_AS_STRING(py_bytes_data), PyBytes_Size(py_bytes_data));
            } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileDiff &err) {
                PyErr_Format(PyExc_IOError,
                             "%s: Can not write to a SVF id = \"%s\" as the given data is different from what is there. ERROR: %s",
                             __FUNCTION__, c_id, err.message().c_str());
                goto except;
            } catch (const SVFS::Exceptions::ExceptionSparseVirtualFile &err) {
                PyErr_Format(PyExc_RuntimeError, "%s: Can not write to a SVF id = \"%s\". ERROR: %s",
                             __FUNCTION__, c_id, err.message().c_str());
                goto except;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        goto except;
    }
    Py_INCREF(Py_None);
    ret = Py_None;
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

static const char *cp_SparseVirtualFileSystem_svf_read_docstring = (
        "Read the data to the Sparse Virtual File at file_position and length returning a bytes object."
        " This takes a string as an id, a file position and a length."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        " This will raise an ``IOError`` if any data is not present"
        " This will raise a ``RuntimeError`` if the data can not be read for any other reason"
        "\n\nSignature: ``read(id: str, file_position: int, length: int) -> bytes:``"
);

static PyObject *
cp_SparseVirtualFileSystem_svf_read(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    std::string cpp_id;
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = {"id", "file_position", "length", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sKK", (char **) kwlist, &c_id, &fpos, &len)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            // Create a bytes object
            ret = PyBytes_FromStringAndSize(NULL, len);
            try {
                svf.read(fpos, len, PyBytes_AS_STRING(ret));
            } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileRead &err) {
                PyErr_Format(PyExc_IOError, "%s: Can not read from a SVF id= \"%s\". ERROR: %s",
                             __FUNCTION__, c_id, err.message().c_str());
                goto except;
            } catch (const SVFS::Exceptions::ExceptionSparseVirtualFile &err) {
                PyErr_Format(PyExc_RuntimeError, "%s: Fatal error reading from a SVF id= \"%s\". ERROR: %s",
                             __FUNCTION__, c_id, err.message().c_str());
                goto except;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
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

static const char *cp_SparseVirtualFileSystem_svf_erase_docstring = (
        "Erases the data block in the Sparse Virtual File at a file position."
        " This takes a string as an id and a file_position."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        " This will raise an ``IOError`` if there is not a block at the position."
        " This will raise a ``RuntimeError`` if the data can not be read for any other reason"
        "\n\nSignature: ``erase(id: str, file_position: int) -> None:``"
);

static PyObject *
cp_SparseVirtualFileSystem_svf_erase(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    std::string cpp_id;
    unsigned long long fpos = 0;
    static const char *kwlist[] = {"id", "file_position", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sK", (char **) kwlist, &c_id, &fpos)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            try {
                svf.erase(fpos);
            } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileErase &err) {
                PyErr_Format(PyExc_IOError, "%s: Can not erase block from a SVF id= \"%s\". ERROR: %s",
                             __FUNCTION__, c_id, err.message().c_str());
                goto except;
            } catch (const SVFS::Exceptions::ExceptionSparseVirtualFile &err) {
                PyErr_Format(PyExc_RuntimeError, "%s: Fatal error erasing from a SVF id= \"%s\". ERROR: %s",
                             __FUNCTION__, c_id, err.message().c_str());
                goto except;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        goto except;
    }
    Py_INCREF(Py_None);
    ret = Py_None;
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

static const char *cp_SparseVirtualFileSystem_svf_need_docstring = (
        "Given a file_position and length this returns a ordered list ``[(file_position, length), ...]`` of seek/read"
        " instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will succeed."
        " If greedy_length is > 0 then, if possible, blocks will be coalesced to reduce the size of the return value."
        "\nUsage::\n\n"
        "    if not svfs.has(identity, file_position, length):\n"
        "        for fpos, read_len in svfs.need(identity, file_position, length):\n"
        "            # Somehow get the data at that seek/read position...\n"
        "            svfs.write(identity, fpos, data)\n"
        "    return svfs.read(identity, file_position, length):\n"
        "\n\nSignature:\n\n``need(id: str, file_position: int, length: int, greedy_length: int = 0) -> typing.Tuple[typing.Tuple[int, int], ...]:``"
);

static PyObject *
cp_SparseVirtualFileSystem_svf_need(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL; // PyListObject
    char *c_id = NULL; // PyUnicodeObject
    std::string cpp_id;
    PyObject * list_item = NULL; // PyTupleObject
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    unsigned long long greedy_length = 0;
    static const char *kwlist[] = {"id", "file_position", "length", "greedy_length", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sKK|K", (char **) kwlist, &c_id, &fpos, &len, &greedy_length)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            const SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            SVFS::t_seek_reads seek_read = svf.need(fpos, len, greedy_length);
            ret = PyList_New(seek_read.size());
            for (size_t i = 0; i < seek_read.size(); ++i) {
                list_item = Py_BuildValue("KK", seek_read[i].first, seek_read[i].second);
                if (!list_item) {
                    PyErr_Format(PyExc_MemoryError, "%s: Can not create tuple", __FUNCTION__);
                    goto except;
                }
                PyList_SET_ITEM(ret, i, list_item);
                list_item = NULL;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        goto except;
    }
    assert(!PyErr_Occurred());
    assert(ret);
    goto finally;
    except:
    assert(PyErr_Occurred());
    if (ret) {
        for (Py_ssize_t i = 0; i < PyList_Size(ret); ++i) {
            Py_XDECREF(PyList_GET_ITEM(ret, i));
        }
    }
    Py_XDECREF(ret);
    ret = NULL;
    finally:
    return ret;
}

// This macro is for functions that return a size_t type such as count_write, count_read, bytes_write, bytes_read.
#define SVFS_SVFS_METHOD_SIZE_T_WRAPPER(method_name) static PyObject * \
cp_SparseVirtualFileSystem_svf_##method_name(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) { \
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs); \
    PyObject *ret = NULL; \
    char *c_id = NULL; \
    std::string cpp_id; \
    AcquireLockSVFS _lock(self); \
    static const char *kwlist[] = { "id", NULL}; \
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **)kwlist, &c_id)) { \
        goto except; \
    } \
    cpp_id = std::string(c_id); \
    try { \
        if (self->p_svfs->has(cpp_id)) { \
            const SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id); \
            ret = PyLong_FromLong(svf.method_name()); \
        } else { \
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id); \
            goto except; \
        } \
    } catch (const std::exception &err) { \
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what()); \
        goto except; \
    } \
    assert(! PyErr_Occurred()); \
    assert(ret); \
    goto finally; \
except: \
    assert(PyErr_Occurred()); \
    Py_XDECREF(ret); \
    ret = NULL; \
finally: \
    return ret; \
}

/**
// ---- Meta information about the SVF ----
// The existing blocks.
t_seek_reads blocks() const noexcept;

// Information about memory used:
// size_of() gives best guess of total memory usage.
size_t size_of() const noexcept;

// Gives exact number of data bytes held.
size_t num_bytes() const noexcept { return m_bytes_total; };

// Gives exact number of blocks used.
size_t num_blocks() const noexcept { return m_svf.size(); }
t_fpos last_file_position() const noexcept;

// Check the clients file modification time has changed.
// Caller has to decide what to do...
bool file_mod_time_matches(const double &file_mod_time) const noexcept {
    return file_mod_time == m_file_mod_time;
}
*/

static const char *cp_SparseVirtualFileSystem_svf_blocks_docstring = (
        "This returns a ordered tuple ((file_position, length), ...) of all the blocks held by the SVF identified by the given id." \
    " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``blocks(id: str) -> typing.Tuple[typing.Tuple[int, int], ...]:``"
);

static PyObject *
cp_SparseVirtualFileSystem_svf_blocks(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL; // PyTupleObject
    char *c_id = NULL; // PyUnicodeObject
    std::string cpp_id;
    PyObject * insert_item = NULL; // PyTupleObject
    static const char *kwlist[] = {"id", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **) kwlist, &c_id)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            const SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            SVFS::t_seek_reads seek_read = svf.blocks();
            ret = PyTuple_New(seek_read.size());
            if (!ret) {
                PyErr_Format(PyExc_MemoryError, "%s: Can not create tuple for return", __FUNCTION__);
                goto except;
            }
            for (size_t i = 0; i < seek_read.size(); ++i) {
                insert_item = Py_BuildValue("KK", seek_read[i].first, seek_read[i].second);
                if (!insert_item) {
                    PyErr_Format(PyExc_MemoryError, "%s: Can not create tuple", __FUNCTION__);
                    goto except;
                }
                PyTuple_SET_ITEM(ret, i, insert_item);
                insert_item = NULL;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        goto except;
    }
    assert(!PyErr_Occurred());
    assert(ret);
    goto finally;
    except:
    assert(PyErr_Occurred());
    if (ret) {
        for (Py_ssize_t i = 0; i < PyList_Size(ret); ++i) {
            Py_XDECREF(PyList_GET_ITEM(ret, i));
        }
    }
    Py_XDECREF(ret);
    ret = NULL;
    finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_svf_size_of_docstring = (
        "Returns the best guess of total memory usage used by the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``size_of(id: str) -> int:``"
);

SVFS_SVFS_METHOD_SIZE_T_WRAPPER(size_of);

static const char *cp_SparseVirtualFileSystem_svf_num_bytes_docstring = (
        "Returns the number of bytes of data held by the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``num_bytes(id: str) -> int:``"
);

SVFS_SVFS_METHOD_SIZE_T_WRAPPER(num_bytes);

static const char *cp_SparseVirtualFileSystem_svf_num_blocks_docstring = (
        "Returns the number of data blocks held by the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``num_blocks(id: str) -> int:``"
);

SVFS_SVFS_METHOD_SIZE_T_WRAPPER(num_blocks);

static const char *cp_SparseVirtualFileSystem_svf_file_mod_time_matches_docstring = (
        "Returns True if the file modification time of the Sparse Virtual File identified by the given id the matches"
        " the given time as a float."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``file_mod_time_matches(id: str) -> bool:``"
);

static PyObject *
cp_SparseVirtualFileSystem_svf_file_mod_time_matches(cp_SparseVirtualFileSystem *self, PyObject *args,
                                                     PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    double file_mod_time;
    std::string cpp_id;
    static const char *kwlist[] = {"id", "file_mod_time", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sd", (char **) kwlist, &c_id, &file_mod_time)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            if (svf.file_mod_time_matches(file_mod_time)) {
                Py_INCREF(Py_True);
                ret = Py_True;
            } else {
                Py_INCREF(Py_False);
                ret = Py_False;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
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

/**
// ---- Attribute access ----
const std::string id() const noexcept { return m_id; }
double file_mod_time() const noexcept { return m_file_mod_time; }

size_t count_write() const noexcept { return m_count_write; }
size_t count_read() const noexcept { return m_count_read; }
size_t bytes_write() const noexcept { return m_bytes_write; }
size_t bytes_read() const noexcept { return m_bytes_read; }

std::chrono::time_point<std::chrono::system_clock> time_write() const noexcept { return m_time_write; }
std::chrono::time_point<std::chrono::system_clock> time_read() const noexcept { return m_time_read; }
*/

static const char *cp_SparseVirtualFileSystem_file_mod_time_docstring = (
        "Returns the file modification time as a float in UNIX time of the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``file_mod_time(id: str) -> float:``"
);

// SVF: double file_mod_time() const noexcept { return m_file_mod_time; }
static PyObject *
cp_SparseVirtualFileSystem_file_mod_time(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject * ret = NULL;
    char *c_id = NULL;
    std::string cpp_id;
    static const char *kwlist[] = {"id", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **) kwlist, &c_id)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            ret = PyFloat_FromDouble(svf.file_mod_time());
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
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

static const char *cp_SparseVirtualFileSystem_svf_count_write_docstring = (
        "Returns the count of write operations on the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``count_write(id: str) -> int:``"
);

SVFS_SVFS_METHOD_SIZE_T_WRAPPER(count_write);

static const char *cp_SparseVirtualFileSystem_svf_count_read_docstring = (
        "Returns the count of read operations on the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``count_read(id: str) -> int:``"
);

SVFS_SVFS_METHOD_SIZE_T_WRAPPER(count_read);

static const char *cp_SparseVirtualFileSystem_svf_bytes_write_docstring = (
        "Returns the count of the number of bytes writen to the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``bytes_write(id: str) -> int:``"
);

SVFS_SVFS_METHOD_SIZE_T_WRAPPER(bytes_write);

static const char *cp_SparseVirtualFileSystem_svf_bytes_read_docstring = (
        "Returns the count of the number of bytes read from the Sparse Virtual File identified by the given id."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``bytes_read(id: str) -> int:``"
);

SVFS_SVFS_METHOD_SIZE_T_WRAPPER(bytes_read);

// This macro is for functions that return a datetime type such as time_write, time_read.
#define SVFS_SVFS_METHOD_DATETIME_WRAPPER(method_name) static PyObject * \
cp_SparseVirtualFileSystem_svf_##method_name(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) { \
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs); \
    PyObject *ret = NULL; \
    char *c_id = NULL; \
    std::string cpp_id; \
    static const char *kwlist[] = { "id", NULL }; \
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **)kwlist, &c_id)) { \
        goto except; \
    } \
    cpp_id = std::string(c_id); \
    if (self->p_svfs->has(cpp_id)) { \
        const SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id); \
        auto time = svf.method_name(); \
        const long seconds = std::chrono::time_point_cast<std::chrono::seconds>(time).time_since_epoch().count(); \
        int micro_seconds = std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count() % 1000000; \
        const std::tm *p_struct_tm = std::gmtime(&seconds); \
        ret = datetime_from_struct_tm(p_struct_tm, micro_seconds); \
    } else { \
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id); \
        goto except; \
    } \
    assert(! PyErr_Occurred()); \
    assert(ret); \
    goto finally; \
except: \
    assert(PyErr_Occurred()); \
    Py_XDECREF(ret); \
    ret = NULL; \
finally: \
    return ret; \
}

// NOTE: time_read and time_write functions are very similar.

static const char *cp_SparseVirtualFileSystem_svf_time_write_docstring = (
        "Returns the timestamp of the last write to the Sparse Virtual File identified by the given id as a datetime.datetime."
        " or None if no write has taken place."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``time_write(id: str) -> typing.Optional[datetime.datetime]:``"
);
//SVFS_SVFS_METHOD_DATETIME_WRAPPER(time_write)

static PyObject *
cp_SparseVirtualFileSystem_svf_time_write(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    PyObject * ret = NULL;
    char *c_id = NULL;
    std::string cpp_id;
    static const char *kwlist[] = {"id", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **) kwlist, &c_id)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            const SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            if (svf.count_write()) {
                auto time = svf.time_write();
                const long seconds = std::chrono::time_point_cast<std::chrono::seconds>(
                        time).time_since_epoch().count();
                int micro_seconds =
                        std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count() %
                        1000000;
                const std::tm *p_struct_tm = std::gmtime(&seconds);
                ret = datetime_from_struct_tm(p_struct_tm, micro_seconds);
                if (!ret) {
                    goto except;
                }
            } else {
                Py_INCREF(Py_None);
                ret = Py_None;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No SVF ID \"%s\"", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
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


static const char *cp_SparseVirtualFileSystem_svf_time_read_docstring = (
        "Returns the timestamp of the last read from the Sparse Virtual File identified by the given id as a datetime.datetime."
        " or None if no read has taken place."
        " This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist."
        "\n\nSignature: ``time_read(id: str) -> typing.Optional[datetime.datetime]:``"
);
//SVFS_SVFS_METHOD_DATETIME_WRAPPER(time_read)

static PyObject *
cp_SparseVirtualFileSystem_svf_time_read(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    PyObject * ret = NULL;
    char *c_id = NULL;
    std::string cpp_id;
    static const char *kwlist[] = {"id", NULL};
    AcquireLockSVFS _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char **) kwlist, &c_id)) {
        goto except;
    }
    cpp_id = std::string(c_id);
    try {
        if (self->p_svfs->has(cpp_id)) {
            const SVFS::SparseVirtualFile &svf = self->p_svfs->at(cpp_id);
            if (svf.count_read()) {
                auto time = svf.time_read();
                const long seconds = std::chrono::time_point_cast<std::chrono::seconds>(
                        time).time_since_epoch().count();
                int micro_seconds =
                        std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count() %
                        1000000;
                const std::tm *p_struct_tm = std::gmtime(&seconds);
                ret = datetime_from_struct_tm(p_struct_tm, micro_seconds);
                if (!ret) {
                    goto except;
                }
            } else {
                Py_INCREF(Py_None);
                ret = Py_None;
            }
        } else {
            PyErr_Format(PyExc_IndexError, "%s: No Sparse Virtual File ID \"%s\"", __FUNCTION__, c_id);
            goto except;
        }
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
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

static const char *cp_SparseVirtualFileSystem_config_docstring = (
        "Returns the SVFS configuration as a dict."
        "\n\nSignature: ``config() -> typing.Dict[str, bool]:``"
);

static PyObject *
cp_SparseVirtualFileSystem_config(cp_SparseVirtualFileSystem *self) {
    PyObject * ret = Py_BuildValue(
            "{"
            "s:N"   /* compare_for_diff */
            ",s:N"  /* overwrite_on_exit */
            "}",
            "compare_for_diff", PyBool_FromLong(self->p_svfs->config().compare_for_diff ? 1 : 0),
            "overwrite_on_exit", PyBool_FromLong(self->p_svfs->config().overwrite_on_exit ? 1 : 0)
    );
    return ret;
}


// END: SVF functions
#pragma mark END: SVFS functions

// Mapping method for __len__
static Py_ssize_t
cp_SparseVirtualFileSystem_mapping_length(PyObject * self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    try {
        return ((cp_SparseVirtualFileSystem *) self)->p_svfs->size();
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return NULL;
    }
}

static PyMemberDef cp_SparseVirtualFileSystem_members[] = {
//        {"first", T_OBJECT_EX, offsetof(CustomObject, first), 0,
//                "first name"},
        {NULL, 0, 0, 0, NULL}  /* Sentinel */
};

static PyMethodDef cp_SparseVirtualFileSystem_methods[] = {
        {
                "keys",                  (PyCFunction) cp_SparseVirtualFileSystem_keys,            METH_NOARGS,
                cp_SparseVirtualFileSystem_keys_docstring
        },
        {
                "insert",                (PyCFunction) cp_SparseVirtualFileSystem_insert,          METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_insert_docstring
        },
        {
                "remove",                (PyCFunction) cp_SparseVirtualFileSystem_remove,          METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_remove_docstring
        },
        {
                "has",                   (PyCFunction) cp_SparseVirtualFileSystem_has,             METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_has_docstring
        },
        {
                "total_size_of",         (PyCFunction) cp_SparseVirtualFileSystem_total_size_of,   METH_NOARGS,
                cp_SparseVirtualFileSystem_total_size_of_docstring
        },
        {
                "total_bytes",           (PyCFunction) cp_SparseVirtualFileSystem_total_bytes,     METH_NOARGS,
                cp_SparseVirtualFileSystem_total_bytes_docstring
        },
        {
                "total_blocks",          (PyCFunction) cp_SparseVirtualFileSystem_total_blocks,    METH_NOARGS,
                cp_SparseVirtualFileSystem_total_blocks_docstring
        },
        {
                "has_data",              (PyCFunction) cp_SparseVirtualFileSystem_svf_has_data,    METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_has_data_docstring
        },
        {
                "write",                 (PyCFunction) cp_SparseVirtualFileSystem_svf_write,       METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_write_docstring
        },
        {
                "read",                  (PyCFunction) cp_SparseVirtualFileSystem_svf_read,        METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_read_docstring
        },
        {
                "erase",                 (PyCFunction) cp_SparseVirtualFileSystem_svf_erase,       METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_erase_docstring
        },
        {
                "need",                  (PyCFunction) cp_SparseVirtualFileSystem_svf_need,        METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_need_docstring
        },
        // ---- Meta information about the specific SVF ----
        {
                "blocks",                (PyCFunction) cp_SparseVirtualFileSystem_svf_blocks,      METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_blocks_docstring
        },
        {
                "size_of",               (PyCFunction) cp_SparseVirtualFileSystem_svf_size_of,     METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_size_of_docstring
        },
        {
                "num_bytes",             (PyCFunction) cp_SparseVirtualFileSystem_svf_num_bytes,   METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_num_bytes_docstring
        },
        {
                "num_blocks",            (PyCFunction) cp_SparseVirtualFileSystem_svf_num_blocks,  METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_num_blocks_docstring
        },
        {
                "file_mod_time_matches", (PyCFunction) cp_SparseVirtualFileSystem_svf_file_mod_time_matches,
                                                                                                   METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_file_mod_time_matches_docstring
        },
        // ---- Attribute access ----
        {
                "file_mod_time",         (PyCFunction) cp_SparseVirtualFileSystem_file_mod_time,   METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_file_mod_time_docstring
        },
        {
                "count_write",           (PyCFunction) cp_SparseVirtualFileSystem_svf_count_write, METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_count_write_docstring
        },
        {
                "count_read",            (PyCFunction) cp_SparseVirtualFileSystem_svf_count_read,  METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_count_read_docstring
        },
        {
                "bytes_write",           (PyCFunction) cp_SparseVirtualFileSystem_svf_bytes_write, METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_bytes_write_docstring
        },
        {
                "bytes_read",            (PyCFunction) cp_SparseVirtualFileSystem_svf_bytes_read,  METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_bytes_read_docstring
        },
        {
                "time_write",            (PyCFunction) cp_SparseVirtualFileSystem_svf_time_write,  METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_time_write_docstring
        },
        {
                "time_read",             (PyCFunction) cp_SparseVirtualFileSystem_svf_time_read,   METH_VARARGS |
                                                                                                   METH_KEYWORDS,
                        cp_SparseVirtualFileSystem_svf_time_read_docstring
        },
        {
                "config",                (PyCFunction) cp_SparseVirtualFileSystem_config,          METH_NOARGS,
                cp_SparseVirtualFileSystem_config_docstring
        },
        {NULL, NULL, 0, NULL}  /* Sentinel */
};

PyMappingMethods svfs_mapping_methods = {
        // Just length as we don't (yet) want to expose the underlying SVF to Python code.
        // via either mp_subscript (get), mp_ass_subscript (set).
        .mp_length = &cp_SparseVirtualFileSystem_mapping_length,
};

// clang-format off
// @formatter:off
static const char *svfs_cSVFS_doc = PyDoc_STR(
                                            "This class implements a Sparse Virtual File System where Sparse Virtual Files are mapped to a key (a string)."
                                            " This can be constructed with an optional boolean overwrite flag that ensures in-memory data is overwritten"
                                            " on destruction of any SVF."
                                    );
// clang-format on
// @formatter.on

static PyTypeObject svfsc_cSVFS = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "svfsc.cSVFS",
        .tp_basicsize = sizeof(cp_SparseVirtualFileSystem),
        .tp_itemsize = 0,
        .tp_dealloc = (destructor) cp_SparseVirtualFileSystem_dealloc,
        // Mapping methods, just __len__
        .tp_as_mapping = &svfs_mapping_methods,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_doc = svfs_cSVFS_doc,
        .tp_methods = cp_SparseVirtualFileSystem_methods,
        .tp_members = cp_SparseVirtualFileSystem_members,
        .tp_init = (initproc) cp_SparseVirtualFileSystem_init,
        .tp_new = cp_SparseVirtualFileSystem_new,
};

#pragma mark - END SVFS
