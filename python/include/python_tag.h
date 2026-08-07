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
 * @ingroup Python
 */
typedef struct {
  PyObject_HEAD tag_t tag;
} py_tag_t;

/**
 * @brief Convert C tag to `py_tag_t`
 * @ingroup Python
 *
 * @param c_tag The tag in C.
 * @return py_tag_t* The tag in Python.
 */
py_tag_t* convert_C_tag_to_py(tag_t c_tag);

/**
 * @brief Return the current tag object.
 * @ingroup Python
 *
 * @param self The Python object.
 * @param args The arguments (none).
 * @return The current tag object.
 */
PyObject* py_lf_tag(PyObject* self, PyObject* args);

/**
 * @brief Return the effective start tag of this federate.
 * @ingroup Python
 *
 * The returned tag is the tag at which the `startup` reactions actually fire.
 * This can be later than the federation's nominal start tag when the federate
 * joins after the federation has begun executing (e.g., a transient federate
 * rejoining mid-execution).
 *
 * @param self The Python object.
 * @param args The arguments (none).
 * @return The effective start tag object.
 */
PyObject* py_lf_tag_start_effective(PyObject* self, PyObject* args);

/**
 * @brief Compare two tags.
 * @ingroup Python
 *
 * Return -1 if the first is less than the second, 0 if they are equal,
 * and +1 if the first is greater than the second. A tag is greater than another if
 * its time is greater or if its time is equal and its microstep is greater.
 * @param tag1
 * @param tag2
 * @return -1, 0, or 1 depending on the relation.
 */
PyObject* py_tag_compare(PyObject* self, PyObject* args);

#endif
