/**
 * @file
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @autohr Hou Seng Wong (housengw@berkeley.edu)
 *
 * @section LICENSE
Copyright (c) 2022, The University of California at Berkeley.
Copyright (c) 2021, The University of Texas at Dallas.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 * Implementation of functions defined in @see pythontarget.h
 */
#include <Python.h>
#include <structmember.h>

#include "python_port.h"
#include "tag.h"

///////// Time-keeping functions //////////
/**
 * Return the logical time in nanoseconds.
 */
PyObject* py_lf_time_logical(PyObject *self, PyObject *args) {
    return PyLong_FromLongLong(lf_time_logical(top_level_environment));
}

/**
 * Return the elapsed logical time in nanoseconds.
 */
PyObject* py_lf_time_logical_elapsed(PyObject *self, PyObject *args) {
    return PyLong_FromLongLong(lf_time_logical_elapsed(top_level_environment));
}

/**
 * Return the physical time in nanoseconds.
 */
PyObject* py_lf_time_physical(PyObject *self, PyObject *args) {
    return PyLong_FromLongLong(lf_time_physical());
}

/**
 * Return the elapsed physical time in nanoseconds.
 */
PyObject* py_lf_time_physical_elapsed(PyObject *self, PyObject *args) {
    return PyLong_FromLongLong(lf_time_physical_elapsed());
}

/**
 * Return the start time in nanoseconds.
 */
PyObject* py_lf_time_start(PyObject *self, PyObject *args) {
    return PyLong_FromLongLong(lf_time_start());
}

PyTypeObject PyTimeType;

PyMethodDef PyTimeTypeMethods[] = {
    {"logical", (PyCFunction) py_lf_time_logical, METH_NOARGS|METH_STATIC, "Get the current logical time."},
    {"logical_elapsed", (PyCFunction) py_lf_time_logical_elapsed, METH_NOARGS|METH_STATIC, "Get the current elapsed logical time"},
    {"physical", (PyCFunction) py_lf_time_physical, METH_NOARGS|METH_STATIC, "Get the current physical time"},
    {"physical_elapsed", (PyCFunction) py_lf_time_physical_elapsed, METH_NOARGS|METH_STATIC, "Get the current elapsed physical time"},
    {"start", (PyCFunction) py_lf_time_start, METH_NOARGS|METH_STATIC, "Get the start time"},
    {NULL}  /* Sentinel */
};

/**
 * Definition of the PyTimeType Object.
 **/
PyTypeObject PyTimeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "LinguaFranca.TimeType",
    .tp_doc = "Time object",
    .tp_basicsize = 0,
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = PyTimeTypeMethods,
};
