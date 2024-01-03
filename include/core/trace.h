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
 * Definitions of tracepoint events for use with the C code generator and any other
 * code generator that uses the C infrastructure (such as the Python code generator).
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

#ifdef RTI_TRACE
#define LF_TRACE
#endif

#ifndef TRACE_H
#define TRACE_H

#include "lf_types.h"
#include <stdio.h>

#ifdef FEDERATED
#include "net_common.h"
#endif // FEDERATED

/**
 * Trace event types. If you update this, be sure to update the
 * string representation below. Also, create a tracepoint function
 * for each event type.
 */
typedef enum
{
    reaction_starts,
    reaction_ends,
    reaction_deadline_missed,
    schedule_called,
    user_event,
    user_value,
    worker_wait_starts,
    worker_wait_ends,
    scheduler_advancing_time_starts,
    scheduler_advancing_time_ends,
    federated, // Everything below this is for tracing federated interactions.
    // Sending messages
    send_ACK,
    send_FAILED,
    send_TIMESTAMP,
    send_NET,
    send_LTC,
    send_STOP_REQ,
    send_STOP_REQ_REP,
    send_STOP_GRN,
    send_FED_ID,
    send_PTAG,
    send_TAG,
    send_REJECT,
    send_RESIGN,
    send_PORT_ABS,
    send_CLOSE_RQ,
    send_TAGGED_MSG,
    send_P2P_TAGGED_MSG,
    send_MSG,
    send_P2P_MSG,
    send_ADR_AD,
    send_ADR_QR,
    // Receiving messages
    receive_ACK,
    receive_FAILED,
    receive_TIMESTAMP,
    receive_NET,
    receive_LTC,
    receive_STOP_REQ,
    receive_STOP_REQ_REP,
    receive_STOP_GRN,
    receive_FED_ID,
    receive_PTAG,
    receive_TAG,
    receive_REJECT,
    receive_RESIGN,
    receive_PORT_ABS,
    receive_CLOSE_RQ,
    receive_TAGGED_MSG,
    receive_P2P_TAGGED_MSG,
    receive_MSG,
    receive_P2P_MSG,
    receive_ADR_AD,
    receive_ADR_QR,
    receive_UNIDENTIFIED,
    NUM_EVENT_TYPES
} trace_event_t;

#ifdef LF_TRACE

/**
 * String description of event types.
 */
static const char *trace_event_names[] = {
    "Reaction starts",
    "Reaction ends",
    "Reaction deadline missed",
    "Schedule called",
    "User-defined event",
    "User-defined valued event",
    "Worker wait starts",
    "Worker wait ends",
    "Scheduler advancing time starts",
    "Scheduler advancing time ends",
    "Federated marker",
    // Sending messages
    "Sending ACK",
    "Sending FAILED",
    "Sending TIMESTAMP",
    "Sending NET",
    "Sending LTC",
    "Sending STOP_REQ",
    "Sending STOP_REQ_REP",
    "Sending STOP_GRN",
    "Sending FED_ID",
    "Sending PTAG",
    "Sending TAG",
    "Sending REJECT",
    "Sending RESIGN",
    "Sending PORT_ABS",
    "Sending CLOSE_RQ",
    "Sending TAGGED_MSG",
    "Sending P2P_TAGGED_MSG",
    "Sending MSG",
    "Sending P2P_MSG",
    "Sending ADR_AD",
    "Sending ADR_QR",
    // Receiving messages
    "Receiving ACK",
    "Receiving FAILED",
    "Receiving TIMESTAMP",
    "Receiving NET",
    "Receiving LTC",
    "Receiving STOP_REQ",
    "Receiving STOP_REQ_REP",
    "Receiving STOP_GRN",
    "Receiving FED_ID",
    "Receiving PTAG",
    "Receiving TAG",
    "Receiving REJECT",
    "Receiving RESIGN",
    "Receiving PORT_ABS",
    "Receiving CLOSE_RQ",
    "Receiving TAGGED_MSG",
    "Receiving P2P_TAGGED_MSG",
    "Receiving MSG",
    "Receiving P2P_MSG",
    "Receiving ADR_AD",
    "Receiving ADR_QR",
    "Receiving UNIDENTIFIED",
};

