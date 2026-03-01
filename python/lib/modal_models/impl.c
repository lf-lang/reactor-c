/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief Implementation of modal models support in the Python target.
 */

#include "modal_models/definitions.h"
#include "util.h"

//////////// set Function /////////////

/**
 * Set a new mode for a modal model.
 */
static PyObject* py_mode_set(PyObject* mode_capsule, PyObject* args) {
  mode_capsule_struct_t* m = (mode_capsule_struct_t*)mode_capsule;

  reactor_mode_t* mode = PyCapsule_GetPointer(m->mode, "mode");
  if (mode == NULL) {
    lf_print_error("Null pointer received.");
    exit(1);
  }
  Py_INCREF(m->mode);

  self_base_t* self = PyCapsule_GetPointer(m->lf_self, "lf_self");
  if (self == NULL) {
    lf_print_error("Null pointer received.");
    exit(1);
  }
  Py_INCREF(m->lf_self);

  _LF_SET_MODE_WITH_TYPE(mode, m->change_type);

  Py_INCREF(Py_None);
  return Py_None;
}

//////////// Python Struct /////////////

/**
 * Called when an mode in Python is to be created. Note that this is not normally
 * used because modes are not created in Python.
 *
 * To initialize the mode_capsule, this function first calls the tp_alloc
 * method of type mode_capsule_struct_t and then assign default values of NULL, NULL, 0
 * to the members of the generic_mode_capsule_struct.
 */
PyObject* py_mode_capsule_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
  mode_capsule_struct_t* self = (mode_capsule_struct_t*)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->mode = NULL;
    self->lf_self = NULL;
    self->change_type = 0;
  }
  return (PyObject*)self;
}

/*
 * The function members of mode_capsule.
 * The set function is used to set a new mode.
 */
static PyMethodDef mode_capsule_methods[] = {
    {"set", (PyCFunction)py_mode_set, METH_NOARGS, "Set a new mode."}, {NULL} /* Sentinel */
};

/**
 * Initialize the mode capsule "self" with NULL pointers and default change_type.
 */
static int py_mode_capsule_init(mode_capsule_struct_t* self, PyObject* args, PyObject* kwds) {
  self->mode = NULL;
  self->lf_self = NULL;
  self->change_type = 0;
  return 0;
}

/**
 * Called when an mode capsule in Python is deallocated (generally
 * called by the Python grabage collector).
 * @param self
 */
void py_mode_capsule_dealloc(mode_capsule_struct_t* self) {
  Py_XDECREF(self->mode);
  Py_XDECREF(self->lf_self);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

/*
 * The members of a mode_capsule that are accessible from a Python program, used to define
 * a native Python type.
 */
PyMemberDef py_mode_capsule_members[] = {
    {"mode", T_OBJECT, offsetof(mode_capsule_struct_t, mode), 0, "The pointer to the C mode struct"},
    {"lf_self", T_OBJECT, offsetof(mode_capsule_struct_t, lf_self), 0, "Pointer to LF self"},
    {NULL} /* Sentinel */
};

/*
 * The definition of mode_capsule type object, which is
 * used to describe how mode_capsule behaves.
 */
static PyTypeObject mode_capsule_t = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "LinguaFranca.mode_capsule",
    .tp_doc = "mode_capsule objects",
    .tp_basicsize = sizeof(mode_capsule_struct_t),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = py_mode_capsule_new,
    .tp_init = (initproc)py_mode_capsule_init,
    .tp_dealloc = (destructor)py_mode_capsule_dealloc,
    .tp_members = py_mode_capsule_members,
    .tp_methods = mode_capsule_methods,
};

///////////////// Functions used in mode creation and initialization /////////////

/**
 * @brief Initialize `mode_capsule_t` in the `current_module`.
 *
 */
void initialize_mode_capsule_t(PyObject* current_module) {
  // Initialize the mode_capsule type
  if (PyType_Ready(&mode_capsule_t) < 0) {
    return;
  }

  // Add the mode_capsule type to the module's dictionary.
  Py_INCREF(&mode_capsule_t);
  if (PyModule_AddObject(current_module, "mode_capsule", (PyObject*)&mode_capsule_t) < 0) {
    Py_DECREF(&mode_capsule_t);
    Py_DECREF(current_module);
    lf_print_error_and_exit("Failed to initialize mode_capsule.");
    return;
  }
}

/**
 * Convert a `reactor_mode_t` to a `mode_capsule_t`.
 */
PyObject* convert_C_mode_to_py(reactor_mode_t* mode, self_base_t* lf_self, lf_mode_change_type_t change_type) {
  // Create the mode struct in Python
  mode_capsule_struct_t* cap = (mode_capsule_struct_t*)PyObject_New(mode_capsule_struct_t, &mode_capsule_t);

  if (cap == NULL) {
    lf_print_error_and_exit("Failed to convert mode.");
  }
  Py_INCREF(cap);

  // Create the capsule to hold the reactor_mode_t* mode
  PyObject* capsule = PyCapsule_New(mode, "mode", NULL);
  if (capsule == NULL) {
    lf_print_error_and_exit("Failed to convert mode.");
  }
  Py_INCREF(capsule);
  // Fill in the Python mode struct.
  cap->mode = capsule;

  // Create a capsule to point to the self struct.
  PyObject* self_capsule = PyCapsule_New(lf_self, "lf_self", NULL);
  if (self_capsule == NULL) {
    lf_print_error_and_exit("Failed to convert self.");
  }
  Py_INCREF(self_capsule);
  cap->lf_self = self_capsule;

  cap->change_type = change_type;

  return (PyObject*)cap;
}
