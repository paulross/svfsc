Performance
###########

This describes some of the measured performance of `SVFS`.

C++ Performance
===============

Need
----

This test simulates writing a low level RP66V1 index and then running need on it.
Total bytes written around 1Mb.
Blocks are 800 bytes apart.
There are 238,310 blocks.

.. image:: ../../plots/cpp_need.png

This shows good linear performance.

Write
-----

This show the performance of writing 1MB of data to a `SVF` in two states:

- Each write is contiguous with a previous one so the blocks are always coalesced. The `SVF` only contains one block.
- Each write is *not* contiguous with a previous one so the blocks are *never* coalesced. The `SVF` contains as many blocks as writes.

.. image:: ../../plots/cpp_1mb_write.png


Multi-threaded Writes
---------------------

This studies the performance where many threads might be writing independently to a single `SVF`.

Test function `test_write_multithreaded_n()` which calls `_write_multithreaded()` with a varying number of threds.
`_write_multithreaded()` simulates a RP66V1 file and writes a sparse file with 238,310 blocks each of 4 bytes.

.. image:: ../../plots/cpp_write_multithreaded.png

Read
----

The performance of `read()` on the reference platform is around 1340 MB/s.


Python Performance
==================

TODO.
