/**
 * @file
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Data structures for Python actions.
 */

#ifndef PYTHON_ACTION_H
#define PYTHON_ACTION_H

#include "pythontarget.h"
#include <structmember.h>
#include <stdbool.h>
#include "python_capsule_extension.h"
#include "lf_types.h"

extern PyTypeObject py_action_capsule_t;

/**
 * The struct used to instantiate an action.
 * This is used
 * in the PythonGenerator instead of redefining
 * a struct for each action.
 * This can be used for any Python object,
 * including lists and tuples.
 * PyObject* value: the value of the action with the generic Python type
 * is_present: indicates if the action is present
 *             at the current logical time
 **/
typedef struct {
  token_type_t type;
  lf_token_t* token;
  size_t length;
  bool is_present;
  lf_action_internal_t _base;
  self_base_t* parent;
  bool has_value;
  int source_id;
  PyObject* value;
  FEDERATED_GENERIC_EXTENSION
} generic_action_instance_struct;

/**
 * The struct used to hold an action
 * that is sent to a Python reaction.
 *
 * The "action" field holds a PyCapsule of the
 * void * pointer to an action.
 *
 * The "value" field holds the action value
 * if anything is given. This value is copied over
 * from action->value each time an action is passed
 * to a Python reaction.
 *
 * The "is_present" field is copied over
 * from action->value each time an action is passed
 * to a Python reaction.
 **/
typedef struct {
  PyObject_HEAD PyObject*
      action;      // Hold the void* pointer to a C action instance. However, passing void* directly
                   // to Python is considered unsafe practice. Instead, this void* pointer to the C action
                   // will be stored in a PyCapsule. @see https://docs.python.org/3/c-api/capsule.html
  PyObject* value; // This value will be copied from the C action->value
  bool is_present; // Same as value, is_present will be copied from the C action->is_present
  FEDERATED_CAPSULE_EXTENSION
} generic_action_capsule_struct;

#endif
