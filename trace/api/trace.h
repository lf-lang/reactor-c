#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include <stdbool.h>

#include "lf_core_version.h"

/**
 * Trace event types. If you update this, be sure to update the
 * string representation below. Also, create a tracepoint function
 * for each event type.
 */
typedef enum {
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

/**
 * String description of event types.
 */
static const char* trace_event_names[] = {
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

/**
 * @brief Return a description of the compile-time properties of the current
 * plugin.
 */
version_t lf_version_tracing();

/**
 * Identifier for what is in the object table.
 */
typedef enum {
  trace_reactor, // Self struct.
  trace_trigger, // Timer or action (argument to schedule()).
  trace_user     // User-defined trace object.
} _lf_trace_object_t;

/**
 * Struct for table of pointers to a description of the object.
 */
typedef struct object_description_t object_description_t;
struct object_description_t {
  void* pointer;           // Pointer-sized value that uniquely identifies the object.
  void* trigger;           // Pointer to the trigger (action or timer) or other secondary ID, if any.
  _lf_trace_object_t type; // The type of trace object.
  char* description;       // A NULL terminated string.
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
void lf_tracing_tracepoint(int worker, trace_record_nodeps_t* tr);
/**
 * @brief Shut down the tracing module. Calling other API functions after
 * calling this procedure is undefined behavior.
 */
void lf_tracing_global_shutdown();

#endif // TRACE_H
