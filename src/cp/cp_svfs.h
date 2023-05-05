/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2023-04-27
 *
 * @verbatim
    MIT License

    Copyright (c) 2020-2023 Paul Ross

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 @endverbatim
 */

#ifndef CPPSVF_CP_SVFS_H
#define CPPSVF_CP_SVFS_H

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"
#include "datetime.h"

#ifdef WITH_THREAD

#include "pythread.h"

#define PY_THREAD_SAFE

#endif


#endif //CPPSVF_CP_SVFS_H
