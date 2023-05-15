.. _installation:

Installation
============

Stable release
--------------

.. code-block:: console

    $ pip install svfs

This is the preferred method to install ``svfs``, as it will always install the most recent stable release. 

If you don't have `pip`_ installed, this `Python installation guide`_ can guide
you through the process.

.. _pip: https://pip.pypa.io
.. _Python installation guide: http://docs.python-guide.org/en/latest/starting/installation/

From sources
------------

The sources for ``svfs`` can be downloaded from the `Github repo`_.

You can either clone the public repository:

.. code-block:: console

    $ git clone git://github.com/paulross/svfs

Or download the `tarball`_:

.. code-block:: console

    $ curl -OL https://github.com/paulross/svfs/tarball/master

Once you have a copy of the source, create a virtual environment of your choice:

.. code-block:: console

    $ python3 -m venv <your_virtual_environment_directory>
    $ source <your_virtual_environment_directory>/bin/activate
    $ pip install -r requirements.txt
    $ python setup.py install

Or for development, change the last line to:

.. code-block:: console

    $ python setup.py develop

Building for C++
----------------

Consult the ``CMakeLists.txt`` at the project root for how to do this.

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

The PDF documentation, which does *not* include the Doxygen documentation, is here:

.. code-block:: console

    $ open Sphinx/build/latex/svfs.pdf

.. _Github repo: https://github.com/paulross/svfs
.. _tarball: https://github.com/paulross/svfs/tarball/master
