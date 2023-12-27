/** @file
 * Python wrapper around a C++ SparseVirtualFile.
 *
 * Naming convention:
 *
 * SVF functions are named cp_SparseVirtualFile_...
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

#include "cp_svfs.h"

#include <ctime>
#include <memory>

#include "svf.h"
#include "svfs_util.h"

/** TODO: Implement the Buffer Protocol rather than returning a copy of the bytes? Look for PyBytes_FromStringAndSize().
 * */

/**
 *
 * This macro is for functions that return a size_t type such as count_write, count_read, bytes_write, bytes_read.
 *
 * Usage:
 *
 * PyDoc_STRVAR(
 * cp_SparseVirtualFile_count_write_docstring,
        "count_write(self) -> int\n\n"
        "Returns the count of write operations on the Sparse Virtual File."
 * );
 *
 * Perhaps ret = Py_BuildValue("K", self->pSvf->method_name());
 */
#define SVFS_SVF_METHOD_SIZE_T_WRAPPER(method_name, docstring) \
PyDoc_STRVAR(                                                  \
    cp_SparseVirtualFile_##method_name##_docstring,            \
    #method_name"(self) -> int\n\n"                                \
    docstring                                                  \
);\
static PyObject * \
cp_SparseVirtualFile_##method_name(cp_SparseVirtualFile *self) { \
    ASSERT_FUNCTION_ENTRY_SVF(pSvf); \
    PyObject *ret = NULL; \
    try { \
        ret = PyLong_FromLong(self->pSvf->method_name()); \
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
 * @brief Python wrapper around a C++ SparseVirtualFile.
 *
 * If \c PY_THREAD_SAFE is defined then this also contains a mutex.
 */
typedef struct {
    PyObject_HEAD
    SVFS::SparseVirtualFile *pSvf;
#ifdef PY_THREAD_SAFE
    PyThread_type_lock lock;
#endif
} cp_SparseVirtualFile;

#ifdef PY_THREAD_SAFE

/**
 * @brief A RAII wrapper around the PyThread_type_lock for the CPython SVF.
 *
 * See https://pythonextensionpatterns.readthedocs.io/en/latest/thread_safety.html
 * */
class AcquireLockSVF {
public:
    /**
     * Acquire the lock on the Python cp_SparseVirtualFile
     *
     * @param pSVF The Python cp_SparseVirtualFile.
     */
    explicit AcquireLockSVF(cp_SparseVirtualFile *pSVF) : _pSVF(pSVF) {
        assert(_pSVF);
        assert(_pSVF->lock);
        if (!PyThread_acquire_lock(_pSVF->lock, NOWAIT_LOCK)) {
            Py_BEGIN_ALLOW_THREADS
                PyThread_acquire_lock(_pSVF->lock, WAIT_LOCK);
            Py_END_ALLOW_THREADS
        }
    }

    /**
     * Release the lock on the Python cp_SparseVirtualFile
     *
     * @param pSVF The Python cp_SparseVirtualFile.
     */
    ~AcquireLockSVF() {
        assert(_pSVF);
        assert(_pSVF->lock);
        PyThread_release_lock(_pSVF->lock);
    }

private:
    cp_SparseVirtualFile *_pSVF;
};

#else
/** Make the class a NOP which should get optimised out. */
class AcquireLockSVF {
public:
    AcquireLockSVF(cp_SparseVirtualFile *) {}
};
#endif

/** Function entry point test macro.
 * After construction, we expect this invariant at the entry to each function.
 * The cast is necessary when used with functions that take a \c SVFS as a \c PyObject* such as
 * cp_SparseVirtualFile_mapping_length
 */
#define ASSERT_FUNCTION_ENTRY_SVF(member) do { \
    assert(self); \
    assert(((cp_SparseVirtualFile *)self)->member); \
    assert(! PyErr_Occurred()); \
} while (0)


// Construction and destruction
#pragma mark Construction and destruction

/**
 * Python \c __new__() for a \c cp_SparseVirtualFile.
 * Contents will be \c NULL.
 *
 * @param type The cp_SparseVirtualFile type.
 * @param _unused_args Unused.
 * @param _unused_kwds Unused.
 * @return An empty cp_SparseVirtualFile.
 */
static PyObject *
cp_SparseVirtualFile_new(PyTypeObject *type, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds)) {
    assert(!PyErr_Occurred());
    cp_SparseVirtualFile *self;
    self = (cp_SparseVirtualFile *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->pSvf = nullptr;
#ifdef PY_THREAD_SAFE
        self->lock = NULL;
#endif
    }
//    PyObject_Print((PyObject *)self, stdout);
//    fprintf(stdout, "cp_SparseVirtualFile_new() self %p\n", (void *)self);
    assert(!PyErr_Occurred());
    return (PyObject *) self;
}

