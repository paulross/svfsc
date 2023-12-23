.. _installation:

Installation
============

Stable release
--------------

.. code-block:: console

    $ pip install svfsc

This is the preferred method to install ``svfsc``, as it will always install the most recent stable release.

If you don't have `pip`_ installed, this `Python installation guide`_ can guide
you through the process.

.. _pip: https://pip.pypa.io
.. _Python installation guide: http://docs.python-guide.org/en/latest/starting/installation/

From sources
------------

The sources for ``svfsc`` can be downloaded from the `Github repo`_.

You can either clone the public repository:

.. code-block:: console

    $ git clone git://github.com/paulross/svfsc

Or download the `tarball`_:

.. code-block:: console

    $ curl -OL https://github.com/paulross/svfsc/tarball/master

Once you have a copy of the source, create a virtual environment of your choice:

.. code-block:: console

    $ python3 -m venv <your_virtual_environment_directory>
    $ source <your_virtual_environment_directory>/bin/activate
    $ pip install -r requirements.txt
    $ python setup.py install

Or for development, change the last line to:

.. code-block:: console

    $ python setup.py develop

Dependencies
^^^^^^^^^^^^

- ``psutil`` for memory monitoring.
- ``pytest`` and ancillary libraries for testing.
- ``sphinx`` for documentation.

See ``requirements.txt`` for the specific dependencies.

Building for C++
----------------

Consult the ``CMakeLists.txt`` at the project root for how to do this. The build target is ``cppSVF``.

Build and Test
^^^^^^^^^^^^^^

Typically a release build is done with:

.. code-block:: console

    cmake --build cmake-build-release --target cppSVF -- -j 6

The executable is in ``cmake-build-release/cppSVF``.
Executing that will run all the C++ tests, the exit code will be zero on success, non-zero if any test fails.

Dependencies
^^^^^^^^^^^^

- stdlib
- Python

Incorporating SVF Into Your C++ Project
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to include SVF into your C++ project your ``CMakeLists.txt`` should include:

.. code-block:: cmake

    add_executable(
            <your_project>
            # ...
            # Version information
            src/cpp/cpp_svfs.h
            src/cpp/cpp_svfs.cpp
            # The Sparse Virtual File
            src/cpp/svf.h
            src/cpp/svf.cpp
            # The Sparse Virtual File System
            src/cpp/svfs.h
            src/cpp/svfs.cpp
            # ...
    )

    include_directories(
            # ...
            src/cpp
            # ...
    )

Using SVF in your C++ Project
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To use a SVF in your C++ code:

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

To use a SVFS in your C++ code:

.. code-block:: c++

    #include "svfs.h"
    SVFS::SparseVirtualFileSystem svfs;
    std::string id = "Some file ID";

    // Add a SVF with a modification time of 1672574430.0 (2023-01-01 12:00:30)
    svfs.insert(id, 1672574430.0);

    // Write six char at file position 14
    // .at() will throw a SVFS::Exceptions::ExceptionSparseVirtualFileSystemOutOfRange
    // If the id is not in the SVFS.
    svfs.at(id).write(14, "ABCDEF", 6);

    // Read from it
    char read_buffer[2];
    svfs.at(id).read(16, 2, read_buffer);
    // read_buffer now contains "CD"

    // What do I have to do to read 24 bytes from file "Some file ID" in position 8?
    // This returns a std::vector<std::pair<size_t, size_t>>
    // as ((file_position, read_length), ...)
    auto need = svfs.at(id).need(8, 24);

    // The following prints ((8, 6), (20, 4),)
    std::cout << "(";
    for (auto &val: need) {
        std::cout << "(" << val.first << ", " << val.second << "),";
    }
    std::cout << ")" << std::endl;

Building the Documentation
---------------------------

Prerequisites are `Sphinx`_ and `Doxygen`_ (and Latex for the PDF documentation).
The complete documentation can be built thus:

.. _Sphinx: https://www.sphinx-doc.org/en/master/
.. _Doxygen: https://www.doxygen.nl

.. code-block:: console

    $ cd docs
    $ ./build_docs.sh

The HTML documentation, which includes the Doxygen documentation, is here:

.. code-block:: console

    $ open Sphinx/build/html/index.html

The Doxygen HTML documentation is here:

.. code-block:: console

    $ open Sphinx/build/html/_static/doxygen_html/index.html

The PDF documentation, which does *not* include the Doxygen documentation, is here:

.. code-block:: console

    $ open Sphinx/build/latex/svfsc.pdf

.. _Github repo: https://github.com/paulross/svfsc
.. _tarball: https://github.com/paulross/svfsc/tarball/master
