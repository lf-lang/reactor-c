/**
 * @file
 * @author Soroush Bateni
 * @author Hou Seng Wong
 *
 * @brief Definitions for Python federated extensions.
 */

#ifndef PYTHON_CAPSULE_EXTENSION_H
#define PYTHON_CAPSULE_EXTENSION_H

#ifdef FEDERATED
#ifdef FEDERATED_DECENTRALIZED
#define FEDERATED_GENERIC_EXTENSION                                                                                    \
  tag_t intended_tag;                                                                                                  \
  instant_t physical_time_of_arrival;

#define FEDERATED_CAPSULE_EXTENSION                                                                                    \
  py_tag_t* intended_tag;                                                                                              \
  instant_t physical_time_of_arrival;

#define FEDERATED_CAPSULE_MEMBER                                                                                       \
  {"intended_tag", T_OBJECT, offsetof(generic_port_capsule_struct, intended_tag), READONLY,                            \
   "Original intended tag of the event."},                                                                             \
      {"physical_time_of_arrival", T_LONG, offsetof(generic_port_capsule_struct, physical_time_of_arrival), READONLY,  \
       "Physical time of arrival of the original message."},

#define FEDERATED_ASSIGN_FIELDS(py_port, c_port)                                                                       \
  do {                                                                                                                 \
    py_port->intended_tag = convert_C_tag_to_py(c_port->intended_tag);                                                 \
    py_port->physical_time_of_arrival = c_port->physical_time_of_arrival;                                              \
  } while (0)

#else // FEDERATED_CENTRALIZED
#define FEDERATED_GENERIC_EXTENSION instant_t physical_time_of_arrival;

#define FEDERATED_CAPSULE_EXTENSION FEDERATED_GENERIC_EXTENSION

#define FEDERATED_CAPSULE_MEMBER                                                                                       \
  {"physical_time_of_arrival", T_INT, offsetof(generic_port_capsule_struct, physical_time_of_arrival), READONLY,       \
   "Physical time of arrival of the original message."},

#define FEDERATED_ASSIGN_FIELDS(py_port, c_port)                                                                       \
  do {                                                                                                                 \
    py_port->physical_time_of_arrival = c_port->physical_time_of_arrival;                                              \
  } while (0)
#endif                                            // FEDERATED_DECENTRALIZED
#else                                             // not FEDERATED
#define FEDERATED_GENERIC_EXTENSION               // Empty
#define FEDERATED_CAPSULE_EXTENSION               // Empty
#define FEDERATED_CAPSULE_MEMBER                  // Empty
#define FEDERATED_ASSIGN_FIELDS(py_port, c_port)  // Empty
#define FEDERATED_COPY_FIELDS(py_port1, py_port2) // Empty
#endif                                            // FEDERATED

#endif
