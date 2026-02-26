/**
 * @file tracepoint.h
 * @brief Definitions of tracepoint functions for use with the C code generator and any other code generator that uses
 * the C infrastructure (such as the Python code generator).
 * @ingroup Internal
 *
 * @author Edward A. Lee
 * @author Peter Donovan
 *
 * See: https://www.lf-lang.org/docs/handbook/tracing?target=c
 *
 * The trace file is named trace.lft and is a binary file with the following format:
 *
 * Header:
 * * instant_t: The start time. This is both the starting physical time and the starting logical time.
 * * int: Size N of the table mapping pointers to descriptions.
 * This is followed by N records each of which has:
 * * A pointer value (the key).
 * * A null-terminated string (the description).
 *
 * Traces:
 * A sequence of traces, each of which begins with an int giving the length of the trace
 * followed by binary representations of the trace_record struct written using fwrite().
 */

/// \cond INTERNAL  // Doxygen conditional.
#ifdef RTI_TRACE
#define LF_TRACE
#endif

#ifndef TRACEPOINT_H
#define TRACEPOINT_H
/// \endcond INTERNAL  // Doxygen conditional.

#include "lf_types.h"
#include <stdio.h>

#ifdef FEDERATED
#include "net_common.h"
#endif // FEDERATED

#include "trace_types.h"

#ifdef LF_TRACE

#include "trace.h"

/**
 * @brief A trace record that gets written in binary to the trace file in the default implementation.
 * @ingroup Internal
 */
typedef struct trace_record_t {
  /** The type of event being traced (e.g., reaction start, reaction end, etc.) */
  trace_event_t event_type;

  /** Pointer identifying the record, typically points to the self struct of a reactor */
  void* pointer;

  /** The ID number of the source (e.g., worker thread or federate ID) or -1 if not applicable */
  int src_id;

  /** The ID number of the destination (e.g., reaction number or federate ID) or -1 if not applicable */
  int dst_id;

  /** The logical time at which the event occurred */
  instant_t logical_time;

  /** The microstep at which the event occurred */
  microstep_t microstep;

  /** The physical time at which the event occurred */
  instant_t physical_time;

  /** Pointer to the trigger associated with this event, if applicable (e.g., for scheduled events) */
  trigger_t* trigger;

  /** Additional delay specified for the event, if applicable (e.g., for scheduled events) */
  interval_t extra_delay;
} trace_record_t;

/**
 * @brief Pass the provided info to the tracing module.
 * @ingroup Internal
 *
 * @param event_type The kind of tracepoint.
 * @param reactor A pointer used as an opaque ID of the source reactor, if one exists.
 * @param tag The tag associated with the tracepoint.
 * @param worker The worker thread where the tracepoint was reached.
 * @param src_id The ID of the source federate/enclave, if applicable.
 * @param dst_id The ID of the destination federate/enclave, if applicable.
 * @param physical_time The time at which the tracepoint was reached, or NULL if not applicable.
 * @param trigger The trigger, if this tracepoint signifies scheduling of an event.
 * @param extra_delay The delay passed to schedule(), if applicable.
 */
void call_tracepoint(int event_type, void* reactor, tag_t tag, int worker, int src_id, int dst_id,
                     instant_t* physical_time, trigger_t* trigger, interval_t extra_delay);

/**
 * @brief Register a trace object.
 * @ingroup Internal
 *
 * @param pointer1 Pointer that identifies the object, typically to a reactor self struct.
 * @param pointer2 Further identifying pointer, typically to a trigger (action or timer) or NULL if irrelevant.
 * @param type The type of trace object.
 * @param description The human-readable description of the object.
 * @return 1 if successful, 0 if the trace object table is full.
 */
int _lf_register_trace_event(void* pointer1, void* pointer2, _lf_trace_object_t type, char* description);

/**
 * @brief Register a user trace event.
 * @ingroup API
 *
 * This should be called once, providing a pointer to a string
 * that describes a phenomenon being traced. Use the same pointer as the first argument to
 * @ref tracepoint_user_event() and @ref tracepoint_user_value().
 *
 * @param self Pointer to the self struct of the reactor from which we want
 * to trace this event. This pointer is used to get the correct environment and
 * thus the correct logical tag of the event.
 * @param description Pointer to a human-readable description of the event.
 * @return 1 if successful, 0 if the trace object table is full.
 */
int register_user_trace_event(void* self, char* description);

