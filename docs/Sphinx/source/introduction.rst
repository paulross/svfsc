
Sparse Virtual File
#################################################

Introduction
======================

Somtimes you don't need the whole file.
Sometimes you don't *want* the whole file.
Especially if it is huge and on some remote server.
But, you might know what parts of the file that you want and ``svfsc`` can help you store them locally so it looks
*as if* you have access to the complete file but with just the pieces of interest.

``svfsc`` is targeted at reading very large binary files such as TIFF, RP66V1, HDF5 where the structure is well known.
For example you might want to parse a TIFF file for its metadata or a particular image tile which is usually a tiny
fraction of the file itself.

``svfsc`` implements a *Sparse Virtual File*, a specialised in-memory cache where a particular file might not be
available but *parts of it can be obtained* without reading the whole file.
A Sparse Virtual File (SVF) is represented internally as a map of blocks of data with the key being their file
offsets.
Any write to an SVF will coalesce those blocks where possible.
A Sparse Virtual File System (SVFS) is an extension of this to provide a key/value store where the key is a file ID
and the value a Sparse Virtual File.

``svfsc`` is written in C++ with a Python interface.
It is thread safe in both domains.

A SVFS might be used like this:

- The user requests some data (for example TIFF metadata) from a remote file using a Parser that knows the TIFF structure.
- The Parser consults the SVFS, if the SVFS has the data then the Parser parses it and gives the results to the user.
- If the SVFS does *not* have the data then the Parser consults the SVFS for what data is needed, then issues the appropriate GET request(s) to the remote server.
- That data is used to update the SVFS, then the parser can use it and give the results to the user.

Here is a conceptual example of an ``SVFS`` running on a local file system.

.. code-block:: console

                CLIENT SIDE           |             LOCAL FILE SYSTEM
                                      .
    /------\      /--------\          |              /-------------\
    | User | <--> | Parser | <-- read(fpos, len) --> | File System |
    \------/      \--------/          |              \-------------/
                       |              .
                       |              |
                  /--------\          .
                  |  SVFS  |          |
                  \--------/          .

.. raw:: latex

    \newpage

Here is a conceptual example of an ``SVFS`` running with a remote file system.

.. code-block:: console

                CLIENT SIDE           |             SERVER SIDE
                                      .
    /------\      /--------\          |             /--------\
    | User | <--> | Parser | <-- GET(fpos, len) --> | Server |
    \------/      \--------/          |             \--------/
                       |              .                  |
                       |              |                  |
                  /--------\          .           /-------------\
                  |  SVFS  |          |           | File System |
                  \--------/          .           \-------------/

Example Python Usage
======================

Installation
------------

Install from pypi:

.. code-block:: console

    $ pip install svfs

Complete installation instructions are :ref:`here <installation>`.

Using a Single SVF
------------------

This shows the basic functionality: ``write()``, ``read()`` and ``need()``:

.. code-block:: python

    # Construct a Sparse Virtual File
    svf = svfs.cSVF('Some file ID')
    # Write six bytes at file position 14
    svf.write(14, b'ABCDEF')
    # Read from it
    svf.read(16, 2) # Returns b'CD'
    # What do I have to do to read 24 bytes from file position 8?
    # This returns a tuple of pairs ((file_position, read_length), ...)
    svf.need(8, 24) # Returns ((8, 6), (20, 4))
    # Go and get the data from those file positions and write it to
    # the SVF then you can read directly from the SVF.

The basic operation is to check if the ``SVF`` has data, if not then get it and write that data to the SVF.
Then you can read directly.
For example:

.. code-block:: python

        if not svf.has_data(file_position, length):
            for read_position, read_length in svf.need(file_position, length):
                # Somehow get data as a bytes object at (read_position, read_length)...
                svf.write(read_position, data)
        # Now read directly
        svf.read(file_position, length)

A Sparse Virtual File System
-------------------------------------

The example above uses a single Sparse Virtual File, but you can also create a Sparse Virtual File System.
This is a key/value store where the key is some string and the value a ``SVF``:

.. code-block:: python

    svfs = svfs.cSVFS()
    # Insert an empty SVF with a corresponding ID
    ID = 'abc'
    svfs.insert(ID)
    # Write six bytes to that SVF at file position 14
    svfs.write(ID, 14, b'ABCDEF')
    # Read from the SVF
    svfs.read(ID, 16, 2) # Returns b'CD'
    # What do I have to do to read 24 bytes from file position 8
    # from that SVF?
    svfs.need(ID, 8, 24) # Returns ((8, 6), (20, 4))


Example C++ Usage
====================

``svfsc`` is written in C++ so can be used directly:

.. code-block:: c++

    #include "svf.h"

    SVFS::SparseVirtualFile svf("Some file ID");
    // Write six bytes at file position 14
    svf.write(14, "ABCDEF", 6);
    // Read from it
    char read_buffer[2];
    svf.read(16, 2, read_buffer);
    // What do I have to do to read 24 bytes from file position 8?
    // This returns a std::vector<std::pair<size_t, size_t>>
    // as ((file_position, read_length), ...)
    auto need = svf.need(8, 24);
    // This prints ((8, 6), (20, 4),)
    std::cout << "(";
    for (auto &val: need) {
        std::cout << "(" << val.first << ", " << val.second << "),";
    }
    std::cout << ")" << std::endl;

