/**
 * @file
 * @author Edward A. Lee
 *
 * @section LICENSE
Copyright (c) 2020, The University of California at Berkeley and TU Dresden

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 * Include this file instead of trace.h to get tracing.
 * See trace.h file for instructions.
 */

#include "trace.h"

#ifdef LF_TRACE

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"

#ifdef RTI_TRACE
#include "net_common.h"  // Defines message types
#endif // RTI_TRACE

#include "reactor_common.h"
#include "util.h"
#include "plugin-apis/trace.h"

/** Macro to use when access to trace file fails. */
#define _LF_TRACE_FAILURE(trace) \
    do { \
        fprintf(stderr, "WARNING: Access to trace file failed.\n"); \
        fclose(trace->_lf_trace_file); \
        trace->_lf_trace_file = NULL; \
        return -1; \
    } while(0)

void lf_tracing_init(int process_id) {
    lf_tracing_global_init(process_id);
}

trace_t* trace_new(environment_t* env, const char * filename) {
    trace_t * trace = (trace_t *) calloc(1, sizeof(trace_t));
    LF_ASSERT(trace, "Out of memory");

    trace->_lf_trace_stop=1;
    trace->env = env;

    // Determine length of the filename
    size_t len = strlen(filename)  + 1;

    // Allocate memory for the filename on the trace struct
    trace->filename = (char*) malloc(len * sizeof(char));
    LF_ASSERT(trace->filename, "Out of memory");

    // Copy it to the struct
    strncpy(trace->filename, filename, len);

    return trace;
}

void trace_free(trace_t *trace) {
    free(trace->filename);
    free(trace);
}

int _lf_register_trace_event(trace_t* trace, void* pointer1, void* pointer2, _lf_trace_object_t type, char* description) {
    lf_critical_section_enter(trace->env);
    if (trace->_lf_trace_object_descriptions_size >= TRACE_OBJECT_TABLE_SIZE) {
        lf_critical_section_exit(trace->env);
        fprintf(stderr, "WARNING: Exceeded trace object table size. Trace file will be incomplete.\n");
        return 0;
    }
    trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].pointer = pointer1;
    trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].trigger = pointer2;
    trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].type = type;
    trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].description = description;
    trace->_lf_trace_object_descriptions_size++;
    lf_critical_section_exit(trace->env);
    return 1;
}

int register_user_trace_event(void *self, char* description) {
    LF_ASSERT(self, "Need a pointer to a self struct to register a user trace event");
    trace_t * trace = ((self_base_t *) self)->environment->trace;
    return _lf_register_trace_event(trace, description, NULL, trace_user, description);
}

/**
 * Write the trace header information.
 * See trace.h.
 * @return The number of items written to the object table or -1 for failure.
 */
static int write_trace_header(trace_t* trace) {
    if (trace->_lf_trace_file != NULL) {
        // The first item in the header is the start time.
        // This is both the starting physical time and the starting logical time.
        instant_t start_time = lf_time_start();
        // printf("DEBUG: Start time written to trace file is %lld.\n", start_time);
        size_t items_written = fwrite(
                &start_time,
                sizeof(instant_t),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) _LF_TRACE_FAILURE(trace);

        // The next item in the header is the size of the
        // _lf_trace_object_descriptions table.
        // printf("DEBUG: Table size written to trace file is %d.\n", _lf_trace_object_descriptions_size);
        items_written = fwrite(
                &trace->_lf_trace_object_descriptions_size,
                sizeof(int),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) _LF_TRACE_FAILURE(trace);

        // Next we write the table.
        for (int i = 0; i < trace->_lf_trace_object_descriptions_size; i++) {
            // printf("DEBUG: Object pointer: %p.\n", _lf_trace_object_descriptions[i].pointer);
            // Write the pointer to the self struct.
            items_written = fwrite(
                        &trace->_lf_trace_object_descriptions[i].pointer,
                        sizeof(void*),
                        1,
                        trace->_lf_trace_file
            );
            if (items_written != 1) _LF_TRACE_FAILURE(trace);

            // Write the pointer to the trigger_t struct.
            items_written = fwrite(
                        &trace->_lf_trace_object_descriptions[i].trigger,
                        sizeof(trigger_t*),
                        1,
                        trace->_lf_trace_file
            );
            if (items_written != 1) _LF_TRACE_FAILURE(trace);

            // Write the object type.
            items_written = fwrite(
                        &trace->_lf_trace_object_descriptions[i].type, // Write the pointer value.
                        sizeof(_lf_trace_object_t),
                        1,
                        trace->_lf_trace_file
            );
            if (items_written != 1) _LF_TRACE_FAILURE(trace);

            // Write the description.
            int description_size = strlen(trace->_lf_trace_object_descriptions[i].description);
            // printf("DEBUG: Object description: %s.\n", trace->_lf_trace_object_descriptions[i].description);
            items_written = fwrite(
                        trace->_lf_trace_object_descriptions[i].description,
                        sizeof(char),
                        description_size + 1, // Include null terminator.
                        trace->_lf_trace_file
            );
            if (items_written != description_size + 1) _LF_TRACE_FAILURE(trace);
        }
    }
    return trace->_lf_trace_object_descriptions_size;
}

