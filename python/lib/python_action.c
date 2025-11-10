/**
 * @file
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Implementation of action-related functions.
 */

#include <Python.h>

#include "python_action.h"

PyTypeObject py_action_capsule_t;

///////////////// Functions used in action creation, initialization and deletion /////////////
/**
 * Called when an action in Python is deallocated (generally
 * called by the Python grabage collector).
 * @param self
 */
void py_action_capsule_dealloc(generic_action_capsule_struct* self) {
  Py_XDECREF(self->action);
  Py_XDECREF(self->value);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

/**
 * Called when an action in Python is to be created. Note that LinguaFranca.action_capsule
 * follows the same structure as the @see generic_action_capsule_struct.
 *
 * To initialize the action_capsule, this function first calls the tp_alloc
 * method of type py_action_capsule_t and then assign default values of NULL, NULL, 0
 * to the members of the generic_action_capsule_struct.
 */
PyObject* py_action_capsule_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
  generic_action_capsule_struct* self;
  self = (generic_action_capsule_struct*)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->action = NULL;
    self->value = NULL;
    self->is_present = false;
  }
  return (PyObject*)self;
}

/**
 * Initialize the action capsule "self" with the given optional values for
 * action (void *), value (PyObject*), and is_present (bool). If any of these arguments
 * are missing, the default values are assigned.
 *
 * @see port_intance_new
 * @param self The port_instance PyObject that follows
 *              the generic_port_instance_struct* internal structure
 * @param args The optional arguments that are:
 *      - action: The void * pointer to a C action instance struct
 *      - value: value of the port
 *      - is_present: An indication of whether or not the value of the port
 *                      is present at the current logical time.
 *      - num_destination: Used for reference-keeping inside the C runtime
 */
int py_action_capsule_init(generic_action_capsule_struct* self, PyObject* args, PyObject* kwds) {
  static char* kwlist[] = {"action", "value", "is_present", NULL};
  PyObject *action = NULL, *value = NULL, *tmp;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist, &action, &value, &self->is_present)) {
    return -1;
  }
  if (action) {
    tmp = self->action;
    Py_INCREF(action);
    self->action = action;
    Py_XDECREF(tmp);
  }
  if (value) {
    tmp = self->value;
    Py_INCREF(value);
    self->value = value;
    Py_XDECREF(tmp);
  }
  return 0;
}

//////////////////////////////////////////////////////////////
/////////////  Python Structs
//// Actions /////
/*
 * The members of a action_capsule that are accessible from a Python program, used to define
 * a native Python type.
 */
PyMemberDef py_action_capsule_members[] = {
    {"action", T_OBJECT, offsetof(generic_action_capsule_struct, action), 0, "The pointer to the C action struct"},
    {"value", T_OBJECT, offsetof(generic_action_capsule_struct, value), 0, "Value of the action"},
    {"is_present", T_BOOL, offsetof(generic_action_capsule_struct, is_present), 0,
     "Check that shows if action is present"},
    {NULL} /* Sentinel */
};

/**
 * The function members of action capsule
 */
PyMethodDef py_action_capsule_methods[] = {
    {"schedule", (PyCFunction)py_schedule, METH_VARARGS, "Schedule the action with the given offset"},
    {NULL} /* Sentinel */
};

/*
 * The definition of action_capsule type object.
 * Used to describe how an action_capsule behaves.
 */
PyTypeObject py_action_capsule_t = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "LinguaFranca.action_instance",
    .tp_doc = "action_instance object",
    .tp_basicsize = sizeof(generic_action_capsule_struct),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = py_action_capsule_new,
    .tp_init = (initproc)py_action_capsule_init,
    .tp_dealloc = (destructor)py_action_capsule_dealloc,
    .tp_members = py_action_capsule_members,
    .tp_methods = py_action_capsule_methods,
};