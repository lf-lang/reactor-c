/**
 * @file trace.h
 * @brief API for the tracing module that records runtime events for debugging and analysis.
 * @ingroup Tracing
 *
 * @author Edward A. Lee
 * @author Peter Donovan
 *
 * This header file defines the API for the tracing module, which records
 * runtime events for debugging and analysis purposes. The tracing module
 * maintains a table of objects and their descriptions, and records events
 * with timestamps and other metadata.
 */

#ifndef TRACE_H
#define TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "lf_core_version.h"

/**
 * @brief Return a description of the compile-time properties of the current plugin.
 * @ingroup Tracing
 */
const version_t* lf_version_tracing();

/**
 * @brief Identifier for what is in the object table.
 * @ingroup Tracing
 */
typedef enum {
  /** @brief A reactor instance, identified by its self struct pointer. */
  trace_reactor,

  /** @brief A trigger (timer or action), identified by its trigger pointer. */
  trace_trigger,

  /** @brief A user-defined trace object. */
  trace_user
} _lf_trace_object_t;

/**
 * @brief Struct for table of pointers to a description of the object.
 * @ingroup Tracing
 */
typedef struct object_description_t object_description_t;
struct object_description_t {
  /** @brief Pointer-sized value that uniquely identifies the object. */
  void* pointer;

  /** @brief Pointer to the trigger (action or timer) or other secondary ID. */
  void* trigger;

  /** @brief The type of trace object. */
  _lf_trace_object_t type;

  /** @brief A NULL terminated string describing the object. */
  char* description;
};

/**
 * @brief Trace record without dependencies.
 * @ingroup Tracing
 */
typedef struct {
  int event_type;
  void* pointer;
  int src_id;
  int dst_id;
  int64_t logical_time;
  int64_t microstep;
  int64_t physical_time;
  void* trigger;
  int64_t extra_delay;
} trace_record_nodeps_t;

/**
 * @brief Initialize the tracing module.
 * @ingroup Tracing
 *
 * Calling other API functions before calling this procedure is undefined behavior.
 *
 * @param process_name The name of the current federate, or a placeholder if this is not a federate.
 * @param process_names The names of all federates, separated by commas, or NULL
 * if that information is not available.
 * @param process_id The ID of the current federate, or -1 if this is the RTI. 0
 * if unfederated.
 * @param max_num_local_threads An upper bound on the number of threads created
 * by this process.
 */
void lf_tracing_global_init(char* process_name, char* process_names, int process_id, int max_num_local_threads);

/**
 * @brief Register a kind of trace event.
 * @ingroup Tracing
 *
 * This should be called before tracepoints are reached.
 *
 * @param description A description of some trace events which may be received in the future.
 * This may be invoked after many tracepoints have already been recorded but should be invoked early.
 */
void lf_tracing_register_trace_event(object_description_t description);

/**
 * @brief Give the tracing module access to the start time.
 * @ingroup Tracing
 *
 * This may be invoked after many tracepoints have already been recorded but should be invoked early.
 */
void lf_tracing_set_start_time(int64_t start_time);

/**
 * @brief Submit a tracepoint from the given worker to the tracing module.
 * @ingroup Tracing
 */
void lf_tracing_tracepoint(int worker, trace_record_nodeps_t* tr);

/**
 * @brief Shut down the tracing module.
 * @ingroup Tracing
 *
 * Calling other API functions after calling this procedure is undefined behavior.
 */
void lf_tracing_global_shutdown();

#ifdef __cplusplus
}
#endif

#endif // TRACE_H
