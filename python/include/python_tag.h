/**
 * @file
 * @author Hou Seng Wong
 *
 * @brief Functions for tag operations in Python.
 */

#ifndef PYTHON_TAG_H
#define PYTHON_TAG_H
#include <Python.h>
#include <structmember.h>
#include "tag.h"

extern PyTypeObject PyTagType;

/**
 * Python wrapper for the tag_t struct in the C target.
 **/
typedef struct {
  PyObject_HEAD tag_t tag;
} py_tag_t;

/**
 * @brief Convert C tag to `py_tag_t`
 *
 * @param c_tag The tag in C.
 * @return py_tag_t* The tag in Python.
 */
py_tag_t* convert_C_tag_to_py(tag_t c_tag);

PyObject* py_lf_tag(PyObject* self, PyObject* args);
PyObject* py_tag_compare(PyObject* self, PyObject* args);

#endif
