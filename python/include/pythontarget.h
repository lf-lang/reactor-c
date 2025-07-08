/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Target-specific runtime functions for the Python target language.
 *
 * Note for target language developers:
 * This is one way of developing a target language where
 * the C core runtime is adopted. This file is a translation layer that implements Lingua Franca
 * APIs which interact with the lf_set and lf_schedule APIs. This file can act as a
 * template for future runtime developement for target languages.
 * For source generation, see xtext/org.icyphy.linguafranca/src/org/icyphy/generator/PythonGenerator.xtend.
 */

#ifndef PYTHON_TARGET_H
#define PYTHON_TARGET_H

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <structmember.h>
#include <limits.h>
#include "python_tag.h"
#include "python_port.h"
#include "python_action.h"

#ifdef _MSC_VER
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#endif

// MODULE_NAME is expected to be defined in the main file of the generated code
#ifndef MODULE_NAME
#error "MODULE_NAME is undefined"
#endif

#define CONCAT(x, y) x##y
#define GEN_NAME(x, y) CONCAT(x, y)
#define STRINGIFY(X) #X
#define TOSTRING(x) STRINGIFY(x)

////////////// Global variables ///////////////
extern PyObject* globalPythonModule;
extern PyObject* globalPythonModuleDict;
extern PyObject* global_pickler;
extern environment_t* top_level_environment;

//////////////////////////////////////////////////////////////
/////////////  schedule Functions (to schedule an action)

/**
 * @brief Schedule an action to occur with the specified time offset and (optional) value.
 *
 * This function is callable in Python by calling action_name.schedule(offset, value).
 * @param self Pointer to the calling Python object, an action.
 * @param args contains:
 *      - offset: The time offset over and above that in the action.
 *      - value: Optional value to be conveyed.
 */
PyObject* py_schedule(PyObject* self, PyObject* args);

//////////////////////////////////////////////////////////////
/////////////  Python Helper Functions (called from Python code)

/**
 * @brief Stop execution at the conclusion of the current logical time.
 * @param self The calling Python object
 * @param args Empty
 * @return Py_None
 */
PyObject* py_request_stop(PyObject* self, PyObject* args);

/**
 * @brief Return the source directory path (where the main .lf file is) as a string.
 * @param self The lf object.
 * @param args Empty.
 * @return PyObject* A Python string.
 */
PyObject* py_source_directory(PyObject* self, PyObject* args);

/**
 * @brief Return the root project directory path as a string.
 * @param self The lf object.
 * @param args Empty.
 * @return PyObject* A Python string.
 */
PyObject* py_package_directory(PyObject* self, PyObject* args);

/**
 * @brief Check whether the deadline of the currently executing reaction has passed.
 *
 * If the deadline has passed and invoke_deadline_handler is True,
 * invoke the deadline handler, if there is one.
 *
 * @param self The Python object of the reactor.
 * @param args contains:
 *      - invoke_deadline_handler: Whether to invoke the deadline handler if the deadline has passed.
 * @return True if the deadline has passed and a deadline handler is invoked, False otherwise.
 */
PyObject* py_check_deadline(PyObject* self, PyObject* args);

//////////////////////////////////////////////////////////////
///////////// Main function callable from Python code

/**
 * The main function of this Python module.
 *
 * @param py_args A single object, which should be a list
 *  of arguments taken from sys.argv().
 */
PyObject* py_main(PyObject* self, PyObject* args);

//////////////////////////////////////////////////////////////
/////////////  Python Helper Functions

/**
 * @brief Convert a C port to a Python port capsule.
 *
 * This function is called any time a Python reaction is called with
 * ports as inputs and outputs. This function converts ports that are
 * either a multiport or a non-multiport into a port_capsule.
 *
 * First, the void* pointer is stored in a PyCapsule. If the port is not
 * a multiport, the value and is_present fields are copied verbatim. These
 * feilds then can be accessed from the Python code as port.value and
 * port.is_present. If the value is absent, it will be set to None.
 *
 * For multiports, the value of the port_capsule (i.e., port.value) is always
 * set to None and is_present is set to false.
 * Individual ports can then later be accessed in Python code as port[idx].
 */
