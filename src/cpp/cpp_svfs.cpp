/** @file
 *
 * A Sparse Virtual File version information.
 *
 * Created by Paul Ross on 15/06/2023.
 *
 * @verbatim
    MIT License

    Copyright (c) 2023 Paul Ross

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

#include "cpp_svfs.h"

/** @brief The version as a string. */
const char *SVFS_CPP_VERSION = "0.2.1";

/** @brief Major version number. */
int SVFS_CPP_VERSION_MAJOR = 0;
/** @brief Minor version number. */
int SVFS_CPP_VERSION_MINOR = 2;
/** @brief Patch version number. */
int SVFS_CPP_VERSION_PATCH = 1;
/** @brief Version suffix string, could be "rc2" for example. */
const char *SVFS_CPP_VERSION_SUFFIX = "";