/**
 * @brief Flush the specified buffer to a file.
 * This assumes the caller has entered a critical section.
 * @param worker Index specifying the trace to flush.
 */
void flush_trace_locked(trace_t* trace, int worker) {
    if (trace->_lf_trace_stop == 0
        && trace->_lf_trace_file != NULL
        && trace->_lf_trace_buffer_size[worker] > 0
    ) {
        // If the trace header has not been written, write it now.
        // This is deferred to here so that user trace objects can be
        // registered in startup reactions.
        if (!trace->_lf_trace_header_written) {
            if (write_trace_header(trace) < 0) {
                lf_print_error("Failed to write trace header. Trace file will be incomplete.");
                return;
            }
            trace->_lf_trace_header_written = true;
        }

        // Write first the length of the array.
        size_t items_written = fwrite(
                &trace->_lf_trace_buffer_size[worker],
                sizeof(int),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) {
            fprintf(stderr, "WARNING: Access to trace file failed.\n");
            fclose(trace->_lf_trace_file);
            trace->_lf_trace_file = NULL;
        } else {
            // Write the contents.
            items_written = fwrite(
                    trace->_lf_trace_buffer[worker],
                    sizeof(trace_record_t),
                    trace->_lf_trace_buffer_size[worker],
                    trace->_lf_trace_file
            );
            if (items_written != trace->_lf_trace_buffer_size[worker]) {
                fprintf(stderr, "WARNING: Access to trace file failed.\n");
                fclose(trace->_lf_trace_file);
                trace->_lf_trace_file = NULL;
            }
        }
        trace->_lf_trace_buffer_size[worker] = 0;
    }
}

/**
 * @brief Flush the specified buffer to a file.
 * @param worker Index specifying the trace to flush.
 */
void flush_trace(trace_t* trace, int worker) {
    // To avoid having more than one worker writing to the file at the same time,
    // enter a critical section.
    lf_critical_section_enter(GLOBAL_ENVIRONMENT);
    flush_trace_locked(trace, worker);
    lf_critical_section_exit(GLOBAL_ENVIRONMENT);
}

void start_trace(trace_t* trace) {
    // FIXME: location of trace file should be customizable.
    trace->_lf_trace_file = fopen(trace->filename, "w");
    if (trace->_lf_trace_file == NULL) {
        fprintf(stderr, "WARNING: Failed to open log file with error code %d."
                "No log will be written.\n", errno);
    }
    // Do not write the trace header information to the file yet
    // so that startup reactions can register user-defined trace objects.
    // write_trace_header();
    trace->_lf_trace_header_written = false;

    // Allocate an array of arrays of trace records, one per worker thread plus one
    // for the 0 thread (the main thread, or in an single-threaded program, the only
    // thread).
    trace->_lf_number_of_trace_buffers = _lf_number_of_workers + 1;
    trace->_lf_trace_buffer = (trace_record_t**)malloc(sizeof(trace_record_t*) * trace->_lf_number_of_trace_buffers);
    for (int i = 0; i < trace->_lf_number_of_trace_buffers; i++) {
        trace->_lf_trace_buffer[i] = (trace_record_t*)malloc(sizeof(trace_record_t) * TRACE_BUFFER_CAPACITY);
    }
    // Array of counters that track the size of each trace record (per thread).
    trace->_lf_trace_buffer_size = (int*)calloc(sizeof(int), trace->_lf_number_of_trace_buffers);

    trace->_lf_trace_stop = 0;
    LF_PRINT_DEBUG("Started tracing.");
}

void call_tracepoint(
        trace_t* trace,
        int event_type,
        void* reactor,
        tag_t* tag,
        int worker,
        int src_id,
        int dst_id,
        instant_t* physical_time,
        trigger_t* trigger,
        interval_t extra_delay,
        bool is_interval_start
) {
    environment_t *env = trace->env;
    instant_t local_time;
    if (!is_interval_start && physical_time == NULL) {
        local_time = lf_time_physical();
        physical_time = &local_time;
    }
    tag_t local_tag;
    if (tag != NULL) {
        local_tag.time = tag->time;
        local_tag.microstep = tag->microstep;
    } else if (env != NULL) {
        local_tag.time = ((environment_t *)env)->current_tag.time;
        local_tag.microstep = ((environment_t*)env)->current_tag.microstep;
    }
    tag = &local_tag;
    trace_record_t tr = {
        .event_type = event_type,
        .pointer = reactor,
        .src_id = src_id,
        .dst_id = dst_id,
        .logical_time = tag->time,
        .microstep = tag->microstep,
        .trigger = trigger,
        .extra_delay = extra_delay,
        .physical_time = *physical_time
    };
    tracepoint(trace, worker, (trace_record_nodeps_t*) &tr);
}

