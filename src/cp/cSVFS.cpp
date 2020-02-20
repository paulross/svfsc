#include <Python.h>
#include "structmember.h"
#include "datetime.h"

#include <ctime>
#include <memory>

#include "svfs.h"
#include "svfs_util.h"

typedef struct {
    PyObject_HEAD
    SVFS::SparseVirtualFileSystem *p_svfs;
} cp_SparseVirtualFileSystem;

// After construction we expect this invariant at the entry to each function.
// The cast is necessary when used with functions that take a SVFS as a PyObject* such as
// cp_SparseVirtualFileSystem_mapping_length
#define ASSERT_FUNCTION_ENTRY_SVFS(member) do { \
    assert(self); \
    assert(((cp_SparseVirtualFileSystem *)self)->member); \
    assert(! PyErr_Occurred()); \
} while (0)

static PyObject *
cp_SparseVirtualFileSystem_new(PyTypeObject *type, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    cp_SparseVirtualFileSystem *self;
    self = (cp_SparseVirtualFileSystem *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->p_svfs = nullptr;
    }
    return (PyObject *) self;
}

static int
cp_SparseVirtualFileSystem_init(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwds)
{
    static const char *kwlist[] = {"coalesce", "overwrite", NULL};
    PyObject *coalesce = NULL;
    PyObject *overwrite = NULL;

    // Parse args/kwargs for coalesce (int), overwrite(bool)
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|ip", const_cast<char**>(kwlist), &coalesce, &overwrite)) {
        return -1;
    }
    self->p_svfs = new SVFS::SparseVirtualFileSystem(PyLong_AsLong(coalesce), overwrite == Py_True);
    return 0;
}

static void
cp_SparseVirtualFileSystem_dealloc(cp_SparseVirtualFileSystem *self)
{
    delete self->p_svfs;
    Py_TYPE(self)->tp_free((PyObject *) self);
}


/* If you are interested this is a way that you can trace the input.
 PyObject_Print(self, stdout, 0);
 fprintf(stdout, "\n");
 PyObject_Print(args, stdout, 0);
 fprintf(stdout, "\n");
 PyObject_Print(kwargs, stdout, 0);
 fprintf(stdout, "\n");
 * End trace */

static const char *cp_SparseVirtualFileSystem_keys_docstring = \
"Returns the IDs of all the Sparse Virtual Files.";

