#ifndef TRACE_ABI_H
#define TRACE_ABI_H

#include <stdint.h>
#include <stdbool.h>

#include "plugin-apis/version-abi.h"

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
    void* pointer;      // Pointer to the reactor self struct or other identifying pointer.
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

void lf_tracing_global_init(char* file_name_prefix, int process_id, int max_num_local_threads);
void lf_tracing_register_trace_event(object_description_t description);
void lf_tracing_set_start_time(int64_t start_time);
void tracepoint(
    int worker,
    trace_record_nodeps_t* tr
);
void lf_tracing_global_shutdown();

#endif // TRACE_ABI_H
