/**
 * @file trace_types.h
 * @author Peter Donovan
 *
 * @brief Definitions that are needed by both implementors and callers of the
 * trace API regardless of whether tracing is enabled at compile time.
 * @ingroup Tracing
 */

#ifndef TRACE_TYPES_H
#define TRACE_TYPES_H

/**
 * @brief Trace event types.
 * @ingroup Tracing
 *
 * If you update this, be sure to update the string representation below.
 * Also, create a tracepoint function for each event type.
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
  send_DNET,
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
  receive_DNET,
  receive_UNIDENTIFIED,
  NUM_EVENT_TYPES
} trace_event_t;

/**
 * @brief String description of event types.
 * @ingroup Tracing
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
    "Sending DNET",
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
    "Receiving DNET",
    "Receiving UNIDENTIFIED",
};

static inline void _suppress_unused_variable_warning_for_static_variable() { (void)trace_event_names; }

#endif