/**
 * @brief Trace the start of a reaction execution.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 * @param reaction Pointer to the reaction_t struct for the reaction.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
#define tracepoint_reaction_starts(env, reaction, worker)                                                              \
  call_tracepoint(reaction_starts, reaction->self, env->current_tag, worker, worker, reaction->number, NULL, NULL,     \
                  reaction->deadline)

/**
 * @brief Trace the end of a reaction execution.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 * @param reaction Pointer to the reaction_t struct for the reaction.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
#define tracepoint_reaction_ends(env, reaction, worker)                                                                \
  call_tracepoint(reaction_ends, reaction->self, env->current_tag, worker, worker, reaction->number, NULL, NULL,       \
                  reaction->deadline)

/**
 * @brief Trace a call to schedule.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 * @param trigger Pointer to the trigger_t struct for the trigger.
 * @param extra_delay The extra delay passed to schedule().
 */
void tracepoint_schedule(environment_t* env, trigger_t* trigger, interval_t extra_delay);

/**
 * @brief Trace a user-defined event.
 * @ingroup API
 *
 * Before calling this, you must call
 * @ref register_user_trace_event() with a pointer to the same string
 * or else the event will not be recognized.
 *
 * @param self Pointer to the self struct of the reactor from which we want
 * to trace this event. This pointer is used to get the correct environment and
 * thus the correct logical tag of the event.
 * @param description Pointer to the description string.
 */
void tracepoint_user_event(void* self, char* description);

/**
 * @brief Trace a user-defined event with a value.
 * @ingroup API
 *
 * Before calling this, you must call
 * @ref register_user_trace_event() with a pointer to the same string
 * or else the event will not be recognized.
 *
 * @param self Pointer to the self struct of the reactor from which we want
 * to trace this event. This pointer is used to get the correct environment and
 * thus the correct logical tag of the event.
 * @param description Pointer to the description string.
 * @param value The value of the event. This is a long long for
 *  convenience so that time values can be passed unchanged.
 *  But int values work as well.
 */
void tracepoint_user_value(void* self, char* description, long long value);

/**
 * @brief Trace the start of a worker waiting for something to change on the reaction queue.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
#define tracepoint_worker_wait_starts(env, worker)                                                                     \
  call_tracepoint(worker_wait_starts, NULL, env->current_tag, worker, worker, -1, NULL, NULL, 0)

/**
 * @brief Trace the end of a worker waiting for something to change on the event or reaction queue.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
#define tracepoint_worker_wait_ends(env, worker)                                                                       \
  call_tracepoint(worker_wait_ends, NULL, env->current_tag, worker, worker, -1, NULL, NULL, 0)

/**
 * @brief Trace the start of the scheduler waiting for logical time to advance or an event to
 * appear on the event queue.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 */
#define tracepoint_scheduler_advancing_time_starts(env)                                                                \
  call_tracepoint(scheduler_advancing_time_starts, NULL, env->current_tag, -1, -1, -1, NULL, NULL, 0);

/**
 * @brief Trace the end of the scheduler waiting for logical time to advance or an event to
 * appear on the event queue.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 */
#define tracepoint_scheduler_advancing_time_ends(env)                                                                  \
  call_tracepoint(scheduler_advancing_time_ends, NULL, env->current_tag, -1, -1, -1, NULL, NULL, 0)

/**
 * @brief Trace the occurrence of a deadline miss.
 * @ingroup Internal
 *
 * @param env The environment in which we are executing
 * @param reaction Pointer to the reaction_t struct for the reaction.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
#define tracepoint_reaction_deadline_missed(env, reaction, worker)                                                     \
  call_tracepoint(reaction_deadline_missed, reaction->self, env->current_tag, worker, worker, reaction->number, NULL,  \
                  NULL, 0)

/**
 * @brief Check if the tracing library is compatible with the current version of the runtime.
 * @ingroup Internal
 */
void lf_tracing_check_version();

////////////////////////////////////////////////////////////
//// For federated execution

#if defined(FEDERATED) || defined(LF_ENCLAVES)

/**
 * @brief Trace federate sending a message to the RTI.
 * @ingroup Federated
 *
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_federate_to_rti(trace_event_t event_type, int fed_id, tag_t* tag);

/**
 * @brief Trace federate receiving a message from the RTI.
 * @ingroup Federated
 *
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param tag Pointer to the tag that has been received, or NULL.
 */
void tracepoint_federate_from_rti(trace_event_t event_type, int fed_id, tag_t* tag);

