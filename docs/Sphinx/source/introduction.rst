
Sparse Virtual File
#################################################

Somtimes you don't need the whole file.
Sometimes you dont *want* the whole file.
Especially if it is huge and on some remote server.
But, you might know what parts of the file that you want and ``svfs`` can help you store them locally so it looks
*as if* you have access to the complete file, or the pieces of interest.
``svfs`` is targeted at reading very large binary files such as TIFF, RP66V1, HDM5 where the structure is well known.

`svfs` implements a *Sparse Virtual File System*.
This is a specialised cache where a particular file might not be available but *parts of it can be obtained* without
reading the whole thing.

``svfs`` is written in C++ with a Python interface.
It is thread safe in both domains.

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

        if not svf.has_data(position, length):
            for position, read_length in svf.need(position, length):
                # Somehow get data as a bytes object at position...
                svf.write(fposition, data)
        # Now read directly
        svf.read(file_position, length)

Creating a Sparse Virtual File System
-------------------------------------

The example above uses a single Sparse Virtual File, but you can also create a Sparse Virtual File System.
This is a key value store where the key is some string and the value a ``SVF``:

.. code-block:: python
    :caption: Example Sparse Virtual File System

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
    // Tbhis prints ((8, 6), (20, 4),)
    std::cout << "(";
    for (auto &val: need) {
        std::cout << "(" << val.first << ", " << val.second << "),";
    }
    std::cout << ")" << std::endl;

