//
// Created by Paul Ross on 27/04/2023.
//

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
