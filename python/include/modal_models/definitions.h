/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief Definitions needed for the modal models in the Python target.
 */

#ifndef PYTHON_MODAL_MODELS_DEFS_H
#define PYTHON_MODAL_MODELS_DEFS_H

#ifdef MODAL_REACTORS
#include <Python.h>
#include <structmember.h>
#include "tag.h"
#include "../include/api/schedule.h"

/**
 * The struct used to represent modes in Python.
 * An instance of this struct is created when entering a reaction that
 * has declared a mode as an effect. This struct represents everything
 * needed to take a transition to that mode, including a pointer to
 * that mode and the type of transition (reset or history).
 */
typedef struct {
  PyObject_HEAD PyObject* mode;
  PyObject* lf_self;
  lf_mode_change_type_t change_type;
} mode_capsule_struct_t;

/**
 * Set a new mode for a modal model.
 */
static PyObject* py_mode_set(PyObject* self, PyObject* args);

/**
 * Convert a `reactor_mode_t` to a `mode_capsule_t`.
 */
PyObject* convert_C_mode_to_py(reactor_mode_t* mode, self_base_t* lf_self, lf_mode_change_type_t change_type);

/**
 * @brief Initialize `mode_capsule_t` in the `current_module`.
 *
 */
void initialize_mode_capsule_t(PyObject* current_module);

#else
#define initialize_mode_capsule_t(...)
#endif // MODAL_REACTORS

#endif // PYTHON_MODAL_MODELS_DEFS_H