/**
 * Trace a call to schedule.
 * @param trigger Pointer to the trigger_t struct for the trigger.
 * @param extra_delay The extra delay passed to schedule().
 */
void tracepoint_schedule(trace_t* trace, trigger_t* trigger, interval_t extra_delay) {
    // schedule() can only trigger reactions within the same reactor as the action
    // or timer. If there is such a reaction, find its reactor's self struct and
    // put that into the tracepoint. We only have to look at the first reaction.
    // If there is no reaction, insert NULL for the reactor.
    void* reactor = NULL;
    if (trigger->number_of_reactions > 0
            && trigger->reactions[0] != NULL) {
        reactor = trigger->reactions[0]->self;
    }
    // NOTE: The -1 argument indicates no worker.
    // This is OK because it is called only while holding the mutex lock.
    // True argument specifies to record physical time as late as possible, when
    // the event is already on the event queue.
    call_tracepoint(trace, schedule_called, reactor, NULL, -1, 0, 0, NULL, trigger, extra_delay, true);
}

/**
 * Trace a user-defined event. Before calling this, you must call
 * register_user_trace_event() with a pointer to the same string
 * or else the event will not be recognized.
 * @param self Pointer to the self struct of the reactor from which we want
 * to trace this event. This pointer is used to get the correct environment and
 * thus the correct logical tag of the event.
 * @param description Pointer to the description string.
 */
void tracepoint_user_event(void* self, char* description) {
    // -1s indicate unknown reaction number and worker thread.
    // NOTE: We currently have no way to get the number of the worker that
    // is executing the reaction that calls this, so we can't pass a worker
    // number to the tracepoint function. We pass -1, indicating no worker.
    // But to be safe, then, we have acquire a mutex before calling this
    // because multiple reactions might be calling the same tracepoint function.
    // There will be a performance hit for this.
    LF_ASSERT(self, "A pointer to the self struct is needed to trace an event");
    environment_t *env = ((self_base_t *)self)->environment;
    trace_t *trace = env->trace;
    lf_critical_section_enter(env);
    call_tracepoint(trace, user_event, description, NULL, -1, -1, -1, NULL, NULL, 0, false);
    lf_critical_section_exit(env);
}

/**
 * Trace a user-defined event with a value.
 * Before calling this, you must call
 * register_user_trace_event() with a pointer to the same string
 * or else the event will not be recognized.
 * @param self Pointer to the self struct of the reactor from which we want
 * to trace this event. This pointer is used to get the correct environment and
 * thus the correct logical tag of the event.
 * @param description Pointer to the description string.
 * @param value The value of the event. This is a long long for
 *  convenience so that time values can be passed unchanged.
 *  But int values work as well.
 */
void tracepoint_user_value(void* self, char* description, long long value) {
    // -1s indicate unknown reaction number and worker thread.
    // NOTE: We currently have no way to get the number of the worker that
    // is executing the reaction that calls this, so we can't pass a worker
    // number to the tracepoint function. We pass -1, indicating no worker.
    // But to be safe, then, we have acquire a mutex before calling this
    // because multiple reactions might be calling the same tracepoint function.
    // There will be a performance hit for this.
    environment_t *env = ((self_base_t *)self)->environment;
    trace_t *trace = env->trace;
    lf_critical_section_enter(env);
    call_tracepoint(trace, user_value, description,  NULL, -1, -1, -1, NULL, NULL, value, false);
    lf_critical_section_exit(env);
}

void stop_trace(trace_t* trace) {
    lf_critical_section_enter(trace->env);
    stop_trace_locked(trace);
    lf_critical_section_exit(trace->env);
}

void stop_trace_locked(trace_t* trace) {
    if (trace->_lf_trace_stop) {
        // Trace was already stopped. Nothing to do.
        return;
    }
    // In multithreaded execution, thread 0 invokes wrapup reactions, so we
    // put that trace last. However, it could also include some startup events.
    // In any case, the trace file does not guarantee any ordering.
    for (int i = 1; i < trace->_lf_number_of_trace_buffers; i++) {
        // Flush the buffer if it has data.
        // printf("DEBUG: Trace buffer %d has %d records.\n", i, trace->_lf_trace_buffer_size[i]);
        if (trace->_lf_trace_buffer_size && trace->_lf_trace_buffer_size[i] > 0) {
            flush_trace_locked(trace, i);
        }
    }
    if (trace->_lf_trace_buffer_size && trace->_lf_trace_buffer_size[0] > 0) {
        flush_trace_locked(trace, 0);
    }
    trace->_lf_trace_stop = 1;
    if (trace->_lf_trace_file != NULL) {
        fclose(trace->_lf_trace_file);
        trace->_lf_trace_file = NULL;
    }
    LF_PRINT_DEBUG("Stopped tracing.");
}

