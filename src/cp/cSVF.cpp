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
 * SVF functions are named cp_SparseVirtualFile_svf_...
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
    double mod_time;
    static const char *kwlist[] = {"id", "mod_time", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sd", (char **) kwlist, &c_id, &mod_time)) {
        assert(PyErr_Occurred());
        return -1;
    }
    try {
        self->pSvf = new SVFS::SparseVirtualFile(c_id, mod_time);
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

static const char *cp_SparseVirtualFile_id_docstring = "Returns the ID of the Sparse Virtual File.";

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

static const char *cp_SparseVirtualFile_size_of_docstring = \
"Returns the estimate of total memory usage of the Sparse Virtual File.";

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

static const char *cp_SparseVirtualFile_num_bytes_docstring = \
"Returns the total number of file bytes held by the Sparse Virtual File.";

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

static const char *cp_SparseVirtualFile_num_blocks_docstring = \
"Returns the total number of blocks of data held by the Sparse Virtual File System.";

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

static const char *cp_SparseVirtualFile_svf_has_data_docstring = \
"Checks if the Sparse Virtual File of the ID has data at the given file_position and length." \
" This takes a file position and a length." \
" This returns True if the Sparse Virtual File has the data, False otherwise.";

static PyObject *
cp_SparseVirtualFile_svf_has_data(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
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

static const char *cp_SparseVirtualFile_svf_write_docstring = \
"Writes the data to the Sparse Virtual File of the given ID at file_position and length." \
" This takes a file position and data as a bytes object." \
" This will raise an IOError if the given data is different than that seen before and only" \
" new data up to this point will be written." \
" This will raise a RuntimeError if the data can not be written for any other reason";

static PyObject *
cp_SparseVirtualFile_svf_write(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    unsigned long long fpos = 0;
    PyObject * py_bytes_data = NULL;
    static const char *kwlist[] = {"file_position", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KS", (char **) kwlist, &fpos, &py_bytes_data)) {
        goto except;
    }
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

static const char *cp_SparseVirtualFile_svf_read_docstring = \
"Read the data from the Sparse Virtual File at file_position and length returning a bytes object." \
" This takes a file position and a length." \
" This will raise an IOError if any data is not present" \
" This will raise a RuntimeError if the data can not be read for any other reason";

static PyObject *
cp_SparseVirtualFile_svf_read(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL;
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = {"file_position", "length", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KK", (char **) kwlist, &fpos, &len)) {
        goto except;
    }
    // Create a bytes object
    ret = PyBytes_FromStringAndSize(NULL, len);
    try {
        self->pSvf->read(fpos, len, PyBytes_AS_STRING(ret));
    } catch (const SVFS::ExceptionSparseVirtualFileRead &err) {
        PyErr_Format(PyExc_IOError, "%s: Can not read from a SVF. ERROR: %s",
                     __FUNCTION__, err.message().c_str());
        goto except;
    } catch (const SVFS::ExceptionSparseVirtualFile &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: Fatal error reading from a SVF. ERROR: %s",
                     __FUNCTION__, err.message().c_str());
        goto except;
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

static const char *cp_SparseVirtualFile_svf_need_docstring = \
"Given a file_position and length this returns a ordered list [(file_position, length), ...] of seek/read" \
" instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will succeed." \
"\nUsage:\n" \
"    if not svf.has(file_position, length):\n" \
"        for seek, read in svf.need(file_position, length):\n" \
"            # Somehow get the data at all seek/read positions...\n" \
"            svf.write(seek, data, read)\n" \
"    return svf.read(file_position, length):\n" \
;

static PyObject *
cp_SparseVirtualFile_svf_need(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // PyListObject
    PyObject * list_item = NULL; // PyTupleObject
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = {"file_position", "length", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KK", (char **) kwlist, &fpos, &len)) {
        goto except;
    }
    try {
        SVFS::t_seek_read seek_read = self->pSvf->need(fpos, len);
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
t_seek_read blocks() const noexcept;

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

static const char *cp_SparseVirtualFile_svf_blocks_docstring = \
"This returns a ordered tuple ((file_position, length), ...) of all the blocks held by the SVF.";

static PyObject *
cp_SparseVirtualFile_svf_blocks(cp_SparseVirtualFile *self) {
    ASSERT_FUNCTION_ENTRY_SVF(pSvf);

    PyObject * ret = NULL; // PyTupleObject
    PyObject * insert_item = NULL; // PyTupleObject
    try {
        SVFS::t_seek_read seek_read = self->pSvf->blocks();
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
cp_SparseVirtualFile_svf_##method_name(cp_SparseVirtualFile *self) { \
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

static const char *cp_SparseVirtualFile_svf_size_of_docstring = \
"Returns the best guess of total memory usage used by the Sparse Virtual File.";

SVFS_SVF_METHOD_SIZE_T_WRAPPER(size_of);

static const char *cp_SparseVirtualFile_svf_num_bytes_docstring = \
"Returns the number of bytes of data held by the Sparse Virtual File.";

SVFS_SVF_METHOD_SIZE_T_WRAPPER(num_bytes);

static const char *cp_SparseVirtualFile_svf_num_blocks_docstring = \
"Returns the number of data blocks held by the Sparse Virtual File.";

SVFS_SVF_METHOD_SIZE_T_WRAPPER(num_blocks);

static const char *cp_SparseVirtualFile_svf_file_mod_time_matches_docstring = \
"Returns True if the file modification time of the Sparse Virtual File matches the given time as a float.";

static PyObject *
cp_SparseVirtualFile_svf_file_mod_time_matches(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) {
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

static const char *cp_SparseVirtualFile_file_mod_time_docstring = \
"Returns the file modification time as a float in UNIX time of the Sparse Virtual File identified by the given id.";

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

static const char *cp_SparseVirtualFile_svf_count_write_docstring = \
"Returns the count of write operations on the Sparse Virtual File.";

SVFS_SVF_METHOD_SIZE_T_WRAPPER(count_write);

static const char *cp_SparseVirtualFile_svf_count_read_docstring = \
"Returns the count of read operations on the Sparse Virtual File.";

SVFS_SVF_METHOD_SIZE_T_WRAPPER(count_read);

static const char *cp_SparseVirtualFile_svf_bytes_write_docstring = \
"Returns the count of the number of bytes writen to the Sparse Virtual File.";

SVFS_SVF_METHOD_SIZE_T_WRAPPER(bytes_write);

static const char *cp_SparseVirtualFile_svf_bytes_read_docstring = \
"Returns the count of the number of bytes read from the Sparse Virtual File.";

SVFS_SVF_METHOD_SIZE_T_WRAPPER(bytes_read);

// This macro is for functions that return a datetime type such as time_write, time_read.
#define SVFS_SVF_METHOD_DATETIME_WRAPPER(method_name) static PyObject * \
cp_SparseVirtualFile_svf_##method_name(cp_SparseVirtualFile *self, PyObject *args, PyObject *kwargs) { \
    ASSERT_FUNCTION_ENTRY_SVF(pSvf); \
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

static const char *cp_SparseVirtualFile_svf_time_write_docstring = \
"Returns the timestamp of the last write to the Sparse Virtual File as a datetime.datetime" \
" or None if no read has taken place.";
//SVFS_SVF_METHOD_DATETIME_WRAPPER(time_write)

static PyObject *
cp_SparseVirtualFile_svf_time_write(cp_SparseVirtualFile *self) {
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


static const char *cp_SparseVirtualFile_svf_time_read_docstring = \
"Returns the timestamp of the last read from the Sparse Virtual File as a datetime.datetime." \
" or None if no read has taken place.";
//SVFS_SVF_METHOD_DATETIME_WRAPPER(time_read)

static PyObject *
cp_SparseVirtualFile_svf_time_read(cp_SparseVirtualFile *self) {
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

// END: SVF functions
#pragma mark END: SVF functions

static PyMemberDef cp_SparseVirtualFile_members[] = {
//        {"first", T_OBJECT_EX, offsetof(CustomObject, first), 0,
//                "first name"},
        {NULL, 0, 0, 0, NULL}  /* Sentinel */
};

static PyMethodDef cp_SparseVirtualFile_methods[] = {
        {
                "id",                    (PyCFunction) cp_SparseVirtualFile_id,              METH_NOARGS,
                cp_SparseVirtualFile_id_docstring
        },
        {
                "size_of",               (PyCFunction) cp_SparseVirtualFile_size_of,         METH_NOARGS,
                cp_SparseVirtualFile_size_of_docstring
        },
        {
                "num_bytes",             (PyCFunction) cp_SparseVirtualFile_num_bytes,       METH_NOARGS,
                cp_SparseVirtualFile_num_bytes_docstring
        },
        {
                "num_blocks",            (PyCFunction) cp_SparseVirtualFile_num_blocks,      METH_NOARGS,
                cp_SparseVirtualFile_num_blocks_docstring
        },
        {
                "has_data",              (PyCFunction) cp_SparseVirtualFile_svf_has_data,    METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_has_data_docstring
        },
        {
                "write",                 (PyCFunction) cp_SparseVirtualFile_svf_write,       METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_write_docstring
        },
        {
                "read",                  (PyCFunction) cp_SparseVirtualFile_svf_read,        METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_read_docstring
        },
        {
                "need",                  (PyCFunction) cp_SparseVirtualFile_svf_need,        METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_need_docstring
        },
        // ---- Meta information about the specific SVF ----
        {
                "blocks",                (PyCFunction) cp_SparseVirtualFile_svf_blocks,      METH_NOARGS,
                cp_SparseVirtualFile_svf_blocks_docstring
        },
        {
                "size_of",               (PyCFunction) cp_SparseVirtualFile_svf_size_of,     METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_size_of_docstring
        },
        {
                "num_bytes",             (PyCFunction) cp_SparseVirtualFile_svf_num_bytes,   METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_num_bytes_docstring
        },
        {
                "num_blocks",            (PyCFunction) cp_SparseVirtualFile_svf_num_blocks,  METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_num_blocks_docstring
        },
        {
                "file_mod_time_matches", (PyCFunction) cp_SparseVirtualFile_svf_file_mod_time_matches,
                                                                                             METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_file_mod_time_matches_docstring
        },
        // ---- Attribute access ----
        {
                "file_mod_time",         (PyCFunction) cp_SparseVirtualFile_file_mod_time,   METH_NOARGS,
                cp_SparseVirtualFile_file_mod_time_docstring
        },
        {
                "count_write",           (PyCFunction) cp_SparseVirtualFile_svf_count_write, METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_count_write_docstring
        },
        {
                "count_read",            (PyCFunction) cp_SparseVirtualFile_svf_count_read,  METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_count_read_docstring
        },
        {
                "bytes_write",           (PyCFunction) cp_SparseVirtualFile_svf_bytes_write, METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_bytes_write_docstring
        },
        {
                "bytes_read",            (PyCFunction) cp_SparseVirtualFile_svf_bytes_read,  METH_VARARGS |
                                                                                             METH_KEYWORDS,
                        cp_SparseVirtualFile_svf_bytes_read_docstring
        },
        {
                "time_write",            (PyCFunction) cp_SparseVirtualFile_svf_time_write,  METH_NOARGS,
                cp_SparseVirtualFile_svf_time_write_docstring
        },
        {
                "time_read",             (PyCFunction) cp_SparseVirtualFile_svf_time_read,   METH_NOARGS,
                cp_SparseVirtualFile_svf_time_read_docstring
        },
        {NULL, NULL, 0, NULL}  /* Sentinel */
};

static PyTypeObject svfs_SVF = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "svfs.SVF",
        .tp_basicsize = sizeof(cp_SparseVirtualFile),
        .tp_itemsize = 0,
        .tp_dealloc = (destructor) cp_SparseVirtualFile_dealloc,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_doc = "This class implements a Sparse Virtual File.",
        .tp_methods = cp_SparseVirtualFile_methods,
        .tp_members = cp_SparseVirtualFile_members,
        .tp_init = (initproc) cp_SparseVirtualFile_init,
        .tp_new = cp_SparseVirtualFile_new,
};

#pragma mark - END SVF

// Module initialisation
#pragma mark Module initialisation

static PyModuleDef svfmodule = {
        PyModuleDef_HEAD_INIT,
        .m_name = "svf",
        .m_doc = \
        "This module contains Sparse Virtual File class."
        "\n"
        "A Sparse Virtual File (SVF) is one where some data from the actual file is held in memory at the specific"
        " file locations as the original file."
        " Data can be written to an SVF, if the data differs from that existing an error will be raised."
        " Data can be read from an SVF, if the SVF does not have the data an error will be raised."
        " Before any read() the SVF can describe what, if any, data is missing and the user can obtain and write that"
        " data to the SVF before reading.",
        .m_size = -1,
};

PyMODINIT_FUNC
PyInit_svf(void) {
    PyObject * m = NULL;
    if (PyType_Ready(&svfs_SVF) < 0) {
        return NULL;
    }

    m = PyModule_Create(&svfmodule);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&svfs_SVF);
    PyModule_AddObject(m, "SVF", (PyObject *) &svfs_SVF);

    PyDateTime_IMPORT;
    return m;
}

// END: Module initialisation
#pragma mark END: Module initialisation
