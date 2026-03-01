/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Python port support for Lingua Franca.
 */

#ifndef PYTHON_PORT_H
#define PYTHON_PORT_H

#include <Python.h>
#include <structmember.h>
#include <stdbool.h>

#include "python_capsule_extension.h"
#include "lf_types.h"
#include "port.h"
#include "python_action.h"

extern PyTypeObject py_port_capsule_t;

/**
 * The struct used to instantiate a port in Lingua Franca. This is used
 * in the PythonGenerator instead of redefining a struct for each port.
 * This can be used for any Python object, including lists and tuples.
 * PyObject* value is the value of the port with the generic Python type.
 * NOTE: The structure here must follow exactly that of lf_port_base_t,
 * which includes as its first element a token_template_t, which includes
 * as its first element a token_type_t.
 */
typedef struct {
  size_t element_size;                    // token_type_t
  void (*destructor)(void* value);        // token_type_t
  void* (*copy_constructor)(void* value); // token_type_t
  lf_token_t* token;                      // token_template_t
  size_t length;                          // token_template_t
  bool is_present;                        // lf_port_base_t
  lf_port_internal_t _base;               // lf_port_internal_t
  PyObject* value;
  FEDERATED_GENERIC_EXTENSION
} generic_port_instance_struct;

/**
 * The struct used to represent ports in Python
 * This template is used as a blueprint to create
 * Python objects that follow the same structure.
 * The resulting Python object will have the type
 * py_port_capsule_t in C (LinguaFranca.port_capsule in Python).
 *
 * port: A PyCapsule (https://docs.python.org/3/c-api/capsule.html)
 *       that safely holds a C void* inside a Python object. This capsule
 *       is passed through the Python code and is extracted in C functions
 *       like set and __getitem__.
 * value: The value of the port at the time of invocation of @see convert_C_port_to_py.
 *        The value and is_present are copied from the port if it is not a multiport and can be accessed as
 *        port.value. For multiports, is_present will be false and value will be None. The value of each individual
 *        port can be accessed as port[idx].value (@see port_capsule_get_item).
 *        Subsequent calls to set will also need to update the value and is_present fields so that they are reflected
 *        in Python code.
 * is_present: Indicates if the value of the singular port is present
 *             at the current logical time
 * width: Indicates the width of the multiport. This is set to -2 for non-multiports.
 * current_index: Used to facilitate iterative functions (@see port_iter)
 **/
typedef struct {
  PyObject_HEAD PyObject* port;
  PyObject* value;
  bool is_present;
  int width;
  long current_index;
  FEDERATED_CAPSULE_EXTENSION
} generic_port_capsule_struct;

void python_count_decrement(void* py_object);
#endif
