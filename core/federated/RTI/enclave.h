/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @author Erling Jellum (erling.r.jellum@ntnu.no)
 * @author Chadlia Jerad (chadlia.jerad@ensi-uma.tn)
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Declarations for runtime infrastructure (RTI) for scheduling enclaves and distributed Lingua Franca programs.
 * This file declares RTI features that are used by scheduling enclaves as well as federated
 * LF programs.
 */

#ifndef ENCLAVE_H
#define ENCLAVE_H

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

/** State of a enclave during execution. */
typedef enum fed_state_t {
    NOT_CONNECTED,  // The federate has not connected.
    GRANTED,        // Most recent MSG_TYPE_NEXT_EVENT_TAG has been granted.
    PENDING         // Waiting for upstream federates.
} fed_state_t;

/**
 * Information about enclave known to the RTI, including its runtime state,
 * mode of execution, and connectivity with other enclaves.
 * The list of upstream and downstream enclaves does not include
 * those that are connected via a "physical" connection (one
 * denoted with ~>) because those connections do not impose
 * any scheduling constraints.
 */
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
    lf_cond_t next_event_condition; // Condition variable used by enclaves to notify an enclave
                                    // that it's call to next_event_tag() should unblock.
} enclave_t;

/**
 * Structure that an enclave RTI instance uses to keep track of its own and its
 * corresponding enclaves'state.
 *     // **************** IMPORTANT!!! ********************
 *     // **   If you make any change to this struct,     **
 *     // **  you MUST also change federation_rti_t in    **
 *     // ** (rti_lib.h)! The change must exactly match.  **
 *     // **************************************************
 */

typedef struct enclave_rti_t {
    // The enclaves.
    enclave_t **enclaves;

    // Number of enclaves
    int32_t number_of_enclaves;

    // RTI's decided stop tag for enclaves
    tag_t max_stop_tag;

    // Number of enclaves handling stop
    int num_enclaves_handling_stop;

    // Boolean indicating that tracing is enabled.
    bool tracing_enabled;

    // Trace object
    trace_t* trace;
} enclave_rti_t;


/**
 * An enclave calls this function after it completed a tag. 
 * The function updates the completed tag and check if the downstream enclaves 
 * are eligible for receiving TAGs.
 * 
 * @param enclave The enclave
 * @param completed The completed tag of the enclave
 */
void logical_tag_complete(enclave_t* enclave, tag_t completed);

typedef struct {
    tag_t tag;           // NEVER if there is no tag advance grant.
    bool is_provisional; // True for PTAG, false for TAG.
} tag_advance_grant_t;

/** 
 * Initialize the enclave with the specified ID.
 * 
 * @param e The enclave
 * @param id The enclave ID.
 */
void initialize_enclave(enclave_t* e, uint16_t id);

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
 * Notify a tag advance grant (TAG) message to the specified federate.
 * Do not notify it if a previously sent PTAG was greater or if a
 * previously sent TAG was greater or equal.
 *
 * This function will keep a record of this TAG in the federate's last_granted
 * field.
 *
 * This function assumes that the caller holds the mutex lock.
 * 
 * FIXME: This needs two implementations, one for enclaves and one for federates.
 *
 * @param e The enclave.
 * @param tag The tag to grant.
 */
void notify_tag_advance_grant(enclave_t* e, tag_t tag);

/**
 * @brief Either send to a federate or unblock an enclave to give it a tag.
 * This function requires two different implementations, one for enclaves
 * and one for federates.
 * 
 * This assumes the caller holds the mutex.
 * 
 * @param e The enclave.
 */
void notify_advance_grant_if_safe(enclave_t* e);

/**
 * Nontify a provisional tag advance grant (PTAG) message to the specified enclave.
 * Do not notify it if a previously sent PTAG or TAG was greater or equal.
 *
 * This function will keep a record of this PTAG in the federate's last_provisionally_granted
 * field.
 *
 * This function assumes that the caller holds the mutex lock.
 *
 * FIXME: This needs two implementations, one for enclaves and one for federates.
 *
 * @param e The enclave.
 * @param tag The tag to grant.
 */
void notify_provisional_tag_advance_grant(enclave_t* e, tag_t tag);

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
 * @param e The enclave
 * @return If granted, return the tag value and whether it is provisional. 
 *  Otherwise, return the NEVER_TAG.
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
 * @return If granted, return the TAG and whether it is provisional or not. 
 *  Otherwise, return the NEVER_TAG.
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

/**
 * Find the earliest tag at which the specified federate may
 * experience its next event. This is the least next event tag (NET)
 * of the specified federate and (transitively) upstream federates
 * (with delays of the connections added). For upstream federates,
 * we assume (conservatively) that federate upstream of those
 * may also send an event. The result will never be less than
 * the completion time of the federate (which may be NEVER,
 * if the federate has not yet completed a logical time).
 *
 * FIXME: This could be made less conservative by building
 * at code generation time a causality interface table indicating
 * which outputs can be triggered by which inputs. For now, we
 * assume any output can be triggered by any input.
 *
 * @param e The enclave.
 * @param candidate A candidate tag (for the first invocation,
 *  this should be fed->next_event).
 * @param visited An array of booleans indicating which federates
 *  have been visited (for the first invocation, this should be
 *  an array of falses of size _RTI.number_of_federates).
 * @return The earliest next event tag of the enclave e.
 */
tag_t transitive_next_event(enclave_t *e, tag_t candidate, bool visited[]);

#endif // ENCLAVE_H