// FIXME: Target property should specify the capacity of the trace buffer.
#define TRACE_BUFFER_CAPACITY 2048

/** Size of the table of trace objects. */
#define TRACE_OBJECT_TABLE_SIZE 1024

/**
 * @brief A trace record that is written in binary to the trace file.
 */
typedef struct trace_record_t {
    trace_event_t event_type;
    void* pointer;  // pointer identifying the record, e.g. to self struct for a reactor.
    int src_id;     // The ID number of the source (e.g. worker or federate) or -1 for no ID number.
    int dst_id;     // The ID number of the destination (e.g. reaction or federate) or -1 for no ID number.
    instant_t logical_time;
    microstep_t microstep;
    instant_t physical_time;
    trigger_t* trigger;
    interval_t extra_delay;
} trace_record_t;

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
/**
 * 
 * @brief This struct holds all the state associated with tracing in a single environment.
 * Each environment which has tracing enabled will have such a struct on its environment struct.
 * 
 */
typedef struct trace_t {
    /**
     * Array of buffers into which traces are written.
     * When a buffer becomes full, the contents is flushed to the file,
     * which will create a significant pause in the calling thread.
     */
    trace_record_t** _lf_trace_buffer;
    int* _lf_trace_buffer_size;

    /** The number of trace buffers allocated when tracing starts. */
    int _lf_number_of_trace_buffers;

    /** Marker that tracing is stopping or has stopped. */
    int _lf_trace_stop;

    /** The file into which traces are written. */
    FILE* _lf_trace_file;

    /** The file name where the traces are written*/
    char *filename;

    /** Table of pointers to a description of the object. */
    object_description_t _lf_trace_object_descriptions[TRACE_OBJECT_TABLE_SIZE];
    int _lf_trace_object_descriptions_size;

    /** Indicator that the trace header information has been written to the file. */
    bool _lf_trace_header_written;

    /** Pointer back to the environment which we are tracing within*/
    environment_t* env;
} trace_t;


/**
 * @brief Dynamically allocate a new tracing object. 
 * 
 * @param env The environment in which we are tracing. If passed NULL we use the GLOBAL_ENVIRONMENT
 * @param filename Name of the file in which to store the trace
 * @return trace_t* A newly allocated trace object with environment pointer and filename initialized
 */
trace_t* trace_new(environment_t *env, const char *filename);

/**
 * @brief Free the memory allocated for the trace object
 * 
 * @param trace 
 */
void trace_free(trace_t *trace);


/**
 * Register a trace object.
 * @param env Pointer to the environment in which the event is traced
 * @param pointer1 Pointer that identifies the object, typically to a reactor self struct.
 * @param pointer2 Further identifying pointer, typically to a trigger (action or timer) or NULL if irrelevant.
 * @param type The type of trace object.
 * @param description The human-readable description of the object.
 * @return 1 if successful, 0 if the trace object table is full.
 */
int _lf_register_trace_event(trace_t* trace, void* pointer1, void* pointer2, _lf_trace_object_t type, char* description);

/**
 * Register a user trace event. This should be called once, providing a pointer to a string
 * that describes a phenomenon being traced. Use the same pointer as the first argument to
 * tracepoint_user_event() and tracepoint_user_value().
 * @param description Pointer to a human-readable description of the event.
 * @return 1 if successful, 0 if the trace object table is full.
 */
int register_user_trace_event(void* self, char* description);

/**
 * Open a trace file and start tracing.
 * @param filename The filename for the trace file.
 */
void start_trace(trace_t* trace);

