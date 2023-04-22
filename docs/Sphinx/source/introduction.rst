
A Sparse Virtual File
#################################################

Somtimes you don't need the whole file.
Sometimes you dont *want* the whole file.
Especially if it is huge and on some remote server.
But, you might know what bits of the file that you want and ``svfs`` can help you store them so it looks like you have access to the complete file.

`svfs` implements a *Sparse Virtual File System*.
This is one where a particular file might not be available but *parts of it can be obtained* without reading the whole thing.

``svfs`` is written in C++ with bindings to Python.
It can be used by any language.
