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

#include "tracepoint.h"

#ifdef LF_TRACE

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "low_level_platform.h"

#ifdef RTI_TRACE
#include "net_common.h"  // Defines message types
#endif // RTI_TRACE

#include "reactor_common.h"
#include "util.h"

int _lf_register_trace_event(void* pointer1, void* pointer2, _lf_trace_object_t type, char* description) {
    object_description_t desc = {
        .pointer = pointer1,
        .trigger = pointer2,
        .type = type,
        .description = description
    };
    lf_tracing_register_trace_event(desc);
    return 1;
}

int register_user_trace_event(void *self, char* description) {
    LF_ASSERT(self, "Need a pointer to a self struct to register a user trace event");  // FIXME: Not needed. self not needed either
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
        bool is_interval_start
) {
    instant_t local_time;
    if (physical_time == NULL) {
        local_time = lf_time_physical();
        physical_time = &local_time;
    }
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
    lf_tracing_tracepoint(worker, (trace_record_nodeps_t*) &tr);
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
    call_tracepoint(user_event, description, env->current_tag, -1, -1, -1, NULL, NULL, 0, false);
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
    call_tracepoint(user_value, description, env->current_tag, -1, -1, -1, NULL, NULL, value, false);
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
        tag ? *tag : NEVER_TAG,    // tag* tag,
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
        tag ? *tag : NEVER_TAG,    // tag* tag,
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
        tag ? *tag : NEVER_TAG,    // tag* tag,
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
        tag ? *tag : NEVER_TAG,    // tag* tag,
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
        tag ? *tag : NEVER_TAG,    // tag_t* tag,
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
        tag ? *tag : NEVER_TAG,    // tag_t* tag,
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
