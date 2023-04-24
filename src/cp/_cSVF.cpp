#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"

#include <ctime>
#include <memory>

#include "svf.h"
#include "svfs_util.h"

/* TODO: Implement pickling an SVF.
 * TODO: Implement the Buffer Protocol rather than returning a copy of the bytes? Look for PyBytes_FromStringAndSize().
 * */

/**
 * Naming convention:
 *
 * SVF functions are named cp_SparseVirtualFile_...
 */

typedef struct {
    PyObject_HEAD
    SVFS::SparseVirtualFile *pSvf;
} cp_SparseVirtualFile;

// Function entry point test macro.
// After construction we expect this invariant at the entry to each function.
// The cast is necessary when used with functions that take a SVFS as a PyObject* such as
// cp_SparseVirtualFile_mapping_length
#define ASSERT_FUNCTION_ENTRY_SVF(member) do { \
    assert(self); \
    assert(((cp_SparseVirtualFile *)self)->member); \
    assert(! PyErr_Occurred()); \
} while (0)


// Construction and destruction
#pragma mark Construction and destruction

static PyObject *
cp_SparseVirtualFile_new(PyTypeObject *type, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds)) {
    assert(!PyErr_Occurred());
    cp_SparseVirtualFile *self;
    self = (cp_SparseVirtualFile *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->pSvf = nullptr;
    }
//    PyObject_Print((PyObject *)self, stdout);
//    fprintf(stdout, "cp_SparseVirtualFile_new() self %p\n", (void *)self);
    assert(!PyErr_Occurred());
    return (PyObject *) self;
}

static int
cp_SparseVirtualFile_init(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    assert(!PyErr_Occurred());

    char *c_id = NULL;
    double mod_time = 0.0;
    static const char *kwlist[] = {"id", "mod_time", "overwrite_on_exit", "compare_for_diff", NULL};
    SVFS::tSparseVirtualFileConfig config;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dpp", (char **) kwlist, &c_id, &mod_time,
                                     &config.overwrite_on_exit, &config.compare_for_diff)) {
        assert(PyErr_Occurred());
        return -1;
    }
    try {
        self->pSvf = new SVFS::SparseVirtualFile(c_id, mod_time, config);
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return -1;
    }
//    fprintf(stdout, "cp_SparseVirtualFile_init() self->pSvf %p\n", (void *)self->pSvf);
    assert(!PyErr_Occurred());
    return 0;
}

static void
cp_SparseVirtualFile_dealloc(cp_SparseVirtualFile *self) {
    delete self->pSvf;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

// END: Construction and destruction
#pragma mark END: Construction and destruction

/* If you are interested this is a way that you can trace the input. */
#define TRACE_SELF_ARGS_KWARGS \
    PyObject_Print(self, stdout, Py_PRINT_RAW); \
    fprintf(stdout, "\n"); \
    PyObject_Print(args, stdout, Py_PRINT_RAW); \
    fprintf(stdout, "\n"); \
    PyObject_Print(kwargs, stdout, Py_PRINT_RAW); \
    fprintf(stdout, "\n");

// SVFS functions
#pragma mark SVF functions

static const char *cp_SparseVirtualFile_id_docstring = (
        "Returns the ID of the Sparse Virtual File."
        "\n\nSignature:\n\n``id() -> str:``"
);

static PyObject *
cp_SparseVirtualFile_id(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    ret = PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, self->pSvf->id().c_str(), self->pSvf->id().size());
    if (!ret) {
        PyErr_Format(PyExc_RuntimeError, "%s: Can create id for %s", __FUNCTION__, self->pSvf->id().c_str());
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

static const char *cp_SparseVirtualFile_size_of_docstring = (
        "Returns the estimate of total memory usage of the Sparse Virtual File."
        "\n\nSignature:\n\n``size_of() -> int:``"
);

static PyObject *
cp_SparseVirtualFile_size_of(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);
    try {
        return PyLong_FromLong(self->pSvf->size_of());
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return NULL;
    }
}

static const char *cp_SparseVirtualFile_num_bytes_docstring = (
        "Returns the total number of file bytes held by the Sparse Virtual File."
        "\n\nSignature:\n\n``num_bytes() -> int:``"
);

static PyObject *
cp_SparseVirtualFile_num_bytes(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);
    try {
        return PyLong_FromLong(self->pSvf->num_bytes());
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return NULL;
    }
}

