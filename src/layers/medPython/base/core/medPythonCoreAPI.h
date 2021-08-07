#pragma once
/*==============================================================================

 medInria

 Copyright (c) INRIA 2021. All rights reserved.
 See LICENSE.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

==============================================================================*/

/// NOTE: CPython defines pre-processor macros that affect the standard headers.
/// Since this file includes the Python header, it must therefore always be
/// included before any standard headers. To make sure this is the case, the
/// include directive should be placed at the top of any file that directly
/// or indirectly includes this one.
///

// One of Python's structs has a member named slots, and this causes conflicts
// with the the Qt keyword of the same name.
#undef slots
#define slots _slots

// Python recommends to always define this macro before including its header.
#define PY_SSIZE_T_CLEAN

// The Python header is included only for the structs, macros etc. The pointers
// declared below are used instead of the functions and global variables defined
// in the header.
#include <Python.h>

#undef slots
#define slots Q_SLOTS