/**
 * Initialise the cp_SparseVirtualFile.
 *
 * See the defaults for SVFS::SparseVirtualFileConfig for how that works.
 *
 * Arguments/keywords:
 *
 * - @c id Mandatory, str, ID.
 * - @c mod_time Optional, float, modification time, defaults to 0.0
 * - @c overwrite_on_exit Optional, bool, See the defaults for SVFS::SparseVirtualFileConfig
 * - @c compare_for_diff Optional, bool, See the defaults for SVFS::SparseVirtualFileConfig
 *
 * @param self The cp_SparseVirtualFile.
 * @param args Order: "id", "mod_time", "overwrite_on_exit", "compare_for_diff".
 * @param kwargs Can be "id", "mod_time", "overwrite_on_exit", "compare_for_diff".
 * @return Zero on success, non-zero on failure.
 */
static int
cp_SparseVirtualFile_init(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    assert(!PyErr_Occurred());

    char *c_id = NULL;
    double mod_time = 0.0;
    static const char *kwlist[] = {"id", "mod_time", "overwrite_on_exit", "compare_for_diff", NULL};
    SVFS::tSparseVirtualFileConfig config;

//    TRACE_SELF_ARGS_KWARGS;
//    fprintf(stdout, "Config was compare_for_diff=%d overwrite_on_exit=%d\n", config.compare_for_diff,
//            config.overwrite_on_exit);

    // NOTE: With format unit 'p' we need to pass in an int.
    int overwrite_on_exit = config.overwrite_on_exit ? 1 : 0;
    int compare_for_diff = config.compare_for_diff ? 1 : 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dpp", (char **) kwlist, &c_id, &mod_time,
                                     &overwrite_on_exit, &compare_for_diff)) {
        assert(PyErr_Occurred());
        return -1;
    }
    config.overwrite_on_exit = overwrite_on_exit != 0;
    config.compare_for_diff = compare_for_diff != 0;

//    fprintf(stdout, "Config now compare_for_diff=%d overwrite_on_exit=%d\n", config.compare_for_diff,
//            config.overwrite_on_exit);

    try {
        self->pSvf = new SVFS::SparseVirtualFile(c_id, mod_time, config);
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return -1;
    }
#ifdef PY_THREAD_SAFE
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        delete self->pSvf;
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate thread lock.");
        return -2;
    }
#endif
//    fprintf(stdout, "cp_SparseVirtualFile_init() self->pSvf %p\n", (void *)self->pSvf);
    assert(!PyErr_Occurred());
    return 0;
}

/**
 * Deallocate the SparseVirtualFile.
 * @param self The Python SparseVirtualFile.
 */
static void
cp_SparseVirtualFile_dealloc(cp_SparseVirtualFile *self) {
    delete self->pSvf;
#ifdef PY_THREAD_SAFE
    if (self->lock) {
        PyThread_free_lock(self->lock);
        self->lock = NULL;
    }
#endif
    Py_TYPE(self)->tp_free((PyObject *) self);
}

// END: Construction and destruction
#pragma mark END: Construction and destruction

// SVFS functions
#pragma mark SVF functions

PyDoc_STRVAR(
        cp_SparseVirtualFile_id_docstring,
        "id(self) -> str\n\n"
        "Returns the ID of the Sparse Virtual File."
);

static PyObject *
cp_SparseVirtualFile_id(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    ret = PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, self->pSvf->id().c_str(), self->pSvf->id().size());
    if (!ret) {
        PyErr_Format(PyExc_RuntimeError, "%s: Can not create id for %s", __FUNCTION__, self->pSvf->id().c_str());
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


SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        size_of,
        "Returns the estimate of total memory usage of the Sparse Virtual File."
);


SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        num_bytes,
        "Returns the total number of file bytes held by the Sparse Virtual File."
);


SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        num_blocks,
        "Returns the total number of blocks of data held by the Sparse Virtual File System."
);


SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        last_file_position,
        "Returns the file position immediately past the last block."
);

PyDoc_STRVAR(
        cp_SparseVirtualFile_has_data_docstring,
        "has_data(self, file_position: int, length: int) -> bool\n\n"
        "Checks if the Sparse Virtual File of the ID has data at the given ``file_position`` and ``length``.\n\n"
        "Parameters\n\n"
        "file_position: int\n"
        "    The absolute file position of the start of the data.\n\n"
        "length: int\n"
        "    The length of the required data in bytes.\n\n"
        "Returns\n\n"
        "bool: True if the SVF contains the data, False otherwise."
);

