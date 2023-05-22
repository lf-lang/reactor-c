#ifndef ENCLAVE_H
#define ENCLAVE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>      // Defines perror(), errno
#include <assert.h>
#include "platform.h"   // Platform-specific types and functions
#include "util.h"       // Defines print functions (e.g., lf_print).
#include "net_util.h"   // Defines network functions.
#include "net_common.h" // Defines message types, etc. Includes <pthread.h> and "reactor.h".
#include "tag.h"        // Time-related types and functions.
#include "trace.h"      // Tracing related functions


/** Mode of execution of a federate. */
typedef enum execution_mode_t {
    FAST,
    REALTIME
} execution_mode_t;

/** State of a federate during execution. */
typedef enum fed_state_t {
    NOT_CONNECTED,  // The federate has not connected.
    GRANTED,        // Most recent MSG_TYPE_NEXT_EVENT_TAG has been granted.
    PENDING         // Waiting for upstream federates.
} fed_state_t;

typedef struct enclave_t {
    uint16_t id;            // ID of this enclave.
    tag_t completed;        // The largest logical tag completed by the federate (or NEVER if no LTC has been received).
    tag_t last_granted;     // The maximum TAG that has been granted so far (or NEVER if none granted)
    tag_t last_provisionally_granted;      // The maximum PTAG that has been provisionally granted (or NEVER if none granted)
    tag_t next_event;       // Most recent NET received from the federate (or NEVER if none received).
    fed_state_t state;      // State of the federate.
    int* upstream;          // Array of upstream federate ids.
    interval_t* upstream_delay;    // Minimum delay on connections from upstream federates.
    							   // Here, NEVER encodes no delay. 0LL is a microstep delay.
    int num_upstream;              // Size of the array of upstream federates and delays.
    int* downstream;        // Array of downstream federate ids.
    int num_downstream;     // Size of the array of downstream federates.
    execution_mode_t mode;  // FAST or REALTIME.
    bool requested_stop;    // Indicates that the federate has requested stop or has replied
                            // to a request for stop from the RTI. Used to prevent double-counting
                            // a federate when handling lf_request_stop().
    lf_cond_t next_event_condition; // Condition variable used by enclaves to notify an enclave
                                    // that it's call to next_event_tag() should unblock.
// FIXME: This has to be initialized!
// lf_cond_init(&next_event_condition, &rti_mutex);
} enclave_t;

// FIXME: Docs
void logical_tag_complete(enclave_t* enclave, tag_t completed);

typedef struct {
    tag_t tag;           // NEVER if there is no tag advance grant.
    bool is_provisional; // True for PTAG, false for TAG.
} tag_advance_grant_t;

/**
 * For all enclaves downstream of the specified enclave, determine
 * whether they should be notified of a TAG or PTAG and notify them if so.
 *
 * This assumes the caller holds the mutex.
 *
 * @param e The upstream enclave.
 * @param visited An array of booleans used to determine whether an enclave has
 *  been visited (initially all false).
 */
void notify_downstream_advance_grant_if_safe(enclave_t* e, bool visited[]);

/**
 * @brief Either send to a federate or unblock an enclave to give it a tag.
 * This function requires two different implementations, one for enclaves
 * and one for federates.
 * FIXME: This is only implemented for federates right now.
 * @param e The enclave.
 */
void notify_advance_grant_if_safe(enclave_t* e);

/**
 * Determine whether the specified enclave is eligible for a tag advance grant,
 * (TAG) and, if so, return the details. This is called upon receiving a LTC, NET
 * or resign from an upstream enclave.
 *
 * This function calculates the minimum M over
 * all upstream enclaves of the "after" delay plus the most recently
 * received LTC from that enclave. If M is greater than the
 * most recent TAG to e or greater than or equal to the most
 * recent PTAG, then return TAG(M).
 *
 * If the above conditions do not result in returning a TAG, then find the
 * minimum M of the earliest possible future message from upstream federates.
 * This is calculated by transitively looking at the most recently received
 * NET calls from upstream enclaves.
 * If M is greater than the NET of e or the most recent PTAG to e, then
 * return a TAG with tag equal to the NET of e or the PTAG.
 * If M is equal to the NET of the federate, then return PTAG(M).
 *
 * This should be called whenever an immediately upstream federate sends to
 * the RTI an LTC (Logical Tag Complete), or when a transitive upstream
 * federate sends a NET (Next Event Tag) message.
 * It is also called when an upstream federate resigns from the federation.
 *
 * This function assumes that the caller holds the mutex lock.
 *
 * @return True if the TAG message is sent and false otherwise.
 */
tag_advance_grant_t tag_advance_grant_if_safe(enclave_t* e);

/**
 * @brief Get the tag to advance to.
 *
 * An enclave should call this function when it is ready to advance its tag,
 * passing as the second argument the tag of the earliest event on its event queue.
 * The returned tag may be less than or equal to the argument tag and is interpreted
 * by the enclave as the tag to which it can advance.
 * 
 * This will also notify downstream enclaves with a TAG or PTAG if appropriate,
 * possibly unblocking their own calls to this same function.
 *
 * @param e The enclave.
 * @param next_event_tag The next event tag for e.
 */
tag_advance_grant_t next_event_tag(enclave_t* e, tag_t next_event_tag);

/**
 * @brief Update the next event tag of an enclave.
 *
 * This will notify downstream enclaves with a TAG or PTAG if appropriate.
 *
 * This function assumes that the caller is holding the rti_mutex.
 *
 * @param e The enclave.
 * @param next_event_tag The next event tag for e.
 */
void update_enclave_next_event_tag_locked(enclave_t* e, tag_t next_event_tag);

#endif // ENCLAVE_H