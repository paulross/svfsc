
Introduction
============

Somtimes you don't need the whole file.
Sometimes you dont *want* the whole file especially if it is huge and on some remote server.
But you might know what bits of the file that you want, for example the metadata within a TIFF file.

`svfs` implements a *Sparse Virtual File System*.
This is one where the contents of a particular file is not locally available but parts of it *can be obtained*
without downloading the whole thing.