static PyObject *
cp_SparseVirtualFile_has_data(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = {"file_position", "length", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KK", (char **) kwlist, &fpos, &len)) {
        goto except;
    }
    try {
        if (self->pSvf->has(fpos, len)) {
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

PyDoc_STRVAR(
        cp_SparseVirtualFile_write_docstring,
        "write(self, file_position: int, data: bytes) -> None\n\n"
        "Writes the data to the Sparse Virtual File of the given ID at ``file_position`` and ``data`` as a ``bytes`` object."
        " This will raise an ``IOError`` if ``self.compare_for_diff`` is True and given data is different than"
        " that seen before and only new data up to this point will be written."
        " If the ``byte`` data is empty nothing will be done."
        " This will raise a RuntimeError if the data can not be written for any other reason"
);

static PyObject *
cp_SparseVirtualFile_write(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    unsigned long long fpos = 0;
    PyObject * py_bytes_data = NULL;
    static const char *kwlist[] = {"file_position", "data", NULL};
    AcquireLockSVF _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KS", (char **) kwlist, &fpos, &py_bytes_data)) {
        goto except;
    }
    if (PyBytes_GET_SIZE(py_bytes_data) > 0) {
        try {
//            fprintf(stdout, "TRACE: %s Writing fpos %llu length %zd\n", __FUNCTION__, fpos, PyBytes_Size(py_bytes_data));
            self->pSvf->write(fpos, PyBytes_AS_STRING(py_bytes_data), PyBytes_Size(py_bytes_data));
        } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileDiff &err) {
            PyErr_Format(PyExc_IOError,
                         "%s: Can not write to a SVF as the given data is different from what is there. ERROR: %s",
                         __FUNCTION__, err.message().c_str());
            goto except;
        } catch (const SVFS::Exceptions::ExceptionSparseVirtualFile &err) {
            PyErr_Format(PyExc_RuntimeError, "%s: Can not write to a SVF. ERROR: %s",
                         __FUNCTION__, err.message().c_str());
            goto except;
        } catch (const std::exception &err) {
            PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
            goto except;
        }
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

PyDoc_STRVAR(
        cp_SparseVirtualFile_read_docstring,
        "read(self, file_position: int, length: int) -> bytes\n\n"
        "Read the data from the Sparse Virtual File at ``file_position`` and ``length`` returning a ``bytes`` object."
        " This takes a file position and a length."
        " This will raise an ``IOError`` if any data is not present"
        " This will raise a ``RuntimeError`` if the data can not be read for any other reason"
);

static PyObject *
private_SparseVirtualFile_svf_read_as_py_bytes(cp_SparseVirtualFile *self, unsigned long long fpos,
                                               unsigned long long len) {
    // Create a bytes object
    PyObject * ret = PyBytes_FromStringAndSize(NULL, len);
    if (!ret) {
        PyErr_Format(PyExc_RuntimeError, "%s()#%d: Could not create bytes object.", __FUNCTION__, __LINE__);
        return NULL;
    }
    try {
        self->pSvf->read(fpos, len, PyBytes_AS_STRING(ret));
    } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileRead &err) {
        PyErr_Format(PyExc_IOError, "%s()#%d: Can not read from a SVF. ERROR: %s",
                     __FUNCTION__, __LINE__, err.message().c_str());
        Py_DECREF(ret);
        return NULL;
    } catch (const SVFS::Exceptions::ExceptionSparseVirtualFile &err) {
        PyErr_Format(PyExc_RuntimeError, "%s()#%d: Fatal error reading from a SVF. ERROR: %s",
                     __FUNCTION__, __LINE__, err.message().c_str());
        Py_DECREF(ret);
        return NULL;
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s()#%d: FATAL caught std::exception %s", __FUNCTION__, __LINE__,
                     err.what());
        Py_DECREF(ret);
        return NULL;
    }
    return ret;
}

static PyObject *
cp_SparseVirtualFile_read(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = {"file_position", "length", NULL};
    AcquireLockSVF _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KK", (char **) kwlist, &fpos, &len)) {
        goto except;
    }
    // Create a bytes object
    ret = private_SparseVirtualFile_svf_read_as_py_bytes(self, fpos, len);
    if (ret == NULL) {
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


PyDoc_STRVAR(
        cp_SparseVirtualFile_erase_docstring,
        "erase(self, file_position: int) -> None\n\n"
        "Erase the data from the Sparse Virtual File at the given ``file_position``"
        " which must be the beginning of a block.\n"
        "This will raise an ``IOError`` if a block is not present at that file position."
);

static PyObject *
cp_SparseVirtualFile_erase(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    unsigned long long fpos = 0;
    static const char *kwlist[] = {"file_position", NULL};
    AcquireLockSVF _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "K", (char **) kwlist, &fpos)) {
        return NULL;
    }
    try {
        self->pSvf->erase(fpos);
    } catch (const SVFS::Exceptions::ExceptionSparseVirtualFileErase &err) {
        /* Do not include line as this can vary between versions. */
        PyErr_Format(PyExc_IOError, "%s(): Can not erase from a SVF. ERROR: %s",
                     __FUNCTION__, err.message().c_str());
        return NULL;
    } catch (const SVFS::Exceptions::ExceptionSparseVirtualFile &err) {
        /* Do not include line as this can vary between versions. */
        PyErr_Format(PyExc_RuntimeError, "%s(): Fatal error reading from a SVF. ERROR: %s",
                     __FUNCTION__, err.message().c_str());
        return NULL;
    } catch (const std::exception &err) {
        /* Do not include line as this can vary between versions. */
        PyErr_Format(PyExc_RuntimeError, "%s(): FATAL caught std::exception %s", __FUNCTION__, __LINE__,
                     err.what());
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(
        cp_SparseVirtualFile_need_docstring,
        "need(self, file_position: int, length: int, greedy_length: int = 0) -> typing.Tuple[typing.Tuple[int, int], ...]\n\n"
        "Given a file_position and length this returns a ordered list ``[(file_position, length), ...]`` of seek/read"
        " instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will"
        " succeed.\n"
        " If greedy_length is > 0 then, if possible, blocks will be coalesced to reduce the size of the return value."
        "\n\n"
        ".. warning::\n"
        "    The SVF has no knowledge of the the actual file size so when using a greedy length the need list might"
        " include positions beyond EOF.\n\n"
        "    For example a file 1024 bytes long and a greedy length of 256 then ``need(1000, 24, 256)`` will create"
        " a need list of ``[(1000, 256),]``."
        " This should generate a ``write(1000, 24)`` not a ``write(1000, 256)``.\n\n"
        "    It is up to the caller to handle this, however, ``reads()`` in C/C++/Python will ignore read lengths past"
        " EOF so the caller does not have to do anything.\n\n"
        "\n\nUsage::\n\n"
        "    if not svf.has_data(position, length):\n"
        "        for read_fpos, read_length in svf.need(position, length):\n"
        "            # Somehow get data as a bytes object at read_fpos, read_length...\n"
        "            svf.write(read_fpos, data)\n"
        "    return svf.read(position, length):\n"
);

/**
 * See cp_SparseVirtualFile_need_docstring
 *
 * @param self The cp_SparseVirtualFile
 * @param args The file_position and length. Optionally a greedy_length.
 * @param kwargs "file_position", "length", "greedy_length".
 * @return List of tuples (file_position, length).
 */
static PyObject *
cp_SparseVirtualFile_need(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // PyListObject
    PyObject * list_item = NULL; // PyTupleObject
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    unsigned long long greedy_len = 0;
    static const char *kwlist[] = {"file_position", "length", "greedy_length", NULL};
    AcquireLockSVF _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KK|K", (char **) kwlist, &fpos, &len, &greedy_len)) {
        goto except;
    }
    try {
        SVFS::t_seek_reads seek_read = self->pSvf->need(fpos, len, greedy_len);
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

PyDoc_STRVAR(
        cp_SparseVirtualFile_blocks_docstring,
        "blocks(self) -> typing.Tuple[typing.Tuple[int, int], ...]\n\n"
        "This returns a ordered tuple ``((file_position, length), ...)``"
        " of the shape of the blocks held by the SVF in file position order."
);

static PyObject *
cp_SparseVirtualFile_blocks(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // PyTupleObject
    PyObject * insert_item = NULL; // PyTupleObject
    AcquireLockSVF _lock(self);

    try {
        SVFS::t_seek_reads seek_read = self->pSvf->blocks();
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

SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        block_touch,
        "Return the latest value of the monotonically increasing block_touch value."
);

PyDoc_STRVAR(
        cp_SparseVirtualFile_block_touches_docstring,
        "block_touches(self) -> typing.Dict[int, int]\n\n"
        "This returns a dict ``{touch_int: file_position, ...}``"
        " of the touch integer of each block mapped to the file position.\n"
        "The caller can decide what older blocks can be used the erase(file_position)."
);

static PyObject *
cp_SparseVirtualFile_block_touches(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // PyDictObject
    AcquireLockSVF _lock(self);

    try {
        ret = PyDict_New();
        if (!ret) {
            PyErr_Format(PyExc_MemoryError, "%s: Can not create dict for return", __FUNCTION__);
            goto except;
        }
        for (const auto &iter: self->pSvf->block_touches()) {
            PyObject * key = PyLong_FromLong(iter.first);
            if (!key) {
                PyErr_Format(PyExc_MemoryError, "%s: Can not create key", __FUNCTION__);
                goto except;
            }
            PyObject * val = PyLong_FromLong(iter.second);
            if (!val) {
                PyErr_Format(PyExc_MemoryError, "%s: Can not create value", __FUNCTION__);
                goto except;
            }
            PyDict_SetItem(ret, key, val);
            Py_DECREF(key);
            Py_DECREF(val);
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

PyDoc_STRVAR(
        cp_SparseVirtualFile_lru_punt_docstring,
        "lru_punt(self, cache_size_upper_bound: int) -> int\n\n"
        "Reduces the size of the cache to < the given size by removing older blocks, at least one block will be left.\n"
        "There are limitations to this tactic, see the documentation in Technical Notes -> Cache Punting."
);

/**
 * See cp_SparseVirtualFile_lru_punt_docstring
 *
 * @param self The cp_SparseVirtualFile
 * @param args The upper bound of the number of bytes held by the cache.
 * @param kwargs "cache_size_upper_bound".
 * @return Number of bytes removed.
 */
static PyObject *
cp_SparseVirtualFile_lru_punt(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // Long
    size_t cache_size_upper_bound;
    static const char *kwlist[] = {"cache_size_upper_bound", NULL};
    AcquireLockSVF _lock(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "K", (char **) kwlist, &cache_size_upper_bound)) {
        goto except;
    }
    try {
        ret = Py_BuildValue("K", self->pSvf->lru_punt(cache_size_upper_bound));
        if (!ret) {
            PyErr_Format(PyExc_MemoryError, "%s: Can not create long", __FUNCTION__);
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

PyDoc_STRVAR(
        cp_SparseVirtualFile_file_mod_time_matches_docstring,
        "file_mod_time_matches(self, file_mod_time: float) -> bool\n\n"
        "Returns True if the file modification time of the Sparse Virtual File matches the given time as a float."
);

static PyObject *
cp_SparseVirtualFile_file_mod_time_matches(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    double file_mod_time;
    static const char *kwlist[] = {"file_mod_time", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d", (char **) kwlist, &file_mod_time)) {
        goto except;
    }
    try {
        if (self->pSvf->file_mod_time_matches(file_mod_time)) {
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

PyDoc_STRVAR(
        cp_SparseVirtualFile_file_mod_time_docstring,
        "file_mod_time(self) -> float\n\n"
        "Returns the file modification time as a float in UNIX time of the Sparse Virtual File."
);

static PyObject *
cp_SparseVirtualFile_file_mod_time(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    try {
        ret = PyFloat_FromDouble(self->pSvf->file_mod_time());
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

SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        count_write,
        "Returns the count of write operations on the Sparse Virtual File."
);

SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        count_read,
        "Returns the count of read operations on the Sparse Virtual File."
);

SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        bytes_write,
        "Returns the count of the number of bytes writen to the Sparse Virtual File."
);

SVFS_SVF_METHOD_SIZE_T_WRAPPER(
        bytes_read,
        "Returns the count of the number of bytes read from the Sparse Virtual File."
);

// NOTE: time_read and time_write functions are very similar.

PyDoc_STRVAR(
        cp_SparseVirtualFile_time_write_docstring,
        "time_write(self) -> typing.Optional[datetime.datetime]\n\n"
        "Returns the timestamp of the last write to the Sparse Virtual File as a ``datetime.datetime``"
        " or ``None`` if no write has taken place."
);
//SVFS_SVF_METHOD_DATETIME_WRAPPER(time_write)

static PyObject *
cp_SparseVirtualFile_time_write(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);
    PyObject * ret = NULL;
    try {
        if (self->pSvf->count_write()) {
            auto time = self->pSvf->time_write();
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


PyDoc_STRVAR(
        cp_SparseVirtualFile_time_read_docstring,
        "time_read(self) -> typing.Optional[datetime.datetime]\n\n"
        "Returns the timestamp of the last read from the Sparse Virtual File as a ``datetime.datetime``"
        " or ``None`` if no read has taken place."
);
//SVFS_SVF_METHOD_DATETIME_WRAPPER(time_read)

static PyObject *
cp_SparseVirtualFile_time_read(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    try {
        if (self->pSvf->count_read()) {
            auto time = self->pSvf->time_read();
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

PyDoc_STRVAR(
        cp_SparseVirtualFile_config_docstring,
        "config(self) -> typing.Dict[str, bool]\n\n"
        "Returns the SVF configuration as a dict."
);

static PyObject *
cp_SparseVirtualFile_config(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = Py_BuildValue(
            "{"
            "s:N"   /* compare_for_diff */
            ",s:N"  /* overwrite_on_exit */
            "}",
            "compare_for_diff", PyBool_FromLong(self->pSvf->config().compare_for_diff ? 1 : 0),
            "overwrite_on_exit", PyBool_FromLong(self->pSvf->config().overwrite_on_exit ? 1 : 0)
    );
    return ret;
}

/* Pickle the object */
#pragma mark Pickling

static const char *PICKLE_ID_KEY = "id";
static const char *PICKLE_FILE_MOD_TIME_KEY = "file_mod_time";
static const char *PICKLE_BLOCKS_KEY = "blocks";
static const char *PICKLE_VERSION_KEY = "pickle_version";
static int PICKLE_VERSION = 1;

/**
 * Returns a Python dict suitable for pickling.
 * Key/values are:
 * id, file_mod_time, list_of_file_pos/bytes
 * @param self
 * @param _unused_ignored
 * @return
 */
static PyObject *
cp_SparseVirtualFile___getstate__(cp_SparseVirtualFile *self, PyObject *Py_UNUSED(ignored)) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    SVFS::t_seek_reads blocks_fpos_len = self->pSvf->blocks();
    /* Build a tuple of ((fpos, bytes), ...) */
    PyObject * blocks_fpos_bytes = PyTuple_New(blocks_fpos_len.size());
    if (!blocks_fpos_bytes) {
        PyErr_Format(PyExc_RuntimeError, "%s()#d Can not create blocks tuple.", __FUNCTION__, __LINE__);
        return NULL;
    }
    Py_ssize_t index = 0;
    for (SVFS::t_seek_reads::const_iterator iter = blocks_fpos_len.cbegin(); iter != blocks_fpos_len.cend(); ++iter) {
        PyObject * bytes_object = private_SparseVirtualFile_svf_read_as_py_bytes(self, iter->first, iter->second);
        if (bytes_object == NULL) {
            Py_DECREF(blocks_fpos_bytes);
            PyErr_Format(PyExc_RuntimeError, "%s()#d Can not create a bytes object.", __FUNCTION__, __LINE__);
            return NULL;
        }
        /* value is (fpos, bytes) */
        PyObject * fpos_bytes = Py_BuildValue("KN", iter->first, bytes_object);
        if (fpos_bytes == NULL) {
            Py_DECREF(bytes_object);
            Py_DECREF(blocks_fpos_bytes);
            PyErr_Format(PyExc_RuntimeError, "%s()#d Can not build a value.", __FUNCTION__, __LINE__);
            return NULL;
        }
        PyTuple_SET_ITEM(blocks_fpos_bytes, index, fpos_bytes);
        ++index;
    }
    /* Now build the pickle dict. */
    PyObject * ret = Py_BuildValue(
            "{"
            "s:N"   /* id */
            ",s:d"  /* file_mod_time */
            ",s:N"  /* blocks */
            ",s:i"  /* pickle_version */
            "}",
            PICKLE_ID_KEY, PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND,
                                                     self->pSvf->id().c_str(),
                                                     self->pSvf->id().size()
            ),
            PICKLE_FILE_MOD_TIME_KEY, self->pSvf->file_mod_time(),
            PICKLE_BLOCKS_KEY, blocks_fpos_bytes,
            PICKLE_VERSION_KEY, PICKLE_VERSION
    );
    if (!ret) {
        Py_DECREF(blocks_fpos_bytes);
    }
    return ret;
}

static PyObject *
cp_SparseVirtualFile___setstate__(cp_SparseVirtualFile *self, PyObject *state) {
//    PyObject * key, *value;
//    Py_ssize_t pos = 0;

    if (!PyDict_CheckExact(state)) {
        PyErr_Format(PyExc_ValueError, "%s()#%d: Pickled object is not a dict.", __FUNCTION__, __LINE__);
        return NULL;
    }
    /* Version check. */
    /* Borrowed reference but no need to increment as we create a C long from it. */
    PyObject * temp = PyDict_GetItemString(state, PICKLE_VERSION_KEY);
    if (temp == NULL) {
        /* PyDict_GetItemString does not set any error state so we have to. */
        PyErr_Format(PyExc_KeyError, "%s()#%d: No \"%s\" in pickled dict.", __FUNCTION__, __LINE__, PICKLE_VERSION_KEY);
        return NULL;
    }
    int pickle_version = (int) PyLong_AsLong(temp);
    if (pickle_version != PICKLE_VERSION) {
        PyErr_Format(PyExc_ValueError, "%s()#d Pickle version mismatch. Got version %d but expected version %d.",
                     __FUNCTION__, __LINE__, pickle_version, PICKLE_VERSION);
        return NULL;
    }
    /* Create a tuple to pass in as args.*/
    PyObject * id = PyDict_GetItemString(state, PICKLE_ID_KEY); /* Borrowed reference. */
    if (id == NULL) {
        PyErr_Format(PyExc_KeyError, "%s()#%d: No \"%s\" in pickled dict.", __FUNCTION__, __LINE__, PICKLE_ID_KEY);
        return NULL;
    }
    if (!PyUnicode_Check(id)) {
        PyErr_Format(PyExc_TypeError, "%s()#%d: \"%s\" is not string.", __FUNCTION__, __LINE__, PICKLE_ID_KEY);
        return NULL;
    }
    Py_INCREF(id);
    PyObject * file_mod_time = PyDict_GetItemString(state, PICKLE_FILE_MOD_TIME_KEY); /* Borrowed reference. */
    if (file_mod_time == NULL) {
        Py_DECREF(id);
        PyErr_Format(PyExc_KeyError, "%s()#%d: No \"%s\" in pickled dict.", __FUNCTION__,
                     __LINE__, PICKLE_FILE_MOD_TIME_KEY);
        return NULL;
    }
    if (!PyFloat_Check(file_mod_time)) {
        Py_DECREF(id);
        PyErr_Format(PyExc_TypeError, "%s()#%d: \"%s\" is not a double.", __FUNCTION__,
                     __LINE__, PICKLE_FILE_MOD_TIME_KEY);
        return NULL;
    }
    Py_INCREF(file_mod_time);
    PyObject * args = Py_BuildValue("OO", id, file_mod_time); /* New reference. */
    if (args == NULL) {
        Py_DECREF(file_mod_time);
        Py_DECREF(id);
        PyErr_Format(PyExc_RuntimeError, "%s()#d Can not create arguments.", __FUNCTION__, __LINE__);
        return NULL;
    }
    delete self->pSvf;
    self->pSvf = NULL;
    if (cp_SparseVirtualFile_init(self, args, NULL)) {
        Py_DECREF(file_mod_time);
        Py_DECREF(id);
        PyErr_Format(PyExc_RuntimeError, "%s()#%d: Can not create new SVF object.", __FUNCTION__, __LINE__);
        return NULL;

    }
    Py_DECREF(args);
    args = NULL;
    Py_DECREF(file_mod_time);
    file_mod_time = NULL;
    Py_DECREF(id);
    id = NULL;
    /* Play back the blocks. */
    PyObject * blocks = PyDict_GetItemString(state, PICKLE_BLOCKS_KEY); /* Borrowed reference. */
    if (blocks == NULL) {
        PyErr_Format(PyExc_KeyError, "%s()#%d: No \"%s\" in pickled dict.", __FUNCTION__, __LINE__, PICKLE_BLOCKS_KEY);
        return NULL;
    }
    if (!PyTuple_Check(blocks)) {
        PyErr_Format(PyExc_TypeError, "%s()#%d: \"%s\" is not a tuple.", __FUNCTION__, __LINE__, PICKLE_BLOCKS_KEY);
        return NULL;
    }
    Py_INCREF(blocks);
    for (Py_ssize_t i = 0; i < PyTuple_Size(blocks); ++i) {
        PyObject * fpos_bytes = PyTuple_GetItem(blocks, i); /* Borrowed reference. */
        Py_INCREF(fpos_bytes);
        unsigned long long fpos;
        PyObject * block_bytes = NULL; /* Borrowed reference. */
        if (!PyArg_ParseTuple(fpos_bytes, "KO", &fpos, &block_bytes)) {
            PyErr_Format(PyExc_ValueError, "%s()#%d: Can not parse block (fpos, bytes) tuple.", __FUNCTION__,
                         __LINE__, PICKLE_BLOCKS_KEY);
            Py_DECREF(fpos_bytes);
            return NULL;
        }
        Py_INCREF(block_bytes);
        if (!PyBytes_Check(block_bytes)) {
            PyErr_Format(PyExc_TypeError, "%s()#%d: Second item of \"%s\" is not a bytes object.", __FUNCTION__,
                         __LINE__, PICKLE_BLOCKS_KEY);
            Py_DECREF(block_bytes);
            Py_DECREF(fpos_bytes);
            return NULL;
        }
        self->pSvf->write(fpos, PyBytes_AS_STRING(block_bytes), PyBytes_GET_SIZE(block_bytes));
        Py_DECREF(block_bytes);
        Py_DECREF(fpos_bytes);
    }
    Py_DECREF(blocks);
    blocks = NULL;
    Py_RETURN_NONE;
}

#pragma mark END Pickling

#pragma mark END: SVF functions

static PyMemberDef cp_SparseVirtualFile_members[] = {
//        {"first", T_OBJECT_EX, offsetof(CustomObject, first), 0,
//                "first name"},
        {NULL, 0, 0, 0, NULL}  /* Sentinel */
};

static PyMethodDef cp_SparseVirtualFile_methods[] = {
        {
                "id",                    (PyCFunction) cp_SparseVirtualFile_id,                 METH_NOARGS,
                cp_SparseVirtualFile_id_docstring
        },
        {
                "size_of",               (PyCFunction) cp_SparseVirtualFile_size_of,            METH_NOARGS,
                cp_SparseVirtualFile_size_of_docstring
        },
        {
                "num_bytes",             (PyCFunction) cp_SparseVirtualFile_num_bytes,          METH_NOARGS,
                cp_SparseVirtualFile_num_bytes_docstring
        },
        {
                "num_blocks",            (PyCFunction) cp_SparseVirtualFile_num_blocks,         METH_NOARGS,
                cp_SparseVirtualFile_num_blocks_docstring
        },
        {
                "last_file_position",    (PyCFunction) cp_SparseVirtualFile_last_file_position, METH_NOARGS,
                cp_SparseVirtualFile_last_file_position_docstring
        },
        {
                "has_data",              (PyCFunction) cp_SparseVirtualFile_has_data,           METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_has_data_docstring
        },
        {
                "write",                 (PyCFunction) cp_SparseVirtualFile_write,              METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_write_docstring
        },
        {
                "read",                  (PyCFunction) cp_SparseVirtualFile_read,               METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_read_docstring
        },
        {
                "erase",                 (PyCFunction) cp_SparseVirtualFile_erase,              METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_erase_docstring
        },
        {
                "need",                  (PyCFunction) cp_SparseVirtualFile_need,               METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_need_docstring
        },
        // ---- Meta information about the specific SVF ----
        {
                "blocks",                (PyCFunction) cp_SparseVirtualFile_blocks,             METH_NOARGS,
                cp_SparseVirtualFile_blocks_docstring
        },
        {
                "block_touch",           (PyCFunction) cp_SparseVirtualFile_block_touch,        METH_NOARGS,
                cp_SparseVirtualFile_block_touch_docstring
        },
        {
                "block_touches",         (PyCFunction) cp_SparseVirtualFile_block_touches,      METH_NOARGS,
                cp_SparseVirtualFile_block_touches_docstring
        },
        {
                "lru_punt",              (PyCFunction) cp_SparseVirtualFile_lru_punt,
                                                                                                METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_lru_punt_docstring
        },
        {
                "file_mod_time_matches", (PyCFunction) cp_SparseVirtualFile_file_mod_time_matches,
                                                                                                METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_file_mod_time_matches_docstring
        },
        // ---- Attribute access ----
        {
                "file_mod_time",         (PyCFunction) cp_SparseVirtualFile_file_mod_time,      METH_NOARGS,
                cp_SparseVirtualFile_file_mod_time_docstring
        },
        {
                "count_write",           (PyCFunction) cp_SparseVirtualFile_count_write,        METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_count_write_docstring
        },
        {
                "count_read",            (PyCFunction) cp_SparseVirtualFile_count_read,         METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_count_read_docstring
        },
        {
                "bytes_write",           (PyCFunction) cp_SparseVirtualFile_bytes_write,        METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_bytes_write_docstring
        },
        {
                "bytes_read",            (PyCFunction) cp_SparseVirtualFile_bytes_read,         METH_VARARGS |
                                                                                                METH_KEYWORDS,
                        cp_SparseVirtualFile_bytes_read_docstring
        },
        {
                "time_write",            (PyCFunction) cp_SparseVirtualFile_time_write,         METH_NOARGS,
                cp_SparseVirtualFile_time_write_docstring
        },
        {
                "config",                (PyCFunction) cp_SparseVirtualFile_config,             METH_NOARGS,
                cp_SparseVirtualFile_config_docstring
        },
        {
                "time_read",             (PyCFunction) cp_SparseVirtualFile_time_read,          METH_NOARGS,
                cp_SparseVirtualFile_time_read_docstring
        },
        {       "__getstate__",          (PyCFunction) cp_SparseVirtualFile___getstate__,       METH_NOARGS,
                "Return the state for pickling."
        },
        {       "__setstate__",          (PyCFunction) cp_SparseVirtualFile___setstate__,       METH_O,
                "Set the state from a pickled object."
        },
        {NULL, NULL, 0, NULL}  /* Sentinel */
};

// clang-format off
// @formatter:off
PyDoc_STRVAR(
        svfs_cSVF_doc,
        "This class implements a Sparse Virtual File (SVF)."
        " This is an in-memory file that has fragments of a real file."
        " It has read/write operations and can describe what file fragments are needed, if any, before any read operation."
        "\n\n"
        "The constructor takes a string as an ID and optionally:\n"
        " - A file modification time as a float (default 0.0)."
        " This can be used for checking if the actual file might been changed which might invalidate the SVF.\n"
        " - ``overwrite_on_exit``, a boolean that will overwrite the memory on destruction (default ``False``)."
        " If ``True`` then ``clear()`` on a 1Mb SVF typically takes 35 µs, if ``False`` 1.5 µs.\n"
        " - ``compare_for_diff``, a boolean that will check that overlapping writes match (default ``True``)."
        " If ``True`` this adds about 25% time to an overlapping write but gives better chance of catching changes to the"
        " original file.\n"
        "\n\n"
        "For example::"
        "\n\n"
        "       import svfsc\n"
        "       \n"
        "       svf = svfsc.cSVF('some ID')\n"
        "       svf.write(12, b'ABCD')\n"
        "       svf.read(13, 2)  # Returns b'BC'\n"
        "       svf.need(10, 12)  # Returns ((10, 2), 16, 6)), the file positions and lengths the the SVF needs\n"
        "       svf.read(1024, 18)  # SVF raises an error as it has no data here.\n"
        "\n"
        "Signature:\n\n``svfsc.cSVF(id: str, mod_time: float = 0.0, overwrite_on_exit: bool = False, compare_for_diff: bool = True)``"
);
// @formatter:on
// clang-format on

static PyTypeObject svfsc_cSVF = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "svfsc.cSVF",
        .tp_basicsize = sizeof(cp_SparseVirtualFile),
        .tp_itemsize = 0,
        .tp_dealloc = (destructor) cp_SparseVirtualFile_dealloc,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_doc = svfs_cSVF_doc,
        .tp_methods = cp_SparseVirtualFile_methods,
        .tp_members = cp_SparseVirtualFile_members,
        .tp_init = (initproc) cp_SparseVirtualFile_init,
        .tp_new = cp_SparseVirtualFile_new,
};

#pragma mark - END SVF