/**
 * @brief Trace federate sending a message to another federate.
 * @ingroup Federated
 *
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param partner_id The partner federate identifier.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_federate_to_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t* tag);

/**
 * @brief Trace federate receiving a message from another federate.
 * @ingroup Federated
 *
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param partner_id The partner federate identifier.
 * @param tag Pointer to the tag that has been received, or NULL.
 */
void tracepoint_federate_from_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t* tag);

#else
static inline void tracepoint_federate_to_rti(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
static inline void tracepoint_federate_from_rti(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
static inline void tracepoint_federate_to_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)partner_id;
  (void)tag;
}
static inline void tracepoint_federate_from_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)partner_id;
  (void)tag;
}
#endif // FEDERATED

////////////////////////////////////////////////////////////
//// For RTI execution

#ifdef RTI_TRACE

/**
 * @brief Trace RTI sending a message to a federate.
 * @ingroup RTI
 *
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The fedaerate ID.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_rti_to_federate(trace_event_t event_type, int fed_id, tag_t* tag);

/**
 * @brief Trace RTI receiving a message from a federate.
 * @ingroup RTI
 *
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The fedaerate ID.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_rti_from_federate(trace_event_t event_type, int fed_id, tag_t* tag);

#else
static inline void tracepoint_rti_to_federate(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
static inline void tracepoint_rti_from_federate(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
#endif // RTI_TRACE

#else
typedef struct trace_t trace_t;
static inline int register_user_trace_event(void* self, char* description) {
  (void)self;
  (void)description;
  return 0;
}
static inline void tracepoint_schedule(environment_t* env, trigger_t* trigger, interval_t extra_delay) {
  (void)env;
  (void)trigger;
  (void)extra_delay;
}
static inline void tracepoint_user_event(void* self, char* description) {
  (void)self;
  (void)description;
}
static inline void tracepoint_user_value(void* self, char* description, long long value) {
  (void)self;
  (void)description;
  (void)value;
}
static inline void tracepoint_rti_to_federate(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
static inline void tracepoint_rti_from_federate(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
static inline void tracepoint_federate_to_rti(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
static inline void tracepoint_federate_from_rti(trace_event_t event_type, int fed_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)tag;
}
static inline void tracepoint_federate_to_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)partner_id;
  (void)tag;
}
static inline void tracepoint_federate_from_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t* tag) {
  (void)event_type;
  (void)fed_id;
  (void)partner_id;
  (void)tag;
}

/// \cond INTERNAL  // Doxygen conditional.
// The following is defined in trace.h, so ask Doxygen to ignore this.

static inline void lf_tracing_global_init(char* process_name, char* process_names, int process_id,
                                          int max_num_local_threads) {
  (void)process_name;
  (void)process_names;
  (void)process_id;
  (void)max_num_local_threads;
}
static inline void lf_tracing_global_shutdown() {}
static inline void lf_tracing_set_start_time(int64_t start_time) { (void)start_time; }

/// \endcond // INTERNAL

#define tracepoint_reaction_starts(env, reaction, worker)                                                              \
  while (0) {                                                                                                          \
    (void)env;                                                                                                         \
    (void)reaction;                                                                                                    \
    (void)worker;                                                                                                      \
  }
#define tracepoint_reaction_ends(env, reaction, worker)                                                                \
  while (0) {                                                                                                          \
    (void)env;                                                                                                         \
    (void)reaction;                                                                                                    \
    (void)worker;                                                                                                      \
  }
#define tracepoint_worker_wait_starts(env, worker)                                                                     \
  while (0) {                                                                                                          \
    (void)env;                                                                                                         \
    (void)worker;                                                                                                      \
  }
#define tracepoint_worker_wait_ends(env, worker)                                                                       \
  while (0) {                                                                                                          \
    (void)env;                                                                                                         \
    (void)worker;                                                                                                      \
  }
#define tracepoint_scheduler_advancing_time_starts(env)                                                                \
  while (0) {                                                                                                          \
    (void)env;                                                                                                         \
  }
#define tracepoint_scheduler_advancing_time_ends(env)                                                                  \
  while (0) {                                                                                                          \
    (void)env;                                                                                                         \
  }
#define tracepoint_reaction_deadline_missed(env, reaction, worker)                                                     \
  while (0) {                                                                                                          \
    (void)env;                                                                                                         \
    (void)reaction;                                                                                                    \
    (void)worker;                                                                                                      \
  }

#endif // LF_TRACE
/// \cond INTERNAL  // Doxygen conditional.
#endif // TRACEPOINT_H
       /// \endcond INTERNAL  // Doxygen conditional.