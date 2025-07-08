/**
 * @file
 * @author Hou Seng Wong
 *
 * @brief Implementation of tag-related functions.
 */

#include "util.h"
#include "python_tag.h"
#include "python_port.h"

PyTypeObject PyTagType;

/**
 * Return the current tag object.
 */
PyObject* py_lf_tag(PyObject* self, PyObject* args) {
  py_tag_t* t = (py_tag_t*)PyType_GenericNew(&PyTagType, NULL, NULL);
  if (t == NULL) {
    return NULL;
  }
  t->tag = lf_tag(top_level_environment);
  return (PyObject*)t;
}

/**
 * Compare two tags. Return -1 if the first is less than
 * the second, 0 if they are equal, and +1 if the first is
 * greater than the second. A tag is greater than another if
 * its time is greater or if its time is equal and its microstep
 * is greater.
 * @param tag1
 * @param tag2
 * @return -1, 0, or 1 depending on the relation.
 */
PyObject* py_tag_compare(PyObject* self, PyObject* args) {
  PyObject* tag1;
  PyObject* tag2;
  if (!PyArg_UnpackTuple(args, "args", 2, 2, &tag1, &tag2)) {
    return NULL;
  }
  if (!PyObject_IsInstance(tag1, (PyObject*)&PyTagType) || !PyObject_IsInstance(tag2, (PyObject*)&PyTagType)) {
    PyErr_SetString(PyExc_TypeError, "Arguments must be Tag type.");
    return NULL;
  }
  tag_t tag1_v = ((py_tag_t*)tag1)->tag;
  tag_t tag2_v = ((py_tag_t*)tag2)->tag;
  return PyLong_FromLong(lf_tag_compare(tag1_v, tag2_v));
}

/**
 * Initialize the Tag object with the given values for "time" and "microstep",
 * both of which are required.
 * @param self A py_tag_t object.
 * @param args The arguments are:
 *      - time: A logical time.
 *      - microstep: A microstep within the logical time "time".
 */
static int Tag_init(py_tag_t* self, PyObject* args, PyObject* kwds) {
  static char* kwlist[] = {"time", "microstep", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "Lk", kwlist, &(self->tag.time), &(self->tag.microstep))) {
    return -1;
  }
  return 0;
}

/**
 * Rich compare function for Tag objects. Used in .tp_richcompare.
 *
 * @param self A py_tag_t object on the left side of the operator.
 * @param other A py_tag_t object on the right side of the operator.
 * @param op the comparison operator
 */
static PyObject* Tag_richcompare(py_tag_t* self, PyObject* other, int op) {
  if (!PyObject_IsInstance(other, (PyObject*)&PyTagType)) {
    PyErr_SetString(PyExc_TypeError, "Cannot compare a Tag with a non-Tag type.");
    return NULL;
  }

  tag_t other_tag = ((py_tag_t*)other)->tag;
  int c = -1;
  if (op == Py_LT) {
    c = (lf_tag_compare(self->tag, other_tag) < 0);
  } else if (op == Py_LE) {
    c = (lf_tag_compare(self->tag, other_tag) <= 0);
  } else if (op == Py_EQ) {
    c = (lf_tag_compare(self->tag, other_tag) == 0);
  } else if (op == Py_NE) {
    c = (lf_tag_compare(self->tag, other_tag) != 0);
  } else if (op == Py_GT) {
    c = (lf_tag_compare(self->tag, other_tag) > 0);
  } else if (op == Py_GE) {
    c = (lf_tag_compare(self->tag, other_tag) >= 0);
  }
  if (c < 0) {
    PyErr_SetString(PyExc_RuntimeError, "Invalid comparator (This statement should never be reached). ");
    return NULL;
  } else if (c) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

/**
 * Tag getter for the "time" attribute
 **/
static PyObject* Tag_get_time(py_tag_t* self, void* closure) { return PyLong_FromLongLong(self->tag.time); }

/**
 * Tag getter for the "microstep" attribute
 **/
static PyObject* Tag_get_microstep(py_tag_t* self, void* closure) {
  return PyLong_FromUnsignedLong(self->tag.microstep);
}

/**
 * Link names to getter functions.
 * Getters are used when the variable name specified are referenced with a ".".
 * For example:
 * >>> t = Tag(time=1, microstep=2)
 * >>> t.time   # calls Tag_get_time.
 * >>> t.microstep  # calls Tag_get_microstep.
 * >>> t.time = 1  # illegal since setters are omitted.
 **/
static PyGetSetDef Tag_getsetters[] = {
    {"time", (getter)Tag_get_time}, {"microstep", (getter)Tag_get_microstep}, {NULL} /* Sentinel */
};
/**
 * String representation for Tag object
 **/
PyObject* Tag_str(PyObject* self) {
  // Get PyLong representation of the "time" attribute.
  PyObject* time = Tag_get_time((py_tag_t*)self, NULL);
  // Get PyLong representation of the "microstep" attribute.
  PyObject* microstep = Tag_get_microstep((py_tag_t*)self, NULL);

  // Create the tag's string representation
  PyObject* str = PyUnicode_FromFormat("Tag(time=%U, microstep=%U)", PyObject_Str(time), PyObject_Str(microstep));

  Py_DECREF(time);
  Py_DECREF(microstep);

  return str;
}

/**
 * Definition of the PyTagType Object.
 **/
PyTypeObject PyTagType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "LinguaFranca.Tag",
    .tp_doc = "Tag object",
    .tp_basicsize = sizeof(py_tag_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)Tag_init,
    .tp_richcompare = (richcmpfunc)Tag_richcompare,
    .tp_getset = Tag_getsetters,
    .tp_str = Tag_str,
};

/**
 * @brief Convert C tag to `py_tag_t`
 *
 * @param c_tag The tag in C.
 * @return PyObject* The tag in Python.
 */
py_tag_t* convert_C_tag_to_py(tag_t c_tag) {
  py_tag_t* py_tag = PyObject_New(py_tag_t, &PyTagType);
  if (py_tag == NULL) {
    lf_print_error_and_exit("Failed to convert tag from C to Python.");
  }
  Py_INCREF(py_tag);
  py_tag->tag = c_tag;
  return py_tag;
}
