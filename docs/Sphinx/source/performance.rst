Performance
###########

This describes some of the measured performance of `SVFS`.

C++ Performance
===============

Read
----

This test simulates reading from a one Megabyte of data arranged in a sparse form with different block sizes from 1 byte to 512 bytes.
For the one byte case there are 1,000,000 blocks each of 1 byte, for the 512 byte case there are 2,048 blocks each of 512 bytes.
In the extreme right the data is coalesced into a single one Megabyte block.

The y axis shows the time to read all blocks.

.. image:: ../../plots/cpp_1mb_read.png

The one byte case corresponds to 7.6 MB/s, the 512 bytes case corresponds to 2,174 MB/s, the single 1MB block case
corresponds to 1,480 MB/s.

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

This show the performance of writing 1MB of data to a `SVF` in two ways:

- Each write is contiguous with a previous one so the blocks are always coalesced. The `SVF` always contains only one block.
- Each write is *not* contiguous with a previous one so the blocks are *never* coalesced. The `SVF` eventually contains as many blocks as writes.

.. image:: ../../plots/cpp_1mb_write.png

In the case of storing 1M one byte blocks the `SVF` consumes 34,603,192 bytes of memory, so x33.
In the case of a 256 byte block size the `SVF` consumes 1,179,832 bytes of memory, just a 12.5% premium.

Multi-threaded Writes
---------------------

This looks at the performance where many threads might be writing independently to a single `SVF`.
This requires the code be compiled with `SVF_THREAD_SAFE`.

This is done with the test function `test_write_multithreaded_n()` which calls `_write_multithreaded()` with a varying
number of threads.
`_write_multithreaded()` simulates a RP66V1 file and writes a single sparse file with 238,310 blocks each of 4 bytes.

.. image:: ../../plots/cpp_write_multithreaded.png

Python Performance
==================

TODO.
