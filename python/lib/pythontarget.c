/**
 * @file
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Implementation of functions defined in @see pythontarget.h
 */

#include "pythontarget.h"
#include "modal_models/definitions.h"
#include "platform.h" // defines MAX_PATH on Windows
#include <stdlib.h>
#include <string.h>
#include "python_action.h"
#include "python_port.h"
#include "python_tag.h"
#include "python_time.h"
#include "reactor.h"
#include "tag.h"
#include "util.h"
#include "environment.h"
#include "api/schedule.h"
#include "tracepoint.h"

////////////// Global variables ///////////////
// The global Python object that holds the .py module that the
// C runtime interacts with
PyObject* globalPythonModule = NULL;

// The dictionary of the Python module that is used to load
// class objects from
PyObject* globalPythonModuleDict = NULL;

// Import pickle to enable native serialization
PyObject* global_pickler = NULL;

environment_t* top_level_environment = NULL;

//////////// schedule Function(s) /////////////

PyObject* py_schedule(PyObject* self, PyObject* args) {
  generic_action_capsule_struct* act = (generic_action_capsule_struct*)self;
  long long offset;
  PyObject* value = NULL;

  if (!PyArg_ParseTuple(args, "L|O", &offset, &value))
    return NULL;

  lf_action_base_t* action = (lf_action_base_t*)PyCapsule_GetPointer(act->action, "action");
  if (action == NULL) {
    lf_print_error("Null pointer received.");
    exit(1);
  }

  trigger_t* trigger = action->trigger;
  environment_t* env = action->parent->environment;
  lf_token_t* t = NULL;

  LF_CRITICAL_SECTION_ENTER(env);

  // Check to see if value exists
  if (value) {
    // Allocate a fresh token for this schedule call rather than routing through
    // _lf_initialize_token_with_value / _lf_get_token. Those paths may reuse
    // or replace trigger->tmplt.token, which races with the reaction prologue
    // that reads trigger->tmplt.token->value after an event pop:
    //
    //   1. The scheduler pops an event, sets trigger->tmplt.token = T, and
    //      drops T->ref_count to 1 before releasing the environment lock.
    //   2. A concurrent schedule acquires the environment lock and enters
    //      _lf_get_token, which sees ref_count == 1 and reuses T, freeing
    //      its payload and overwriting it with the new value.
    //   3. The pending reaction finally runs and reads the corrupted value.
    //
    // Allocating a fresh token that lives only on the event queue until
    // _lf_pop_events installs it into the template means schedule paths
    // never write to trigger->tmplt.token, so concurrent schedulers cannot
    // corrupt the payload of a token about to be consumed by a reaction.
    trigger->tmplt.type.element_size = sizeof(PyObject*);
    t = lf_new_token((void*)&trigger->tmplt, value, 1);
#if !defined NDEBUG
    // Keep the payload allocation counter balanced with the decrement that
    // occurs when the token's value is eventually freed.
    LF_CRITICAL_SECTION_ENTER(GLOBAL_ENVIRONMENT);
    extern int _lf_count_payload_allocations;
    _lf_count_payload_allocations++;
    LF_CRITICAL_SECTION_EXIT(GLOBAL_ENVIRONMENT);
#endif

    // Also give the new value back to the Python action itself
    Py_INCREF(value);
    act->value = value;
  }

  lf_schedule_trigger(env, trigger, offset, t);
  lf_notify_of_event(env);

  LF_CRITICAL_SECTION_EXIT(env);

  // FIXME: handle is not passed to the Python side
  Py_INCREF(Py_None);
  return Py_None;
}

/**
 * Prototype for the main function.
 */
int lf_reactor_c_main(int argc, const char* argv[]);

/**
 * Prototype for lf_request_stop().
 * @see reactor.h
 */
void lf_request_stop(void);

///////////////// Other useful functions /////////////////////
/**
 * Stop execution at the conclusion of the current logical time.
 */