static PyObject *
cp_SparseVirtualFileSystem_keys(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    Py_ssize_t index = 0;
    PyObject *key = NULL;
    // All items set to NULL on construction.
    PyObject *ret = PyList_New(self->p_svfs->size());
    if (! ret) {
        PyErr_Format(PyExc_RuntimeError, "%s: Can create list of size %d", __FUNCTION__, self->p_svfs->size());
        goto except;
    }
    for(const auto& cpp_key: self->p_svfs->keys()) {
        key = py_unicode_from_std_string(cpp_key);
        if (! key) {
            PyErr_Format(PyExc_RuntimeError, "%s: Can create key for %s", __FUNCTION__, cpp_key.c_str());
            goto except;
        }
        // No error checking for this line. Steals reference.
        PyList_SET_ITEM(ret, index, key);
        key = NULL; // Safety habit with stolen reference.
        ++index;
    }
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
    except:
    assert(PyErr_Occurred());
    if (ret) {
        for(Py_ssize_t i = 0; i < PyList_Size(ret); ++i) {
            Py_XDECREF(PyList_GET_ITEM(ret, i));
        }
    }
    Py_XDECREF(ret);
    ret = NULL;
    finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_insert_docstring = \
"Removes a Sparse Virtual File of ID and Unix modification time.";

static PyObject *
cp_SparseVirtualFileSystem_insert(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    double mod_time;
    static const char *kwlist[] = { "id", "mod_time", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Sd", (char **)kwlist, &id, &mod_time)) {
        goto except;
    }
    try {
        self->p_svfs->insert(py_unicode_to_std_string(id), mod_time);
    } catch (const SVFS::ExceptionSparseVirtualFileSystemInsert &err) {
        PyErr_Format(PyExc_RuntimeError, "%s: Can not insert a new SVF. ERROR: %s", __FUNCTION__, err.message().c_str());
        goto except;
    }
    Py_INCREF(Py_None);
    ret = Py_None;
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_remove_docstring = \
"Removes a Sparse Virtual File of ID. Will raise an IndexError if the ID is absent.";

static PyObject *
cp_SparseVirtualFileSystem_remove(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    static const char *kwlist[] = { "id", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "S", (char **)kwlist, &id)) {
        goto except;
    }
    try {
        self->p_svfs->remove(py_unicode_to_std_string(id));
    } catch (const SVFS::ExceptionSparseVirtualFileSystemRemove &err) {
        PyErr_Format(PyExc_IndexError, "%s: Can not remove a SVF. ERROR: %s", __FUNCTION__, err.message().c_str());
        goto except;
    }
    Py_INCREF(Py_None);
    ret = Py_None;
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_has_docstring = \
"Returns True if the SVF of the ID is in the SVFS.";

static PyObject *
cp_SparseVirtualFileSystem_has(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    static const char *kwlist[] = { "id", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "S", (char **)kwlist, &id)) {
        goto except;
    }
    if (self->p_svfs->has(py_unicode_to_std_string(id))) {
        Py_INCREF(Py_True);
        ret = Py_True;
    } else {
        Py_INCREF(Py_False);
        ret = Py_False;
    }
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_total_size_of_docstring = \
"Returns the estimate of total memory usage of the Sparse Virtual File System.";

static PyObject *
cp_SparseVirtualFileSystem_total_size_of(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    return PyLong_FromLong(self->p_svfs->size_of());
}

static const char *cp_SparseVirtualFileSystem_total_bytes_docstring = \
"Returns the total number of file bytes held by the Sparse Virtual File System.";

static PyObject *
cp_SparseVirtualFileSystem_total_bytes(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    return PyLong_FromLong(self->p_svfs->num_bytes());
}

static const char *cp_SparseVirtualFileSystem_total_blocks_docstring = \
"Returns the total number of blocks held by the Sparse Virtual File System.";

static PyObject *
cp_SparseVirtualFileSystem_total_blocks(cp_SparseVirtualFileSystem *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    return PyLong_FromLong(self->p_svfs->num_blocks());
}

static const char *cp_SparseVirtualFileSystem_svf_has_data_docstring = \
"Returns True if the SVF of the ID has data at file_position and length.";

static PyObject *
cp_SparseVirtualFileSystem_svf_has_data(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    std::string c_id;
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = { "id", "file_position", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "SKK", (char **)kwlist, &id, &fpos, &len)) {
        goto except;
    }
    c_id = py_unicode_to_std_string(id);
    if (self->p_svfs->has(c_id)) {
        const SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id);
        if (svf.has(fpos, len)) {
            Py_INCREF(Py_True);
            ret = Py_True;
        } else {
            Py_INCREF(Py_False);
            ret = Py_False;
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str());
        goto except;
    }
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_svf_write_docstring = \
"Writes the data to the SVF at file_position and length.";

static PyObject *
cp_SparseVirtualFileSystem_svf_write(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    std::string c_id;
    unsigned long long fpos = 0;
    PyObject *py_bytes_data = NULL;
    unsigned long long len = 0;
    static const char *kwlist[] = { "id", "file_position", "data", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "SKy*K", (char **)kwlist, &id, &fpos, &py_bytes_data, &len)) {
        goto except;
    }
    c_id = py_unicode_to_std_string(id);
    if (self->p_svfs->has(c_id)) {
        SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id);
        try {
            svf.write(fpos, PyBytes_AS_STRING(py_bytes_data), len);
        } catch (const SVFS::ExceptionSparseVirtualFile &err) {
            PyErr_Format(PyExc_RuntimeError, "%s: Can not write to a SVF. ERROR: %s", __FUNCTION__, err.message().c_str());
            goto except;
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str());
        goto except;
    }
    Py_INCREF(Py_None);
    ret = Py_None;
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_svf_read_docstring = \
"Read the data to the SVF at file_position and length returning a bytes object.";

