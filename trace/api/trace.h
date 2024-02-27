#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include <stdbool.h>

#include "lf_core_version.h"

/**
 * @brief Return a description of the compile-time properties of the current
 * plugin.
 */
version_t lf_version_tracing();

/**
 * Identifier for what is in the object table.
 */
typedef enum {
    trace_reactor,   // Self struct.
    trace_trigger,   // Timer or action (argument to schedule()).
    trace_user       // User-defined trace object.
} _lf_trace_object_t;

/**
 * Struct for table of pointers to a description of the object.
 */
typedef struct object_description_t object_description_t;
struct object_description_t {
    void* pointer;      // Pointer-sized value that uniquely identifies the object.
    void* trigger;      // Pointer to the trigger (action or timer) or other secondary ID, if any.
    _lf_trace_object_t type;  // The type of trace object.
    char* description; // A NULL terminated string.
};

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
 * @brief Initialize the tracing module. Calling other API functions before
 * calling this procedure is undefined behavior.
 *
 * @param file_name_prefix Prefix to attach to any files that may be produced by
 * the tracing module.
 * @param process_id The ID of the current federate, or -1 if this is the RTI. 0
 * if unfederated.
 * @param max_num_local_threads An upper bound on the number of threads created
 * by this process.
 */
void lf_tracing_global_init(char* file_name_prefix, int process_id, int max_num_local_threads);
/**
 * @brief Register a kind of trace event. This should be called before
 * tracepoints are reached.
 *
 * @param description A description of some trace events which may be received
 * in the future. This may be invoked after many tracepoints have already been
 * recorded but should be invoked early.
 */
void lf_tracing_register_trace_event(object_description_t description);
/**
 * @brief Give the tracing module access to the start time. This may be invoked
 * after many tracepoints have already been recorded but should be invoked
 * early.
 */
void lf_tracing_set_start_time(int64_t start_time);
/**
 * @brief Submit a tracepoint from the given worker to the tracing module.
 */
void lf_tracing_tracepoint(
    int worker,
    trace_record_nodeps_t* tr
);
/**
 * @brief Shut down the tracing module. Calling other API functions after
 * calling this procedure is undefined behavior.
 */
void lf_tracing_global_shutdown();

#endif // TRACE_H