/**
 * Trace an event identified by a type and a pointer to the self struct of the reactor instance.
 * This is a generic tracepoint function. It is better to use one of the specific functions.
 * The worker argument determines which buffer to write to.
 * Hence, as long as this argument is distinct for each caller, the callers can be in
 * different threads without the need for a mutex lock.
 * @param event_type The type of event (see trace_event_t in trace.h)
 * @param reactor The pointer to the self struct of the reactor instance in the trace table.
 * @param tag Pointer to a tag or NULL to use current tag.
 * @param worker The ID of the worker thread (which determines which buffer to write to).
 * @param src_id The ID number of the source (e.g. worker or federate) or -1 for no ID number.
 * @param dst_id The ID number of the destination (e.g. reaction or federate) or -1 for no ID number.
 * @param physical_time If the caller has already accessed physical time, provide it here.
 *  Otherwise, provide NULL. This argument avoids a second call to lf_time_physical()
 *  and ensures that the physical time in the trace is the same as that used by the caller.
 * @param trigger Pointer to the trigger_t struct for calls to schedule or NULL otherwise.
 * @param extra_delay The extra delay passed to schedule(). If not relevant for this event
 *  type, pass 0.
 * @param is_interval_start True to indicate that this tracepoint is at the beginning of
 *  time interval, such as reaction invocation, so that physical time is captured as late
 *  as possible.  False to indicate that it is at the end of an interval, such as the end
 *  of a reaction invocation, so that physical time is captured as early as possible.
 */
void tracepoint(
        trace_t* trace,
        trace_event_t event_type,
        void* reactor,
        tag_t* tag,
        int worker,
        int src_id,
        int dst_id,
        instant_t* physical_time,
        trigger_t* trigger,
        interval_t extra_delay,
        bool is_interval_start
);

/**
 * Trace the start of a reaction execution.
 * @param env The environment in which we are executing
 * @param reaction Pointer to the reaction_t struct for the reaction.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
void tracepoint_reaction_starts(trace_t* trace, reaction_t* reaction, int worker);

/**
 * Trace the end of a reaction execution.
 * @param env The environment in which we are executing
 * @param reaction Pointer to the reaction_t struct for the reaction.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
void tracepoint_reaction_ends(trace_t* trace, reaction_t* reaction, int worker);

/**
 * Trace a call to schedule.
 * @param env The environment in which we are executing
 * @param trigger Pointer to the trigger_t struct for the trigger.
 * @param extra_delay The extra delay passed to schedule().
 */
void tracepoint_schedule(trace_t* trace, trigger_t* trigger, interval_t extra_delay);

/**
 * Trace a user-defined event. Before calling this, you must call
 * register_user_trace_event() with a pointer to the same string
 * or else the event will not be recognized.
 * @param self Pointer to the self struct of the reactor from which we want
 * to trace this event. This pointer is used to get the correct environment and 
 * thus the correct logical tag of the event.
 * @param description Pointer to the description string.
 */
void tracepoint_user_event(void* self, char* description);

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
void tracepoint_user_value(void* self, char* description, long long value);

/**
 * Trace the start of a worker waiting for something to change on the reaction queue.
 * @param env The environment in which we are executing
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
void tracepoint_worker_wait_starts(trace_t* trace, int worker);

/**
 * Trace the end of a worker waiting for something to change on reaction queue.
 * @param env The environment in which we are executing
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
void tracepoint_worker_wait_ends(trace_t* trace, int worker);

/**
 * Trace the start of the scheduler waiting for logical time to advance or an event to
 * appear on the event queue.
 * @param env The environment in which we are executing
 */
void tracepoint_scheduler_advancing_time_starts(trace_t* trace);

/**
 * Trace the end of the scheduler waiting for logical time to advance or an event to
 * appear on the event queue.
 * @param env The environment in which we are executing
 */
void tracepoint_scheduler_advancing_time_ends(trace_t* trace);