static PyObject *
cp_SparseVirtualFileSystem_svf_read(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    std::string c_id;
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = { "id", "file_position", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "SKK", (char **)kwlist, &id, &fpos, &len)) {
        goto except;
    }
    c_id = py_unicode_to_std_string(id);
    if (self->p_svfs->has(c_id)) {
        SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id);
        // Create a bytes object
        ret = PyBytes_FromStringAndSize(NULL, len);
        try {
            svf.read(fpos, len, PyBytes_AS_STRING(ret));
        } catch (const SVFS::ExceptionSparseVirtualFile &err) {
            PyErr_Format(PyExc_RuntimeError, "%s: Can not read from a SVF. ERROR: %s", __FUNCTION__, err.message().c_str());
            goto except;
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str());
        goto except;
    }
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_svf_need_docstring = \
"Given a file_position and length this returns a ordered list [(file_position, length), ...] of seek/read" \
"instructions of data that is required to be written to the SVF so that a subsequent read will succeed.\n" \
"Usage:\n" \
"    if not svfs.has(identity, file_position, length:\n" \
"        for seek, read in svfs.need(file_position, length):\n" \
"            # Somehow get the data at seek/read\n" \
"            svfs.write(identity, seek, data, read)\n" \
"    return svfs.read(identity, file_position, length):\n" \
;

static PyObject *
cp_SparseVirtualFileSystem_svf_need(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL; // PyListObject
    PyObject *id = NULL; // PyUnicodeObject
    std::string c_id;
    PyObject *list_item = NULL; // PyTupleObject
    unsigned long long fpos = 0;
    unsigned long long len = 0;
    static const char *kwlist[] = { "id", "file_position", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "SKK", (char **)kwlist, &id, &fpos, &len)) {
        goto except;
    }
    c_id = py_unicode_to_std_string(id);
    if (self->p_svfs->has(c_id)) {
        SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id);
        SVFS::t_seek_read seek_read = svf.need(fpos, len);
        ret = PyList_New(seek_read.size());
        for (size_t i = 0; i < seek_read.size(); ++i) {
            list_item = Py_BuildValue("KK", seek_read[i].first, seek_read[i].second);
            if (! list_item) {
                PyErr_Format(PyExc_MemoryError, "%s: Can not create tuple", __FUNCTION__);
                goto except;
            }
            PyList_SET_ITEM(ret, i, list_item);
            list_item = NULL;
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str());
        goto except;
    }
    assert(! PyErr_Occurred());
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
cp_SparseVirtualFileSystem_svf_##method_name(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) { \
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs); \
    PyObject *ret = NULL; \
    PyObject *id = NULL; \
    std::string c_id; \
    static const char *kwlist[] = { "id", NULL}; \
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "S", (char **)kwlist, &id)) { \
        goto except; \
    } \
    c_id = py_unicode_to_std_string(id); \
    if (self->p_svfs->has(c_id)) { \
        const SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id); \
        ret = PyLong_FromLong(svf.method_name()); \
    } else { \
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str()); \
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

static const char *cp_SparseVirtualFileSystem_svf_blocks_docstring = \
"This returns a ordered list [(file_position, length), ...] of the blocks held by the SVF identified by the given id.";

static PyObject *
cp_SparseVirtualFileSystem_svf_blocks(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL; // PyListObject
    PyObject *id = NULL; // PyUnicodeObject
    std::string c_id;
    PyObject *list_item = NULL; // PyTupleObject
    static const char *kwlist[] = { "id", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "S", (char **)kwlist, &id)) {
        goto except;
    }
    c_id = py_unicode_to_std_string(id);
    if (self->p_svfs->has(c_id)) {
        SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id);
        SVFS::t_seek_read seek_read = svf.blocks();
        ret = PyList_New(seek_read.size());
        for (size_t i = 0; i < seek_read.size(); ++i) {
            list_item = Py_BuildValue("KK", seek_read[i].first, seek_read[i].second);
            if (! list_item) {
                PyErr_Format(PyExc_MemoryError, "%s: Can not create tuple", __FUNCTION__);
                goto except;
            }
            PyList_SET_ITEM(ret, i, list_item);
            list_item = NULL;
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str());
        goto except;
    }
    assert(! PyErr_Occurred());
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