PyObject* convert_C_port_to_py(void* port, int width);

/**
 * @brief Convert a C action to a Python action capsule.
 *
 * Python actions have the following fields (for more information @see generic_action_capsule_struct):
 * *  PyObject* action;
 * *  PyObject* value;
 * *  bool is_present;
 *
 * The input to this function is a pointer to a C action, which might or
 * might not contain a value and an is_present field. To simplify the assumptions
 * made by this function, the "value" and "is_present" fields are passed to the function
 * instead of expecting them to exist.
 *
 * The void* pointer to the C action instance is encapsulated in a PyCapsule instead of passing an exposed pointer
 *through Python. @see https://docs.python.org/3/c-api/capsule.html This encapsulation is done by calling
 *PyCapsule_New(action, "name_of_the_container_in_the_capsule", NULL), where "name_of_the_container_in_the_capsule" is
 *an agreed-upon container name inside the capsule. This capsule can then be treated as a PyObject* and safely passed
 *through Python code. On the other end (which is in schedule functions), PyCapsule_GetPointer(recieved_action,"action")
 *can be called to retrieve the void* pointer into recieved_action.
 */
PyObject* convert_C_action_to_py(void* action);

/**
 * @brief Get a Python function from a reactor instance.
 *
 * @param module The Python module name (e.g. "__main__")
 * @param class The class name
 * @param instance_id The instance ID
 * @param func The function name to get
 * @return The Python function object, or NULL if not found
 */
PyObject* get_python_function(string module, string class, int instance_id, string func);

/**
 * @brief Get a Python reactor instance by its module, class name, and instance ID.
 *
 * @param module The Python module name (e.g. "__main__")
 * @param class The class name
 * @param instance_id The instance ID
 * @return The Python reactor instance, or NULL if not found
 */
PyObject* get_python_instance(string module, string class, int instance_id);

/**
 * @brief Set a python field to a hold a C pointer.
 *
 * @param module The module name.
 * @param class The class name of the Python object.
 * @param instance_id The instance ID of the Python object.
 * @param field The field name to set.
 * @param pointer The pointer for the field to hold.
 * @return int 0 if successful.
 */
int set_python_field_to_c_pointer(string module, string class, int instance_id, string field, void* pointer);

/**
 * @brief Load the Serializer class from package name.
 *
 * @param package_name Name of the python package to load
 * @return Initialized Serializer class
 */
PyObject* load_serializer(string package_name);

/**
 * @brief Serialize Python object to a bytes object using external serializer.
 *
 * @param obj The Python object to serialize
 * @param custom_serializer The custom Serializer class
 * @return Serialized Python bytes object
 */
PyObject* custom_serialize(PyObject* obj, PyObject* custom_serializer);

/**
 * @brief Deserialize Python object from a bytes object using external serializer.
 *
 * @param serialized_pyobject The serialized bytes Python object
 * @param custom_serializer The custom Serializer class
 * @return Deserialized Python object
 */
PyObject* custom_deserialize(PyObject* serialized_pyobject, PyObject* custom_serializer);

/**
 * @brief Initialize the Python module.
 *
 * The Python runtime will call this function to initialize the module.
 * The name of this function is dynamically generated to follow
 * the requirement of PyInit_MODULE_NAME. Since the MODULE_NAME is not
 * known prior to compile time, the GEN_NAME macro is used.
 * The generated function will have the name PyInit_MODULE_NAME.
 * For example for a module named LinguaFrancaFoo, this function
 * will be called PyInit_LinguaFrancaFoo
 */
PyMODINIT_FUNC GEN_NAME(PyInit_, MODULE_NAME)(void);

#endif // PYTHON_TARGET_H
