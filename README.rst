
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
For example you might want to parse a TIFF file for its metadata or for a particular image tile or strip which is a tiny
fraction of the file itself.

``svfsc`` implements a *Sparse Virtual File*, a specialised in-memory cache where a particular file might not be
available but *parts of it can be obtained* without reading the whole file.
A Sparse Virtual File (SVF) is represented internally as a map of blocks of data with the key being their file
offsets.
Any write to an SVF will coalesce these blocks where possible.
There is no cache punting strategy implemented so an SVF always accumulates data.
A Sparse Virtual File System (SVFS) is an extension of this to provide a key/value store where the key is a file ID
and the value a Sparse Virtual File.

``svfsc`` is written in C++ with a Python interface.
It is thread safe in both domains.

A SVF might be used like this:

- The user requests some data (for example TIFF metadata) from a remote file using a Parser that knows the TIFF structure.
- The Parser consults the SVF, if the SVF has the data then the Parser parses it and gives the results to the user.
- If the SVF does *not* have the data then the Parser consults the SVF for what data is needed, then issues the appropriate GET request(s) to the remote server.
- That data is used to update the SVF, then the parser can use it and give the results to the user.

Here is a conceptual example of a ``SVF`` running on a local file system containing data from a single file.

.. code-block:: console

                CLIENT SIDE           |             LOCAL FILE SYSTEM
                                      .
    /------\      /--------\          |              /-------------\
    | User | <--> | Parser | <-- read(fpos, len) --> | File System |
    \------/      \--------/          |              \-------------/
                       |              .
                       |              |
                   /-------\          .
                   |  SVF  |          |
                   \-------/          .

Here is a conceptual example of an ``SVFS`` running with a remote file system.

.. code-block:: console

                CLIENT SIDE           |             SERVER SIDE
                                      .
    /------\      /--------\          |             /--------\
    | User | <--> | Parser | <-- GET(fpos, len) --> | Server |
    \------/      \--------/          |             \--------/
                       |              .                  |
                       |              |                  |
                   /-------\          .           /-------------\
                   |  SVF  |          |           | File System |
                   \-------/          .           \-------------/

Example Python Usage
======================

Installation
------------

Install from pypi:

.. code-block:: console

    $ pip install svfsc

Using a Single SVF
------------------

This shows the basic functionality: ``write()``, ``read()`` and ``need()``:

.. code-block:: python

    import svfsc

    # Construct a Sparse Virtual File
    svf = svfsc.cSVF('Some file ID')

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
Then read directly:

.. code-block:: python

    if not svf.has_data(file_position, length):
        for read_position, read_length in svf.need(file_position, length):
            # Somehow get the data as a bytes object at (read_position, read_length)...
            # This could be a GET request to a remote file.
            # Then...
            svf.write(read_position, data)
    # Now read directly
    svf.read(file_position, length)

A Sparse Virtual File System
-------------------------------------

The example above uses a single Sparse Virtual File, but you can also create a Sparse Virtual File System.
This is a key/value store where the key is some string and the value a ``SVF``:

.. code-block:: python

    import svfsc

    svfs = svfsc.cSVFS()

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

    // File modification time of 1672574430.0 (2023-01-01 12:00:30)
    SVFS::SparseVirtualFile svf("Some file ID", 1672574430.0);

    // Write six char at file position 14
    svf.write(14, "ABCDEF", 6);

    // Read from it
    char read_buffer[2];
    svf.read(16, 2, read_buffer);
    // read_buffer now contains "CD"

    // What do I have to do to read 24 bytes from file position 8?
    // This returns a std::vector<std::pair<size_t, size_t>>
    // as ((file_position, read_length), ...)
    auto need = svf.need(8, 24);

    // The following prints ((8, 6), (20, 4),)
    std::cout << "(";
    for (auto &val: need) {
        std::cout << "(" << val.first << ", " << val.second << "),";
    }
    std::cout << ")" << std::endl;

.. note:: Naming conventions

   On PyPi there is a preexisting `SVFS project <https://pypi.org/project/SVFS/>`_
   (no relation, apparently abandoned since its release in 2012).
   This project was renamed to ``svfsc``.
   However there are many internal references in this project to ``SVF``, ``SVFS`` and variations thereof.

   - The Cmake target is ``cppSVF``.
   - The C++ code is in the namespace ``SVFS``, the important classes there are ``SVFS::SparseVirtualFile`` and ``SVFS::SparseVirtualFileSystem``.
   - This `Python project on PyPi <https://pypi.org/project/svfsc/>`_ is named ``svfsc``. This can be installed by: ``pip install svfsc``.
   - Access to the Python interface is done with: ``import svfsc``. The two important Python classes, equivalents of the C++ ones,  are ``svfsc.cSVF`` and ``svfsc.cSVFS``
   - Filenames often use ``svf`` and ``svfs`` in various ways.

Documentation
===============

Build the documentation from the ``docs`` directory or find it on readthedocs: https://svfsc.readthedocs.io/

Acknowledgments
===============

Many thanks to my employer `Paige.ai <https://paige.ai>`_ for allowing me to release this as FOSS software.