static const char *cp_SparseVirtualFileSystem_svf_size_of_docstring = \
"Returns the best guess of total memory usage used by the SVF identified by the given id.";
SVFS_SVF_METHOD_SIZE_T_WRAPPER(size_of);

static const char *cp_SparseVirtualFileSystem_svf_num_bytes_docstring = \
"Returns the number of bytes of data held by the SVF identified by the given id.";
SVFS_SVF_METHOD_SIZE_T_WRAPPER(num_bytes);

static const char *cp_SparseVirtualFileSystem_svf_num_blocks_docstring = \
"Returns the number of data blocks held by the SVF identified by the given id.";
SVFS_SVF_METHOD_SIZE_T_WRAPPER(num_blocks);

static const char *cp_SparseVirtualFileSystem_svf_file_mod_time_matches_docstring = \
"Returns True if the SVF of the ID file modification time matches the time as a float.";

static PyObject *
cp_SparseVirtualFileSystem_svf_file_mod_time_matches(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    double file_mod_time;
    std::string c_id;
    static const char *kwlist[] = { "id", "file_mod_time", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "Sd", (char **)kwlist, &id, &file_mod_time)) {
        goto except;
    }
    c_id = py_unicode_to_std_string(id);
    if (self->p_svfs->has(c_id)) {
        SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id);
        if (svf.file_mod_time_matches(file_mod_time)) {
            Py_INCREF(Py_True);
            ret = Py_True;
        } else {
            Py_INCREF(Py_False);
            ret = Py_False;
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str());
        goto except;
    }
    assert(! PyErr_Occurred());
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

static const char *cp_SparseVirtualFileSystem_file_mod_time_docstring = \
"Returns the file modification time as a float in UNIX time of the SVF identified by the given id.";

// SVF: double file_mod_time() const noexcept { return m_file_mod_time; }
static PyObject *
cp_SparseVirtualFileSystem_file_mod_time(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);

    PyObject *ret = NULL;
    PyObject *id = NULL;
    std::string c_id;
    static const char *kwlist[] = { "id", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "S", (char **)kwlist, &id)) {
        goto except;
    }
    c_id = py_unicode_to_std_string(id);
    if (self->p_svfs->has(c_id)) {
        SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id);
        ret = PyFloat_FromDouble(svf.file_mod_time());
    } else {
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str());
        goto except;
    }
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    assert(PyErr_Occurred());
    Py_XDECREF(ret);
    ret = NULL;
finally:
    return ret;
}

static const char *cp_SparseVirtualFileSystem_svf_count_write_docstring = \
"Returns the count of write operations on the SVF identified by the given id.";
SVFS_SVF_METHOD_SIZE_T_WRAPPER(count_write);

static const char *cp_SparseVirtualFileSystem_svf_count_read_docstring = \
"Returns the count of read operations on the SVF identified by the given id.";
SVFS_SVF_METHOD_SIZE_T_WRAPPER(count_read);

static const char *cp_SparseVirtualFileSystem_svf_bytes_write_docstring = \
"Returns the count of the number of bytes writen to the SVF identified by the given id.";
SVFS_SVF_METHOD_SIZE_T_WRAPPER(bytes_write);

static const char *cp_SparseVirtualFileSystem_svf_bytes_read_docstring = \
"Returns the count of the number of bytes read from the SVF identified by the given id.";
SVFS_SVF_METHOD_SIZE_T_WRAPPER(bytes_read);


#define IMPORT_DATETIME_IF_UNINITIALISED do {   \
    if (! PyDateTimeAPI) {                      \
        PyDateTime_IMPORT;                      \
    }                                           \
} while(0)