////////////////////////////////////////////////////////////
//// For federated execution

#if defined FEDERATED || defined LF_ENCLAVES

/**
 * Trace federate sending a message to the RTI.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_federate_to_rti(trace_t *trace, trace_event_t event_type, int fed_id, tag_t* tag) {
    call_tracepoint(
        trace,
        event_type,
        NULL,   // void* pointer,
        tag,    // tag* tag,
        -1,     // int worker, // no worker ID needed because this is called within a mutex
        fed_id, // int src_id,
        -1,     // int dst_id,
        NULL,   // instant_t* physical_time (will be generated)
        NULL,   // trigger_t* trigger,
        0,      // interval_t extra_delay
        true    // is_interval_start
    );
}

/**
 * Trace federate receiving a message from the RTI.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param tag Pointer to the tag that has been received, or NULL.
 */
void tracepoint_federate_from_rti(trace_t* trace, trace_event_t event_type, int fed_id, tag_t* tag) {
    // trace_event_t event_type = (type == MSG_TYPE_TAG_ADVANCE_GRANT)? federate_TAG : federate_PTAG;
    call_tracepoint(
        trace,
        event_type,
        NULL,   // void* pointer,
        tag,    // tag* tag,
        -1,     // int worker, // no worker ID needed because this is called within a mutex
        fed_id, // int src_id,
        -1,     // int dst_id,
        NULL,   // instant_t* physical_time (will be generated)
        NULL,   // trigger_t* trigger,
        0,      // interval_t extra_delay
        false   // is_interval_start
    );
}

/**
 * Trace federate sending a message to another federate.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param partner_id The partner federate identifier.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_federate_to_federate(trace_t* trace, trace_event_t event_type, int fed_id, int partner_id, tag_t *tag) {
    call_tracepoint(
        trace,
        event_type,
        NULL,   // void* pointer,
        tag,    // tag* tag,
        -1,     // int worker, // no worker ID needed because this is called within a mutex
        fed_id, // int src_id,
        partner_id,     // int dst_id,
        NULL,   // instant_t* physical_time (will be generated)
        NULL,   // trigger_t* trigger,
        0,      // interval_t extra_delay
        true    // is_interval_start
    );
}

/**
 * Trace federate receiving a message from another federate.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param partner_id The partner federate identifier.
 * @param tag Pointer to the tag that has been received, or NULL.
 */
void tracepoint_federate_from_federate(trace_t* trace, trace_event_t event_type, int fed_id, int partner_id, tag_t *tag) {
    call_tracepoint(
        trace,
        event_type,
        NULL,   // void* pointer,
        tag,    // tag* tag,
        -1,     // int worker, // no worker ID needed because this is called within a mutex
        fed_id, // int src_id,
        partner_id,     // int dst_id,
        NULL,   // instant_t* physical_time (will be generated)
        NULL,   // trigger_t* trigger,
        0,      // interval_t extra_delay
        false   // is_interval_start
    );
}
#endif // FEDERATED

////////////////////////////////////////////////////////////
//// For RTI execution

#ifdef RTI_TRACE

/**
 * Trace RTI sending a message to a federate.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The fedaerate ID.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_rti_to_federate(trace_t* trace, trace_event_t event_type, int fed_id, tag_t* tag) {
    call_tracepoint(
        trace,
        event_type,
        NULL,   // void* pointer,
        tag,    // tag_t* tag,
        fed_id, // int worker (one thread per federate)
        -1,     // int src_id
        fed_id, // int dst_id
        NULL,   // instant_t* physical_time (will be generated)
        NULL,   // trigger_t* trigger,
        0,      // interval_t extra_delay
        true    // is_interval_start
    );
}

/**
 * Trace RTI receiving a message from a federate.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The fedaerate ID.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_rti_from_federate(trace_t* trace, trace_event_t event_type, int fed_id, tag_t* tag) {
    call_tracepoint(
        trace,
        event_type,
        NULL,   // void* pointer,
        tag,    // tag_t* tag,
        fed_id, // int worker (one thread per federate)
        -1,     // int src_id  (RTI is the source of the tracepoint)
        fed_id, // int dst_id
        NULL,   // instant_t* physical_time (will be generated)
        NULL,   // trigger_t* trigger,
        0,      // interval_t extra_delay
        false   // is_interval_start
    );
}

#endif // RTI_TRACE

#endif // LF_TRACE
