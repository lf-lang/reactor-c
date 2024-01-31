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

// /**
//  * @brief This struct holds all the state associated with tracing in a single environment.
//  * Each environment which has tracing enabled will have such a struct on its environment struct.
//  *
//  */
// typedef struct trace_t {
//     /**
//      * Array of buffers into which traces are written.
//      * When a buffer becomes full, the contents is flushed to the file,
//      * which will create a significant pause in the calling thread.
//      */
//     trace_record_t** _lf_trace_buffer;
//     int* _lf_trace_buffer_size;

//     /** The number of trace buffers allocated when tracing starts. */
//     int _lf_number_of_trace_buffers;

//     /** Marker that tracing is stopping or has stopped. */
//     int _lf_trace_stop;

//     /** The file into which traces are written. */
//     FILE* _lf_trace_file;

//     /** The file name where the traces are written*/
//     char *filename;

//     /** Table of pointers to a description of the object. */
//     object_description_t _lf_trace_object_descriptions[TRACE_OBJECT_TABLE_SIZE];
//     int _lf_trace_object_descriptions_size;

//     /** Indicator that the trace header information has been written to the file. */
//     bool _lf_trace_header_written;

//     /** Pointer back to the environment which we are tracing within*/
//     environment_t* env;
// } trace_t;

int _lf_register_trace_event(void* pointer1, void* pointer2, _lf_trace_object_t type, char* description) {
    object_description_t desc = {
        .pointer = pointer1,
        .trigger = pointer2,
        .type = type,
        .description = description
    };
    lf_tracing_register_trace_event(desc);
    // trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].pointer = pointer1;
    // trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].trigger = pointer2;
    // trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].type = type;
    // trace->_lf_trace_object_descriptions[trace->_lf_trace_object_descriptions_size].description = description;
    // trace->_lf_trace_object_descriptions_size++;
    return 1;
}

int register_user_trace_event(void *self, char* description) {
    LF_ASSERT(self, "Need a pointer to a self struct to register a user trace event");  // FIXME: Not needed. self not needed either
    // trace_t * trace = ((self_base_t *) self)->environment->trace;
    return _lf_register_trace_event(description, NULL, trace_user, description);
}

void call_tracepoint(
        int event_type,
        void* reactor,
        tag_t tag,
        int worker,
        int src_id,
        int dst_id,
        instant_t* physical_time,
        trigger_t* trigger,
        interval_t extra_delay,
        bool is_interval_start  // FIXME: Unused argument. It originally was a micro-optimization which I suspect is not needed, but if there is (statistically significant) empirical evidence that it is needed, then we should use it. Note that without a memory barrier, even its original use may have had no effect (i.e., it could have generated the same assembly code anyway).
) {
    // environment_t *env = trace->env;
    instant_t local_time;
    if (physical_time == NULL) {
        local_time = lf_time_physical();
        physical_time = &local_time;
    }
    // tag_t local_tag;
    // if (tag != NULL) {
    //     local_tag.time = tag->time;
    //     local_tag.microstep = tag->microstep;
    // }
    // // else if (env != NULL) {
    // //     local_tag.time = ((environment_t *)env)->current_tag.time;
    // //     local_tag.microstep = ((environment_t*)env)->current_tag.microstep;
    // // }
    // tag = &local_tag;
    trace_record_t tr = {
        .event_type = event_type,
        .pointer = reactor,
        .src_id = src_id,
        .dst_id = dst_id,
        .logical_time = tag.time,
        .microstep = tag.microstep,
        .trigger = trigger,
        .extra_delay = extra_delay,
        .physical_time = *physical_time
    };
    tracepoint(worker, (trace_record_nodeps_t*) &tr);
}

/**
 * Trace a call to schedule.
 * @param trigger Pointer to the trigger_t struct for the trigger.
 * @param extra_delay The extra delay passed to schedule().
 */
void tracepoint_schedule(environment_t* env, trigger_t* trigger, interval_t extra_delay) {
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
    call_tracepoint(schedule_called, reactor, env->current_tag, -1, 0, 0, NULL, trigger, extra_delay, true);
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
    // lf_critical_section_enter(env); // FIXME: NOT NEEDED. SELF IS NOT NEEDED
    call_tracepoint(user_event, description, env->current_tag, -1, -1, -1, NULL, NULL, 0, false);
    // lf_critical_section_exit(env);
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
    // trace_t *trace = env->trace;  // FIXME
    // lf_critical_section_enter(env);
    call_tracepoint(user_value, description, env->current_tag, -1, -1, -1, NULL, NULL, value, false);
    // lf_critical_section_exit(env);
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
void tracepoint_federate_to_rti(trace_event_t event_type, int fed_id, tag_t* tag) {
    call_tracepoint(
        event_type,
        NULL,   // void* pointer,
        tag ? *tag : NEVER,    // tag* tag,
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
void tracepoint_federate_from_rti(trace_event_t event_type, int fed_id, tag_t* tag) {
    // trace_event_t event_type = (type == MSG_TYPE_TAG_ADVANCE_GRANT)? federate_TAG : federate_PTAG;
    call_tracepoint(
        event_type,
        NULL,   // void* pointer,
        tag ? *tag : NEVER,    // tag* tag,
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
void tracepoint_federate_to_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t *tag) {
    call_tracepoint(
        event_type,
        NULL,   // void* pointer,
        tag ? *tag : NEVER,    // tag* tag,
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
void tracepoint_federate_from_federate(trace_event_t event_type, int fed_id, int partner_id, tag_t *tag) {
    call_tracepoint(
        event_type,
        NULL,   // void* pointer,
        tag ? *tag : NEVER,    // tag* tag,
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
void tracepoint_rti_to_federate(trace_event_t event_type, int fed_id, tag_t* tag) {
    call_tracepoint(
        event_type,
        NULL,   // void* pointer,
        tag ? *tag : NEVER,    // tag_t* tag,
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
void tracepoint_rti_from_federate(trace_event_t event_type, int fed_id, tag_t* tag) {
    call_tracepoint(
        event_type,
        NULL,   // void* pointer,
        tag ? *tag : NEVER,    // tag_t* tag,
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