static const char *cp_SparseVirtualFile_num_blocks_docstring = (
        "Returns the total number of blocks of data held by the Sparse Virtual File System."
        "\n\nSignature:\n\n``num_blocks() -> int:``"
);

static PyObject *
cp_SparseVirtualFile_num_blocks(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);
    try {
        return PyLong_FromLong(self->pSvf->num_blocks());
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: FATAL caught std::exception %s", __FUNCTION__, err.what());
        return NULL;
    }
}

static const char *cp_SparseVirtualFile_has_data_docstring = (
        "Checks if the Sparse Virtual File of the ID has data at the given ``file_position`` and ``length``."
        " This returns True if the Sparse Virtual File has the data, False otherwise."
        "\n\nSignature:\n\n``has_data(file_position: int, length: int) -> bool:``"
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

static const char *cp_SparseVirtualFile_write_docstring = (
        "Writes the data to the Sparse Virtual File of the given ID at ``file_position`` and ``data`` as a ``bytes`` object."
        " This will raise an ``IOError`` if ``self.compare_for_diff`` is True and given data is different than"
        " that seen before and only new data up to this point will be written."
        " If the ``byte`` data is empty nothing will be done."
        " This will raise a RuntimeError if the data can not be written for any other reason"
        "\n\nSignature:\n\n``write(file_position: int, data: bytes) -> None:``"
);

static PyObject *
cp_SparseVirtualFile_write(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    unsigned long long fpos = 0;
    PyObject * py_bytes_data = NULL;
    static const char *kwlist[] = {"file_position", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KS", (char **) kwlist, &fpos, &py_bytes_data)) {
        goto except;
    }
    if (PyBytes_GET_SIZE(py_bytes_data) > 0) {
        try {
            self->pSvf->write(fpos, PyBytes_AS_STRING(py_bytes_data), PyBytes_Size(py_bytes_data));
        } catch (const SVFS::ExceptionSparseVirtualFileDiff &err) {
            PyErr_Format(PyExc_IOError,
                         "%s: Can not write to a SVF as the given data is different from what is there. ERROR: %s",
                         __FUNCTION__, err.message().c_str());
            goto except;
        } catch (const SVFS::ExceptionSparseVirtualFile &err) {
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

static const char *cp_SparseVirtualFile_read_docstring = (
        "Read the data from the Sparse Virtual File at ``file_position`` and ``length`` returning a ``bytes`` object."
        " This takes a file position and a length."
        " This will raise an ``IOError`` if any data is not present"
        " This will raise a ``RuntimeError`` if the data can not be read for any other reason"
        "\n\nSignature:\n\n``read(file_position: int, length: int) -> bytes:``"
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
    } catch (const SVFS::ExceptionSparseVirtualFileRead &err) {
        PyErr_Format(PyExc_IOError, "%s()#%d: Can not read from a SVF. ERROR: %s",
                     __FUNCTION__, __LINE__, err.message().c_str());
        Py_DECREF(ret);
        return NULL;
    } catch (const SVFS::ExceptionSparseVirtualFile &err) {
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


static const char *cp_SparseVirtualFile_erase_docstring = (
        "Erase the data from the Sparse Virtual File at the given ``file_position`` which must be the beginning of a block."
        " This will raise an ``IOError`` if a block is not present at that file position."
        " This will raise a ``RuntimeError`` if the data can not be read for any other reason"
        "\n\nSignature:\n\n``erase(file_position: int) -> None:``"
);

static PyObject *
cp_SparseVirtualFile_erase(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    unsigned long long fpos = 0;
    static const char *kwlist[] = {"file_position", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "K", (char **) kwlist, &fpos)) {
        return NULL;
    }
    try {
        self->pSvf->erase(fpos);
    } catch (const SVFS::ExceptionSparseVirtualFileErase &err) {
        PyErr_Format(PyExc_IOError, "%s()#%d: Can not erase from a SVF. ERROR: %s",
                     __FUNCTION__, __LINE__, err.message().c_str());
        return NULL;
    } catch (const SVFS::ExceptionSparseVirtualFile &err) {
        PyErr_Format(PyExc_RuntimeError, "%s()#%d: Fatal error reading from a SVF. ERROR: %s",
                     __FUNCTION__, __LINE__, err.message().c_str());
        return NULL;
    } catch (const std::exception &err) {
        PyErr_Format(PyExc_RuntimeError, "%s()#%d: FATAL caught std::exception %s", __FUNCTION__, __LINE__,
                     err.what());
        return NULL;
    }
    Py_RETURN_NONE;
}

static const char *cp_SparseVirtualFile_need_docstring = (
        "Given a file_position and length this returns a ordered list ``[(file_position, length), ...]`` of seek/read"
        " instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will succeed."
        " If greedy_length is > 0 then, if possible, blocks will be coalesced to reduce the size of the return value."
        "\nUsage::\n\n"
        "    if not svf.has_data(position, length):\n"
        "        for position, read_length in svf.need(position, length):\n"
        "            # Somehow get data as a bytes object at position...\n"
        "            svf.write(fposition, data)\n"
        "    return svf.read(file_position, length):\n"
        "\n\nSignature:\n\n``need(file_position: int, length: int, greedy_length: int = 0) -> typing.Tuple[typing.Tuple[int, int], ...]:``"
);

static PyObject *
cp_SparseVirtualFile_need(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // PyListObject
    PyObject * list_item = NULL; // PyTupleObject
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    unsigned long long greedy_len = 0;
    static const char *kwlist[] = {"file_position", "length", "greedy_length", NULL};

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

static const char *cp_SparseVirtualFile_blocks_docstring = (
        "This returns a ordered tuple ``((file_position, length), ...)``"
        " of the shape of the blocks held by the SVF in file position order."
        "\n\nSignature:\n\n``blocks() -> typing.Tuple[typing.Tuple[int, int], ...]:``"
);

static PyObject *
cp_SparseVirtualFile_blocks(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // PyTupleObject
    PyObject * insert_item = NULL; // PyTupleObject
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

// This macro is for functions that return a size_t type such as count_write, count_read, bytes_write, bytes_read.
#define SVFS_SVF_METHOD_SIZE_T_WRAPPER(method_name) static PyObject * \
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

static const char *cp_SparseVirtualFile_file_mod_time_matches_docstring = (
        "Returns True if the file modification time of the Sparse Virtual File matches the given time as a float."
        "\n\nSignature:\n\n``file_mod_time_matches(file_mod_time: float) -> bool:``"
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

static const char *cp_SparseVirtualFile_file_mod_time_docstring = (
        "Returns the file modification time as a float in UNIX time of the Sparse Virtual File."
        "\n\nSignature:\n\n``file_mod_time() -> float:``"
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

static const char *cp_SparseVirtualFile_count_write_docstring = (
        "Returns the count of write operations on the Sparse Virtual File."
        "\n\nSignature:\n\n``count_write() -> int:``"
);

SVFS_SVF_METHOD_SIZE_T_WRAPPER(count_write);

static const char *cp_SparseVirtualFile_count_read_docstring = (
        "Returns the count of read operations on the Sparse Virtual File."
        "\n\nSignature:\n\n``count_read() -> int:``"
);

SVFS_SVF_METHOD_SIZE_T_WRAPPER(count_read);

static const char *cp_SparseVirtualFile_bytes_write_docstring = (
        "Returns the count of the number of bytes writen to the Sparse Virtual File."
        "\n\nSignature:\n\n``bytes_write() -> int:``"
);

SVFS_SVF_METHOD_SIZE_T_WRAPPER(bytes_write);

static const char *cp_SparseVirtualFile_bytes_read_docstring = (
        "Returns the count of the number of bytes read from the Sparse Virtual File."
        "\n\nSignature:\n\n``bytes_read() -> int:``"
);

SVFS_SVF_METHOD_SIZE_T_WRAPPER(bytes_read);

// NOTE: time_read and time_write functions are very similar.

static const char *cp_SparseVirtualFile_time_write_docstring = (
        "Returns the timestamp of the last write to the Sparse Virtual File as a ``datetime.datetime``"
        " or ``None`` if no read has taken place."
        "\n\nSignature:\n\n``time_write() -> datetime.datetime:``"
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


static const char *cp_SparseVirtualFile_time_read_docstring = (
        "Returns the timestamp of the last read from the Sparse Virtual File as a ``datetime.datetime``"
        " or ``None`` if no read has taken place."
        "\n\nSignature:\n\n``time_read() -> datetime.datetime:``"
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
                "id",                    (PyCFunction) cp_SparseVirtualFile_id,            METH_NOARGS,
                cp_SparseVirtualFile_id_docstring
        },
        {
                "size_of",               (PyCFunction) cp_SparseVirtualFile_size_of,       METH_NOARGS,
                cp_SparseVirtualFile_size_of_docstring
        },
        {
                "num_bytes",             (PyCFunction) cp_SparseVirtualFile_num_bytes,     METH_NOARGS,
                cp_SparseVirtualFile_num_bytes_docstring
        },
        {
                "num_blocks",            (PyCFunction) cp_SparseVirtualFile_num_blocks,    METH_NOARGS,
                cp_SparseVirtualFile_num_blocks_docstring
        },
        {
                "has_data",              (PyCFunction) cp_SparseVirtualFile_has_data,      METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_has_data_docstring
        },
        {
                "write",                 (PyCFunction) cp_SparseVirtualFile_write,         METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_write_docstring
        },
        {
                "read",                  (PyCFunction) cp_SparseVirtualFile_read,          METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_read_docstring
        },
        {
                "erase",                 (PyCFunction) cp_SparseVirtualFile_erase,         METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_erase_docstring
        },
        {
                "need",                  (PyCFunction) cp_SparseVirtualFile_need,          METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_need_docstring
        },
        // ---- Meta information about the specific SVF ----
        {
                "blocks",                (PyCFunction) cp_SparseVirtualFile_blocks,        METH_NOARGS,
                cp_SparseVirtualFile_blocks_docstring
        },
        {
                "file_mod_time_matches", (PyCFunction) cp_SparseVirtualFile_file_mod_time_matches,
                                                                                           METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_file_mod_time_matches_docstring
        },
        // ---- Attribute access ----
        {
                "file_mod_time",         (PyCFunction) cp_SparseVirtualFile_file_mod_time, METH_NOARGS,
                cp_SparseVirtualFile_file_mod_time_docstring
        },
        {
                "count_write",           (PyCFunction) cp_SparseVirtualFile_count_write,   METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_count_write_docstring
        },
        {
                "count_read",            (PyCFunction) cp_SparseVirtualFile_count_read,    METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_count_read_docstring
        },
        {
                "bytes_write",           (PyCFunction) cp_SparseVirtualFile_bytes_write,   METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_bytes_write_docstring
        },
        {
                "bytes_read",            (PyCFunction) cp_SparseVirtualFile_bytes_read,    METH_VARARGS |
                                                                                           METH_KEYWORDS,
                        cp_SparseVirtualFile_bytes_read_docstring
        },
        {
                "time_write",            (PyCFunction) cp_SparseVirtualFile_time_write,    METH_NOARGS,
                cp_SparseVirtualFile_time_write_docstring
        },
        {
                "time_read",             (PyCFunction) cp_SparseVirtualFile_time_read,     METH_NOARGS,
                cp_SparseVirtualFile_time_read_docstring
        },
        {       "__getstate__",          (PyCFunction) cp_SparseVirtualFile___getstate__,  METH_NOARGS,
                "Return the state for pickling"
        },
        {       "__setstate__",          (PyCFunction) cp_SparseVirtualFile___setstate__,  METH_O,
                "Set the state from a pickle"
        },
        {NULL, NULL, 0, NULL}  /* Sentinel */
};

// clang-format off
// @formatter:off
static const char *svfs_cSVF_doc = PyDoc_STR(
    "This class implements a Sparse Virtual File (SVF)."
    " This is an in-memory file that has fragments of a real file."
    " It has read/write operations and can describe what file fragments are needed, if any, before any read operation."
    "\n\n"
    "The constructor takes a string as an ID and an optionally:\n"
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
    "       import svfs\n"
    "       \n"
    "       svf = svfs.cSVF('some ID')\n"
    "       svf.write(12, b'ABCD')\n"
    "       svf.read(13, 2)  # Returns b'BC'\n"
    "       svf.need(10, 12)  # Returns ((10, 2), 16, 6)), the file positions and lengths the the SVF needs\n"
    "       svf.read(1024, 18)  # SVF raises an error as it has no data here.\n"
    "\n"
    "Signature:\n\n``svfs.cSVF(id: str, mod_time: float = 0.0, overwrite_on_exit: bool = False, compare_for_diff: bool = True)``"
);
// @formatter:on
// clang-format on

static PyTypeObject svfs_cSVF = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "svfs.cSVF",
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