PyObject* py_request_stop(PyObject* self, PyObject* args) {
  lf_request_stop();

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* py_source_directory(PyObject* self, PyObject* args) {
#ifndef LF_SOURCE_DIRECTORY
  // This should not occur.
  PyErr_SetString(PyExc_RuntimeError, "LF_SOURCE_DIRECTORY constant is not defined.");
  return NULL;
#else
  return PyUnicode_DecodeFSDefault(LF_SOURCE_DIRECTORY);
#endif
}

PyObject* py_package_directory(PyObject* self, PyObject* args) {
#ifndef LF_PACKAGE_DIRECTORY
  // This should not occur.
  PyErr_SetString(PyExc_RuntimeError, "LF_PACKAGE_DIRECTORY constant is not defined.");
  return NULL;
#else
  return PyUnicode_DecodeFSDefault(LF_PACKAGE_DIRECTORY);
#endif
}

/**
 * Parse Python's 'argv' (from sys.argv()) into a pair of C-style
 * 'argc' (the size of command-line parameters array)
 * and 'argv' (an array of char* containing the command-line parameters).
 *
 * This function assumes that argc is already allocated, and will fail if it
 * isn't.
 *
 * @param py_argv The returned value by 'sys.argv()'
 * @param argc Will contain an integer which is the number of arguments
 *  passed on the command line.
 * @return A list of char*, where each item contains an individual
 *  command-line argument.
 */
const char** _lf_py_parse_argv_impl(PyObject* py_argv, size_t* argc) {
  if (argc == NULL) {
    lf_print_error_and_exit("_lf_py_parse_argv_impl called with an unallocated argc argument.");
  }

  // List of arguments
  const char** argv;

  // Read the optional argvs
  PyObject* py_argv_parsed = NULL;

  if (!PyArg_ParseTuple(py_argv, "|O", &py_argv_parsed)) {
    PyErr_SetString(PyExc_TypeError, "Could not get argvs.");
    return NULL;
  }

  if (py_argv_parsed == NULL) {
    // Build a generic argv with just one argument, which
    // is the module name.
    *argc = 1;
    argv = malloc(2 * sizeof(char*));
    argv[0] = TOSTRING(MODULE_NAME);
    argv[1] = NULL;
    return argv;
  }

  Py_ssize_t argv_size = PyList_Size(py_argv_parsed);
  argv = malloc(argv_size * sizeof(char*));
  for (Py_ssize_t i = 0; i < argv_size; i++) {
    PyObject* list_item = PyList_GetItem(py_argv_parsed, i);
    if (list_item == NULL) {
      if (PyErr_Occurred()) {
        PyErr_Print();
      }
      lf_print_error_and_exit("Could not get argv list item %zd.", i);
    }

    PyObject* encoded_string = PyUnicode_AsEncodedString(list_item, "UTF-8", "strict");
    if (encoded_string == NULL) {
      if (PyErr_Occurred()) {
        PyErr_Print();
      }
      lf_print_error_and_exit("Failed to encode argv list item %zd.", i);
    }

    argv[i] = PyBytes_AsString(encoded_string);

    if (PyErr_Occurred()) {
      PyErr_Print();
      lf_print_error_and_exit("Could not convert argv list item %zd to char*.", i);
    }
  }
  *argc = argv_size;
  return argv;
}

static bool py_initialized = false;

/**
 * @brief Initialize the Python interpreter if it hasn't already been.
 */
void py_initialize_interpreter(void) {
  if (!py_initialized) {
    py_initialized = true;

    // Initialize the Python interpreter
    Py_Initialize();

    LF_PRINT_DEBUG("Initialized the Python interpreter.");
  }
}

/**
 * @brief Get the lf_self pointer from the Python object for a reactor.
 *
 * @param self The Python object for the reactor.
 * @return void* The lf_self pointer or NULL if it is not found.
 */
void* get_lf_self_pointer(PyObject* self) {
  // Get the lf_self pointer from the Python object
  PyObject* py_lf_self = PyObject_GetAttrString(self, "lf_self");
  if (py_lf_self == NULL) {
    PyErr_SetString(PyExc_AttributeError, "lf_self attribute not found");
    return NULL;
  }
  // Convert the Python long to a void pointer
  void* self_ptr = PyLong_AsVoidPtr(py_lf_self);
  Py_DECREF(py_lf_self);
  if (self_ptr == NULL) {
    PyErr_SetString(PyExc_ValueError, "Invalid lf_self pointer");
    return NULL;
  }
  return self_ptr;
}

PyObject* py_check_deadline(PyObject* self, PyObject* args) {
  PyObject* py_self;
  int invoke_deadline_handler = 1; // Default to True

  if (!PyArg_ParseTuple(args, "O|p", &py_self, &invoke_deadline_handler)) {
    return NULL;
  }
  void* self_ptr = get_lf_self_pointer(py_self);
  if (self_ptr == NULL) {
    return NULL;
  }
  bool result = lf_check_deadline(self_ptr, invoke_deadline_handler);
  return PyBool_FromLong(result);
}

PyObject* py_update_deadline(PyObject* self, PyObject* args) {
  PyObject* py_self;
  int64_t updated_deadline = 0; // Default to 0
  double updated_deadline_in_double =
      0.0; // Deadline may be passed as a floating-point value in nanoseconds, e.g., SEC(0.5) → 0.5 * 1e9.

  if (!PyArg_ParseTuple(args, "O|d", &py_self, &updated_deadline_in_double)) {
    return NULL;
  }

  // Check overflow before converting a double to int64_t (interval_t).
  if (updated_deadline_in_double > (double)INT64_MAX || updated_deadline_in_double < (double)INT64_MIN) {
    PyErr_SetString(PyExc_OverflowError, "The updated deadline value is out of int64 range");
    return NULL;
  }

  // Convert double to int64_t
  updated_deadline = (int64_t)updated_deadline_in_double;

  void* self_ptr = get_lf_self_pointer(py_self);
  if (self_ptr == NULL) {
    return NULL;
  }
  lf_update_deadline(self_ptr, updated_deadline);

  Py_INCREF(Py_None);
  return Py_None;
}

/**
 * Register a user trace event. Returns an opaque handle (as a Python int)
 * that must be passed to tracepoint_user_event and tracepoint_user_value.
 * When tracing is disabled, returns 0 and tracepoint calls are no-ops.
 */
PyObject* py_register_user_trace_event(PyObject* self, PyObject* args) {
  PyObject* py_self;
  const char* description = NULL;

  if (!PyArg_ParseTuple(args, "Os", &py_self, &description)) {
    return NULL;
  }
  void* self_ptr = get_lf_self_pointer(py_self);
  if (self_ptr == NULL) {
    return NULL;
  }
  size_t len = strlen(description) + 1;
  /* Allocate on the reactor's allocation record so it is freed when the reactor is deallocated. */
  char* desc_copy = (char*)lf_allocate(len, 1, &((self_base_t*)self_ptr)->allocations);
  if (desc_copy == NULL) {
    PyErr_NoMemory();
    return NULL;
  }
  memcpy(desc_copy, description, len);
  int result = register_user_trace_event(self_ptr, desc_copy);
  if (!result) {
    return PyLong_FromLong(0);
  }
  return PyLong_FromVoidPtr(desc_copy);
}

/**
 * Trace a user-defined event. The handle must be the handle
 * returned by register_user_trace_event (an int).
 */
PyObject* py_tracepoint_user_event(PyObject* self, PyObject* args) {
  PyObject* py_self;
  PyObject* handle = NULL;

  if (!PyArg_ParseTuple(args, "OO", &py_self, &handle)) {
    return NULL;
  }
  void* self_ptr = get_lf_self_pointer(py_self);
  if (self_ptr == NULL) {
    return NULL;
  }
  char* desc_ptr = (char*)PyLong_AsVoidPtr(handle);
  if (desc_ptr == NULL && PyErr_Occurred()) {
    return NULL;
  }
  if (desc_ptr != NULL) {
    tracepoint_user_event(self_ptr, desc_ptr);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/**
 * Trace a user-defined event with a value. The handle must be
 * the handle returned by register_user_trace_event (an int).
 */
PyObject* py_tracepoint_user_value(PyObject* self, PyObject* args) {
  PyObject* py_self;
  PyObject* handle = NULL;
  long long value = 0;

  if (!PyArg_ParseTuple(args, "OOL", &py_self, &handle, &value)) {
    return NULL;
  }
  void* self_ptr = get_lf_self_pointer(py_self);
  if (self_ptr == NULL) {
    return NULL;
  }
  char* desc_ptr = (char*)PyLong_AsVoidPtr(handle);
  if (desc_ptr == NULL && PyErr_Occurred()) {
    return NULL;
  }
  if (desc_ptr != NULL) {
    tracepoint_user_value(self_ptr, desc_ptr, value);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

//////////////////////////////////////////////////////////////
///////////// Main function callable from Python code

PyObject* py_main(PyObject* self, PyObject* py_args) {

  LF_PRINT_DEBUG("Initializing main.");

  size_t argc;
  const char** argv = _lf_py_parse_argv_impl(py_args, &argc);

  py_initialize_interpreter();

  // Load the pickle module
  if (global_pickler == NULL) {
    global_pickler = PyImport_ImportModule("pickle");
    if (global_pickler == NULL) {
      if (PyErr_Occurred()) {
        PyErr_Print();
      }
      lf_print_error_and_exit("Failed to load the module 'pickle'.");
    }
  }

  // Store a reference to the top-level environment
  int num_environments = _lf_get_environments(&top_level_environment);
  LF_ASSERT(num_environments == 1, "Python target only supports programs with a single environment/enclave");

  Py_BEGIN_ALLOW_THREADS lf_reactor_c_main(argc, argv);
  Py_END_ALLOW_THREADS

#ifdef LF_TRACE
  // Ensure trace buffers are flushed for Python runs
  lf_tracing_global_shutdown();
#endif

  Py_INCREF(Py_None);
  return Py_None;
}

///// Python Module Built-ins
/**
 * Bind Python function names to the C functions.
 * The name of this struct is dynamically generated because
 * MODULE_NAME is given by the generated code. This struct
 * will be named as MODULE_NAME_methods.
 * For example, for MODULE_NAME=Foo, this struct will
 * be called Foo_methods.
 * start() initiates the main loop in the C core library
 */
static PyMethodDef GEN_NAME(MODULE_NAME, _methods)[] = {
    {"start", py_main, METH_VARARGS, NULL},
    {"tag", py_lf_tag, METH_NOARGS, NULL},
    {"tag_compare", py_tag_compare, METH_VARARGS, NULL},
    {"request_stop", py_request_stop, METH_NOARGS, "Request stop"},
    {"source_directory", py_source_directory, METH_NOARGS, "Source directory path for .lf file"},
    {"package_directory", py_package_directory, METH_NOARGS, "Root package directory path"},
    {"check_deadline", (PyCFunction)py_check_deadline, METH_VARARGS,
     "Check whether the deadline of the currently executing reaction has passed"},
    {"update_deadline", (PyCFunction)py_update_deadline, METH_VARARGS,
     "Update the deadline of the currently executing reaction"},
    {"register_user_trace_event", (PyCFunction)py_register_user_trace_event, METH_VARARGS,
     "Register a user trace event; returns a handle for use with tracepoint_user_event and tracepoint_user_value"},
    {"tracepoint_user_event", (PyCFunction)py_tracepoint_user_event, METH_VARARGS,
     "Trace a user-defined event (pass the handle from register_user_trace_event)"},
    {"tracepoint_user_value", (PyCFunction)py_tracepoint_user_value, METH_VARARGS,
     "Trace a user-defined event with a value (pass the handle from register_user_trace_event)"},
    {NULL, NULL, 0, NULL}};

/**
 * Define the Lingua Franca module.
 * The MODULE_NAME is given by the generated code.
 */
static PyModuleDef MODULE_NAME = {PyModuleDef_HEAD_INIT, TOSTRING(MODULE_NAME), "LinguaFranca Python Module", -1,
                                  GEN_NAME(MODULE_NAME, _methods)};

//////////////////////////////////////////////////////////////
/////////////  Module Initialization

/*
 * The Python runtime will call this function to initialize the module.
 * The name of this function is dynamically generated to follow
 * the requirement of PyInit_MODULE_NAME. Since the MODULE_NAME is not
 * known prior to compile time, the GEN_NAME macro is used.
 * The generated function will have the name PyInit_MODULE_NAME.
 * For example for a module named LinguaFrancaFoo, this function
 * will be called PyInit_LinguaFrancaFoo
 */
PyMODINIT_FUNC GEN_NAME(PyInit_, MODULE_NAME)(void) {

  PyObject* m;

  // As of Python 11, this function may be called before py_main, so we need to
  // initialize the interpreter.
  py_initialize_interpreter();

  // Initialize the port_capsule type
  if (PyType_Ready(&py_port_capsule_t) < 0) {
    return NULL;
  }

  // Initialize the action_capsule type
  if (PyType_Ready(&py_action_capsule_t) < 0) {
    return NULL;
  }

  // Initialize the Tag type
  if (PyType_Ready(&PyTagType) < 0) {
    return NULL;
  }

  // Initialize the Time type
  if (PyType_Ready(&PyTimeType) < 0) {
    return NULL;
  }

  m = PyModule_Create(&MODULE_NAME);

  if (m == NULL) {
    return NULL;
  }

  initialize_mode_capsule_t(m);

  // Add the port_capsule type to the module's dictionary
  Py_INCREF(&py_port_capsule_t);
  if (PyModule_AddObject(m, "port_capsule", (PyObject*)&py_port_capsule_t) < 0) {
    Py_DECREF(&py_port_capsule_t);
    Py_DECREF(m);
    return NULL;
  }

  // Add the action_capsule type to the module's dictionary
  Py_INCREF(&py_action_capsule_t);
  if (PyModule_AddObject(m, "action_capsule_t", (PyObject*)&py_action_capsule_t) < 0) {
    Py_DECREF(&py_action_capsule_t);
    Py_DECREF(m);
    return NULL;
  }

  // Add the Tag type to the module's dictionary
  Py_INCREF(&PyTagType);
  if (PyModule_AddObject(m, "Tag", (PyObject*)&PyTagType) < 0) {
    Py_DECREF(&PyTagType);
    Py_DECREF(m);
    return NULL;
  }

  // Add the Time type to the module's dictionary
  Py_INCREF(&PyTimeType);
  if (PyModule_AddObject(m, "time", (PyObject*)&PyTimeType) < 0) {
    Py_DECREF(&PyTimeType);
    Py_DECREF(m);
    return NULL;
  }
  return m;
}

//////////////////////////////////////////////////////////////
/////////////  Python Helper Functions
/// These functions are called in generated C code for various reasons.
/// Their main purpose is to facilitate C runtime's communication with
/// Python code.
/**
 * A function that destroys action capsules
 **/
void destroy_action_capsule(PyObject* capsule) { free(PyCapsule_GetPointer(capsule, "action")); }

PyObject* convert_C_port_to_py(void* port, int width) {
  // Create the port struct in Python
  PyObject* cap = (PyObject*)PyObject_New(generic_port_capsule_struct, &py_port_capsule_t);
  if (cap == NULL) {
    lf_print_error_and_exit("Failed to convert port.");
  }
  Py_INCREF(cap);

  // Create the capsule to hold the void* port
  PyObject* capsule = PyCapsule_New(port, "port", NULL);
  if (capsule == NULL) {
    lf_print_error_and_exit("Failed to convert port.");
  }
  Py_INCREF(capsule);

  // Fill in the Python port struct
  ((generic_port_capsule_struct*)cap)->port = capsule;
  ((generic_port_capsule_struct*)cap)->width = width;

  if (width == -2) {
    generic_port_instance_struct* cport = (generic_port_instance_struct*)port;
    FEDERATED_ASSIGN_FIELDS(((generic_port_capsule_struct*)cap), cport);

    ((generic_port_capsule_struct*)cap)->is_present = cport->is_present;

    if (cport->value == NULL) {
      // Value is absent
      Py_INCREF(Py_None);
      ((generic_port_capsule_struct*)cap)->value = Py_None;
      return cap;
    }

    // Py_INCREF(cport->value);
    ((generic_port_capsule_struct*)cap)->value = cport->value;
  } else {
    // Multiport. Value of the multiport itself cannot be accessed, so we set it to
    // None.
    Py_INCREF(Py_None);
    ((generic_port_capsule_struct*)cap)->value = Py_None;
    ((generic_port_capsule_struct*)cap)->is_present = false;
  }

  return cap;
}

PyObject* convert_C_action_to_py(void* action) {
  // Convert to trigger_t
  trigger_t* trigger = ((lf_action_base_t*)action)->trigger;

  // Create the action struct in Python
  PyObject* cap = (PyObject*)PyObject_New(generic_action_capsule_struct, &py_action_capsule_t);
  if (cap == NULL) {
    lf_print_error_and_exit("Failed to convert action.");
  }
  Py_INCREF(cap);

  // Create the capsule to hold the void* action
  PyObject* capsule = PyCapsule_New(action, "action", NULL);
  if (capsule == NULL) {
    lf_print_error_and_exit("Failed to convert action.");
  }
  Py_INCREF(capsule);

  // Fill in the Python action struct
  ((generic_action_capsule_struct*)cap)->action = capsule;
  ((generic_action_capsule_struct*)cap)->is_present = trigger->status;
  FEDERATED_ASSIGN_FIELDS(((generic_port_capsule_struct*)cap), ((generic_action_instance_struct*)action));

  // If token is not initialized, that is all we need to set
  if (trigger->tmplt.token == NULL) {
    Py_INCREF(Py_None);
    ((generic_action_capsule_struct*)cap)->value = Py_None;
    return cap;
  }

  // Default value is None
  if (trigger->tmplt.token->value == NULL) {
    Py_INCREF(Py_None);
    trigger->tmplt.token->value = Py_None;
  }

  // Actions in Python always use token type
  if (((generic_action_instance_struct*)action)->token != NULL)
    ((generic_action_capsule_struct*)cap)->value = ((generic_action_instance_struct*)action)->token->value;

  return cap;
}

PyObject* get_python_function(string module, string class, int instance_id, string func) {
  LF_PRINT_DEBUG("Getting Python function %s from %s.%s[%d]", func, module, class, instance_id);

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  // Get the Python instance first
  // NOTE: This also acquires the GIL. OK for this to be nested?
  PyObject* pInstance = get_python_instance(module, class, instance_id);
  if (pInstance == NULL) {
    PyGILState_Release(gstate);
    return NULL;
  }

  // Get the function from the instance
  PyObject* pFunc = PyObject_GetAttrString(pInstance, func);
  Py_DECREF(pInstance); // We don't need the instance anymore

  if (pFunc == NULL) {
    PyErr_Print();
    PyGILState_Release(gstate);
    lf_print_error("Failed to get function %s from instance.", func);
    return NULL;
  }

  // Check if the function is callable
  if (!PyCallable_Check(pFunc)) {
    PyErr_Print();
    lf_print_error("Function %s is not callable.", func);
    Py_DECREF(pFunc);
    PyGILState_Release(gstate);
    return NULL;
  }
  Py_INCREF(pFunc);
  PyGILState_Release(gstate);
  return pFunc;
}

PyObject* get_python_instance(string module, string class, int instance_id) {
  LF_PRINT_DEBUG("Getting Python instance for %s.%s[%d]", module, class, instance_id);

  // Necessary PyObject variables
  PyObject* pFileName = NULL;
  PyObject* pModule = NULL;
  PyObject* pDict = NULL;
  PyObject* pClasses = NULL;
  PyObject* pInstance = NULL;

  // Acquire the GIL
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  // If the Python module is already loaded, skip this.
  if (globalPythonModule == NULL) {
    // Decode the MODULE name into a filesystem compatible string
    pFileName = PyUnicode_DecodeFSDefault(module);

    // Set the Python search path to be the current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      lf_print_error_and_exit("Failed to get the current working directory.");
    }

    wchar_t wcwd[PATH_MAX];
    mbstowcs(wcwd, cwd, PATH_MAX);

    // Initialize Python with custom configuration
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    // Add paths to the configuration
    PyWideStringList_Append(&config.module_search_paths, wcwd);
    // Initialize Python with the custom configuration
    Py_InitializeFromConfig(&config);

    LF_PRINT_DEBUG("Loading module %s in %s.", module, cwd);

    pModule = PyImport_Import(pFileName);
    Py_DECREF(pFileName);

    // Check if the module was correctly loaded
    if (pModule != NULL) {
      pDict = PyModule_GetDict(pModule);
      if (pDict == NULL) {
        PyErr_Print();
        lf_print_error("Failed to load contents of module %s.", module);
        PyGILState_Release(gstate);
        return NULL;
      }

      Py_INCREF(pModule);
      globalPythonModule = pModule;
      Py_INCREF(pDict);
      globalPythonModuleDict = pDict;
    }
  }

  if (globalPythonModuleDict != NULL) {
    // Convert the class name to a PyObject
    PyObject* list_name = PyUnicode_DecodeFSDefault(class);

    // Get the class list
    Py_INCREF(globalPythonModuleDict);
    pClasses = PyDict_GetItem(globalPythonModuleDict, list_name);
    if (pClasses == NULL) {
      PyErr_Print();
      lf_print_error("Failed to load class list \"%s\" in module %s.", class, module);
      Py_DECREF(globalPythonModuleDict);
      PyGILState_Release(gstate);
      return NULL;
    }

    Py_DECREF(globalPythonModuleDict);

    // Get the specific instance from the list
    pInstance = PyList_GetItem(pClasses, instance_id);
    if (pInstance == NULL) {
      PyErr_Print();
      lf_print_error("Failed to load instance \"%s[%d]\" in module %s.", class, instance_id, module);
      PyGILState_Release(gstate);
      return NULL;
    }

    // Increment reference count before returning
    Py_INCREF(pInstance);
    PyGILState_Release(gstate);
    return pInstance;
  }

  PyErr_Print();
  lf_print_error("Failed to load \"%s\".", module);
  PyGILState_Release(gstate);
  return NULL;
}

long lf_py_get_nonnegative_integer_parameter(string module, string instance_name, int instance_id, string param_name) {
  PyGILState_STATE gstate = PyGILState_Ensure();
  long result = -1;

  PyObject* py_inst = get_python_instance(module, instance_name, instance_id);
  if (py_inst == NULL) {
    Py_DECREF(py_inst);
    PyGILState_Release(gstate);
    return -1;
  }

  PyObject* py_param = PyObject_GetAttrString(py_inst, param_name);
  Py_DECREF(py_inst);
  if (py_param == NULL) {
    // Attribute lookup failed; log and clear the Python exception to avoid
    // leaving a pending exception that could affect later code.
    lf_print_error("Could not get Python parameter '%s' from instance '%s'.", param_name, instance_name);
    PyErr_Clear();
    PyGILState_Release(gstate);
    return -1;
  }

  result = PyLong_AsLong(py_param);
  if (PyErr_Occurred()) {
    // Conversion to long failed (e.g., wrong type or overflow). Log and
    // clear the exception, and return an error value.
    lf_print_error("Could not convert Python parameter '%s' of instance '%s' to long.", param_name, instance_name);
    PyErr_Clear();
    result = -1;
  }
  Py_DECREF(py_param);

  PyGILState_Release(gstate);
  return result;
}

int set_python_field_to_c_pointer(string module, string class, int instance_id, string field, void* pointer) {
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  PyObject* py_instance = get_python_instance(module, class, instance_id);
  if (py_instance == NULL) {
    lf_print_error("Could not get Python instance");
    PyGILState_Release(gstate);
    return -1;
  }
  PyObject* ptr = PyLong_FromVoidPtr(pointer);
  if (ptr == NULL) {
    Py_DECREF(py_instance);
    lf_print_error("Could not create Python long from void pointer");
    PyGILState_Release(gstate);
    return -1;
  }
  if (PyObject_SetAttrString(py_instance, field, ptr) < 0) {
    Py_DECREF(ptr);
    Py_DECREF(py_instance);
    lf_print_error("Could not set lf_self attribute");
    PyGILState_Release(gstate);
    return -1;
  }
  Py_DECREF(ptr);
  Py_DECREF(py_instance);
  PyGILState_Release(gstate);
  return 0;
}

PyObject* load_serializer(string package_name) {
  // import package_name
  PyObject* pName = PyUnicode_DecodeFSDefault(package_name);
  PyObject* pModule = PyImport_Import(pName);
  Py_DECREF(pName);
  if (PyErr_Occurred())
    PyErr_Print();
  if (pModule == NULL)
    lf_print_error_and_exit("Could not load the custom serializer package '%s'.", package_name);
  // Get the Serializer class
  PyObject* SerializerClass = PyObject_GetAttrString(pModule, "Serializer");
  if (PyErr_Occurred())
    PyErr_Print();
  if (SerializerClass == NULL)
    lf_print_error_and_exit("Could not find class 'Serializer' in module '%s'.", package_name);
  // Instanciate and initialize Serializer class
  PyObject* custom_serializer = PyObject_CallObject(SerializerClass, NULL);
  if (PyErr_Occurred())
    PyErr_Print();
  if (custom_serializer == NULL)
    lf_print_error_and_exit("Could not instantiate class 'Serializer' in module '%s'.", package_name);
  lf_print_log("Successfully loaded custom serializer package '%s'.\n", package_name);
  return custom_serializer;
}

PyObject* custom_serialize(PyObject* obj, PyObject* custom_serializer) {
  if (custom_serializer == NULL)
    lf_print_error_and_exit("Serializer is null.");
  PyObject* serializer_serialize = PyObject_GetAttrString(custom_serializer, "serialize");
  PyObject* args = PyTuple_Pack(1, obj);
  PyObject* serialized_pyobject = PyObject_CallObject(serializer_serialize, args);
  Py_XDECREF(serializer_serialize);
  Py_XDECREF(args);
  if (PyErr_Occurred())
    PyErr_Print();
  if (serialized_pyobject == NULL)
    lf_print_error_and_exit("Could not serialize object.");
  return serialized_pyobject;
}

PyObject* custom_deserialize(PyObject* serialized_pyobject, PyObject* custom_serializer) {
  if (custom_serializer == NULL)
    lf_print_error_and_exit("Serializer is null.");
  PyObject* serializer_deserialize = PyObject_GetAttrString(custom_serializer, "deserialize");
  PyObject* args = PyTuple_Pack(1, serialized_pyobject);
  PyObject* deserialized_obj = PyObject_CallObject(serializer_deserialize, args);
  Py_XDECREF(serializer_deserialize);
  Py_XDECREF(args);
  if (PyErr_Occurred())
    PyErr_Print();
  if (deserialized_obj == NULL)
    lf_print_error_and_exit("Could not deserialize deserialized_obj.");
  return deserialized_obj;
}