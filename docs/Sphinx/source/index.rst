.. svfs documentation master file, created by
   sphinx-quickstart on Fri Apr 21 13:07:14 2023.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Documentation for ``svfsc``
================================

The ``svfsc`` package provides a Sparse Virtual File System which is capable of holding the essential fragments of a
binary such a metadata.
``svfsc`` can hold this data in-memory in an efficient way without the cost of reading the whole file.
This is especially useful when reading large files from the cloud where the cost of accessing the whole file can be
very high.

.. toctree::
   :maxdepth: 3
   :caption: Contents:

   introduction
   installation
   performance
   tech_notes
   testing
   ref/index.rst
   license

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