PyObject *
datetime_from_struct_tm(const std::tm *bdt, int usecond) {
    PyObject *ret = NULL;

    IMPORT_DATETIME_IF_UNINITIALISED;
    assert(! PyErr_Occurred());
    // This is calling new_datetime_ex() which increfs tz and sets hastzinfo
    ret = PyDateTimeAPI->DateTime_FromDateAndTime(bdt->tm_year, bdt->tm_mon, bdt->tm_mday,
                                                  bdt->tm_hour, bdt->tm_min, bdt->tm_sec,
                                                  usecond, NULL, PyDateTimeAPI->DateTimeType);
    if (! ret) {
        PyErr_Format(PyExc_RuntimeError, "%s: Can not create datetime.datetime", __FUNCTION__);
        goto except;
    }
    assert(! PyErr_Occurred());
    assert(ret);
    goto finally;
except:
    Py_XDECREF(ret);
    assert(PyErr_Occurred());
    ret = NULL;
finally:
    return ret;
}

// This macro is for functions that return a size_t type such as time_write, time_read.
#define SVFS_SVF_METHOD_DATETIME_WRAPPER(method_name) static PyObject * \
cp_SparseVirtualFileSystem_svf_##method_name(cp_SparseVirtualFileSystem *self, PyObject *args, PyObject *kwargs) { \
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs); \
    PyObject *ret = NULL; \
    PyObject *id = NULL; \
    std::string c_id; \
    static const char *kwlist[] = { "id", NULL }; \
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "S", (char **)kwlist, &id)) { \
        goto except; \
    } \
    c_id = py_unicode_to_std_string(id); \
    if (self->p_svfs->has(c_id)) { \
        const SVFS::SparseVirtualFile &svf = self->p_svfs->at(c_id); \
        auto time = svf.method_name(); \
        const long seconds = std::chrono::time_point_cast<std::chrono::seconds>(time).time_since_epoch().count(); \
        int micro_seconds = std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count() % 1000000; \
        const std::tm *p_struct_tm = std::gmtime(&seconds); \
        ret = datetime_from_struct_tm(p_struct_tm, micro_seconds); \
    } else { \
        PyErr_Format(PyExc_IndexError, "%s: No SVF ID %s", __FUNCTION__, c_id.c_str()); \
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

static const char *cp_SparseVirtualFileSystem_svf_time_write_docstring = \
"Returns the timestamp of the last write to the SVF identified by the given id as a datetime.datetime.";
SVFS_SVF_METHOD_DATETIME_WRAPPER(time_write)

static const char *cp_SparseVirtualFileSystem_svf_time_read_docstring = \
"Returns the timestamp of the last read from the SVF identified by the given id as a datetime.datetime.";
SVFS_SVF_METHOD_DATETIME_WRAPPER(time_read)

// Mapping method for __len__
static Py_ssize_t
cp_SparseVirtualFileSystem_mapping_length(PyObject *self) {
    ASSERT_FUNCTION_ENTRY_SVFS(p_svfs);
    return ((cp_SparseVirtualFileSystem *)self)->p_svfs->size();
}


static PyMemberDef cp_SparseVirtualFileSystem_members[] = {
//        {"first", T_OBJECT_EX, offsetof(CustomObject, first), 0,
//                "first name"},
        {NULL, 0, 0, 0, NULL}  /* Sentinel */
};


static PyMethodDef cp_SparseVirtualFileSystem_methods[] = {
    {
        "keys", (PyCFunction) cp_SparseVirtualFileSystem_keys, METH_NOARGS,
        cp_SparseVirtualFileSystem_keys_docstring
    },
    {
        "insert", (PyCFunction) cp_SparseVirtualFileSystem_insert, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_insert_docstring
    },
    {
        "remove", (PyCFunction) cp_SparseVirtualFileSystem_remove, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_remove_docstring
    },
    {
        "has", (PyCFunction) cp_SparseVirtualFileSystem_has, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_has_docstring
    },
    {
        "total_size_of", (PyCFunction) cp_SparseVirtualFileSystem_total_size_of, METH_NOARGS,
        cp_SparseVirtualFileSystem_total_size_of_docstring
    },
    {
        "total_bytes", (PyCFunction) cp_SparseVirtualFileSystem_total_bytes, METH_NOARGS,
        cp_SparseVirtualFileSystem_total_bytes_docstring
    },
    {
        "total_blocks", (PyCFunction) cp_SparseVirtualFileSystem_total_blocks, METH_NOARGS,
        cp_SparseVirtualFileSystem_total_blocks_docstring
    },
    {
        "has_data", (PyCFunction) cp_SparseVirtualFileSystem_svf_has_data, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_has_data_docstring
    },
    {
        "write", (PyCFunction) cp_SparseVirtualFileSystem_svf_write, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_write_docstring
    },
    {
        "read", (PyCFunction) cp_SparseVirtualFileSystem_svf_read, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_read_docstring
    },
    {
        "need", (PyCFunction) cp_SparseVirtualFileSystem_svf_need, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_need_docstring
    },
    // ---- Meta information about the SVF ----
    {
        "blocks", (PyCFunction) cp_SparseVirtualFileSystem_svf_blocks, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_blocks_docstring
    },
    {
        "size_of", (PyCFunction) cp_SparseVirtualFileSystem_svf_size_of, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_size_of_docstring
    },
    {
        "num_bytes", (PyCFunction) cp_SparseVirtualFileSystem_svf_num_bytes, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_num_bytes_docstring
    },
    {
        "num_blocks", (PyCFunction) cp_SparseVirtualFileSystem_svf_num_blocks, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_num_blocks_docstring
    },
    {
        "file_mod_time_matches", (PyCFunction) cp_SparseVirtualFileSystem_svf_file_mod_time_matches,
        METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_file_mod_time_matches_docstring
    },
    // ---- Attribute access ----
    {
        "file_mod_time", (PyCFunction) cp_SparseVirtualFileSystem_file_mod_time, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_file_mod_time_docstring
    },
    {
        "count_write", (PyCFunction) cp_SparseVirtualFileSystem_svf_count_write, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_count_write_docstring
    },
    {
        "count_read", (PyCFunction) cp_SparseVirtualFileSystem_svf_count_read, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_count_read_docstring
    },
    {
        "bytes_write", (PyCFunction) cp_SparseVirtualFileSystem_svf_bytes_write, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_bytes_write_docstring
    },
    {
        "bytes_read", (PyCFunction) cp_SparseVirtualFileSystem_svf_bytes_read, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_bytes_read_docstring
    },
    {
        "time_write", (PyCFunction) cp_SparseVirtualFileSystem_svf_time_write, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_time_write_docstring
    },
    {
        "time_read", (PyCFunction) cp_SparseVirtualFileSystem_svf_time_read, METH_VARARGS | METH_KEYWORDS,
        cp_SparseVirtualFileSystem_svf_time_read_docstring
    },
    {NULL, NULL, 0, NULL}  /* Sentinel */
};


PyMappingMethods svfs_mapping_methods = {
    // Just length as we don't (yet) want to expose the underlying SVF to Python code.
    // via mp_subscript (get), mp_ass_subscript (set).
    .mp_length = &cp_SparseVirtualFileSystem_mapping_length,
};

static PyTypeObject svfs_SVFS = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "svfs.SVFS",
        .tp_doc = "Sparse Virtual File System",
        .tp_basicsize = sizeof(cp_SparseVirtualFileSystem),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_new = cp_SparseVirtualFileSystem_new,
        .tp_init = (initproc) cp_SparseVirtualFileSystem_init,
        .tp_dealloc = (destructor) cp_SparseVirtualFileSystem_dealloc,
        .tp_members = cp_SparseVirtualFileSystem_members,
        .tp_methods = cp_SparseVirtualFileSystem_methods,
        // Mapping methods, just __len__
        .tp_as_mapping = &svfs_mapping_methods,
};

static PyModuleDef svfsmodule = {
        PyModuleDef_HEAD_INIT,
        .m_name = "svfs",
        .m_doc = "This module contains Sparse Virtual File System classes.",
        .m_size = -1,
};

PyMODINIT_FUNC
PyInit_svfs(void)
{
    PyObject *m = NULL;
    if (PyType_Ready(&svfs_SVFS) < 0) {
        return NULL;
    }

    m = PyModule_Create(&svfsmodule);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&svfs_SVFS);
    PyModule_AddObject(m, "SVFS", (PyObject *) &svfs_SVFS);
    return m;
}
