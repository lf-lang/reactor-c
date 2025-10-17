/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Implementation of functions defined in @see pythontarget.h
 */
PyObject* py_lf_time_logical(PyObject* self, PyObject* args);
PyObject* py_lf_time_logical_elapsed(PyObject* self, PyObject* args);
PyObject* py_lf_time_physical(PyObject* self, PyObject* args);
PyObject* py_lf_time_physical_elapsed(PyObject* self, PyObject* args);
PyObject* py_lf_time_start(PyObject* self, PyObject* args);

extern PyTypeObject PyTimeType;

extern PyMethodDef PyTimeTypeMethods[];
extern PyTypeObject PyTimeType;