/**
 * Trace the occurence of a deadline miss.
 * @param env The environment in which we are executing
 * @param reaction Pointer to the reaction_t struct for the reaction.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution.
 */
void tracepoint_reaction_deadline_missed(trace_t* trace, reaction_t *reaction, int worker);

/**
 * Flush any buffered trace records to the trace file and
 * close the files.
 */
void stop_trace(trace_t* trace);

/**
 * Version of stop_trace() that does not lock the trace mutex.
 */
void stop_trace_locked(trace_t* trace);

////////////////////////////////////////////////////////////
//// For federated execution

#if defined(FEDERATED) || defined(LF_ENCLAVES)

/**
 * Trace federate sending a message to the RTI.
 * @param event_type The type of event. Possible values are:
 * 
 * @param fed_id The federate identifier.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_federate_to_rti(trace_t* trace, trace_event_t event_type, int fed_id, tag_t* tag);

/**
 * Trace federate receiving a message from the RTI.
 * @param event_type The type of event. Possible values are:
 * 
 * @param fed_id The federate identifier.
 * @param tag Pointer to the tag that has been received, or NULL.
 */
void tracepoint_federate_from_rti(trace_t* trace, trace_event_t event_type, int fed_id, tag_t* tag);

/**
 * Trace federate sending a message to another federate.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param partner_id The partner federate identifier.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_federate_to_federate(trace_t* trace, trace_event_t event_type, int fed_id, int partner_id, tag_t *tag);

/**
 * Trace federate receiving a message from another federate.
 * @param event_type The type of event. Possible values are:
 *
 * @param fed_id The federate identifier.
 * @param partner_id The partner federate identifier.
 * @param tag Pointer to the tag that has been received, or NULL.
 */
void tracepoint_federate_from_federate(trace_t* trace, trace_event_t event_type, int fed_id, int partner_id, tag_t *tag);

#else
#define tracepoint_federate_to_rti(...);
#define tracepoint_federate_from_rti(...);
#define tracepoint_federate_to_federate(...);
#define tracepoint_federate_from_federate(...);
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
void tracepoint_rti_to_federate(trace_t* trace, trace_event_t event_type, int fed_id, tag_t* tag);

/**
 * Trace RTI receiving a message from a federate.
 * @param event_type The type of event. Possible values are:
 * 
 * @param fed_id The fedaerate ID.
 * @param tag Pointer to the tag that has been sent, or NULL.
 */
void tracepoint_rti_from_federate(trace_t* trace, trace_event_t event_type, int fed_id, tag_t* tag);

#else
#define tracepoint_rti_to_federate(...);
#define tracepoint_rti_from_federate(...) ;
#endif // RTI_TRACE

#else
typedef struct trace_t trace_t;

// empty definition in case we compile without tracing
#define _lf_register_trace_event(...)
#define register_user_trace_event(...)
#define tracepoint(...)
#define tracepoint_reaction_starts(...)
#define tracepoint_reaction_ends(...)
#define tracepoint_schedule(...)
#define tracepoint_user_event(...)
#define tracepoint_user_value(...)
#define tracepoint_worker_wait_starts(...)
#define tracepoint_worker_wait_ends(...)
#define tracepoint_scheduler_advancing_time_starts(...);
#define tracepoint_scheduler_advancing_time_ends(...);
#define tracepoint_reaction_deadline_missed(...);
#define tracepoint_federate_to_rti(...);
#define tracepoint_federate_from_rti(...);
#define tracepoint_federate_to_federate(...) ;
#define tracepoint_federate_from_federate(...) ;
#define tracepoint_rti_to_federate(...);
#define tracepoint_rti_from_federate(...) ;

#define start_trace(...)
#define stop_trace(...)
#define stop_trace_locked(...)
#define trace_new(...) NULL
#define trace_free(...)


#endif // LF_TRACE
#endif // TRACE_H
