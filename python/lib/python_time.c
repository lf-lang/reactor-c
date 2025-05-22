/**
 * @file
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Implementation of time-related functions.
 */
#include <Python.h>
#include <structmember.h>

#include "python_port.h"
#include "tag.h"

///////// Time-keeping functions //////////
/**
 * Return the logical time in nanoseconds.
 */
PyObject* py_lf_time_logical(PyObject* self, PyObject* args) {
  return PyLong_FromLongLong(lf_time_logical(top_level_environment));
}

/**
 * Return the elapsed logical time in nanoseconds.
 */
PyObject* py_lf_time_logical_elapsed(PyObject* self, PyObject* args) {
  return PyLong_FromLongLong(lf_time_logical_elapsed(top_level_environment));
}

/**
 * Return the physical time in nanoseconds.
 */
PyObject* py_lf_time_physical(PyObject* self, PyObject* args) { return PyLong_FromLongLong(lf_time_physical()); }

/**
 * Return the elapsed physical time in nanoseconds.
 */
PyObject* py_lf_time_physical_elapsed(PyObject* self, PyObject* args) {
  return PyLong_FromLongLong(lf_time_physical_elapsed());
}

/**
 * Return the start time in nanoseconds.
 */
PyObject* py_lf_time_start(PyObject* self, PyObject* args) { return PyLong_FromLongLong(lf_time_start()); }

PyTypeObject PyTimeType;

PyMethodDef PyTimeTypeMethods[] = {
    {"logical", (PyCFunction)py_lf_time_logical, METH_NOARGS | METH_STATIC, "Get the current logical time."},
    {"logical_elapsed", (PyCFunction)py_lf_time_logical_elapsed, METH_NOARGS | METH_STATIC,
     "Get the current elapsed logical time"},
    {"physical", (PyCFunction)py_lf_time_physical, METH_NOARGS | METH_STATIC, "Get the current physical time"},
    {"physical_elapsed", (PyCFunction)py_lf_time_physical_elapsed, METH_NOARGS | METH_STATIC,
     "Get the current elapsed physical time"},
    {"start", (PyCFunction)py_lf_time_start, METH_NOARGS | METH_STATIC, "Get the start time"},
    {NULL} /* Sentinel */
};

/**
 * Definition of the PyTimeType Object.
 **/
PyTypeObject PyTimeType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "LinguaFranca.TimeType",
    .tp_doc = "Time object",
    .tp_basicsize = 0,
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = PyTimeTypeMethods,
};
