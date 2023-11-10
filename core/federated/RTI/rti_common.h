/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @author Erling Jellum (erling.r.jellum@ntnu.no)
 * @author Chadlia Jerad (chadlia.jerad@ensi-uma.tn)
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Common declarations for runtime infrastructure (RTI) for scheduling enclaves and distributed Lingua Franca programs.
 * This file declares RTI features that are used by scheduling enclaves as well as federated
 * LF programs.
 */
#if defined STANDALONE_RTI || defined LF_ENCLAVES
#ifndef RTI_COMMON_H
#define RTI_COMMON_H

#include <errno.h>      // Defines perror(), errno
#include <assert.h>
#include "platform.h"   // Platform-specific types and functions
#include "util.h"       // Defines print functions (e.g., lf_print).
#include "tag.h"        // Time-related types and functions.
#include "trace.h"      // Tracing related functions

/** Mode of execution of a federate. */
typedef enum execution_mode_t {
    FAST,
    REALTIME
} execution_mode_t;

/** State of the scheduling node during execution. */
typedef enum scheduling_node_state_t {
    NOT_CONNECTED,  // The scheduling node has not connected.
    GRANTED,        // Most recent MSG_TYPE_NEXT_EVENT_TAG has been granted.
    PENDING         // Waiting for upstream scheduling nodes.
} scheduling_node_state_t;

/**
 * Information about the scheduling nodes coordinated by the RTI.
 * The abstract scheduling node could either be an enclave or a federate.
 * The information includes its runtime state,
 * mode of execution, and connectivity with other scheduling nodes.
 * The list of upstream and downstream scheduling nodes does not include
 * those that are connected via a "physical" connection (one
 * denoted with ~>) because those connections do not impose
 * any scheduling constraints.
 */
typedef struct scheduling_node_t {
    uint16_t id;                        // ID of this scheduling node.
    tag_t completed;                    // The largest logical tag completed by the scheduling node 
                                        // (or NEVER if no LTC has been received).
    tag_t last_granted;                 // The maximum TAG that has been granted so far (or NEVER if none granted)
    tag_t last_provisionally_granted;   // The maximum PTAG that has been provisionally granted (or NEVER if none granted)
    tag_t next_event;                   // Most recent NET received from the scheduling node (or NEVER if none received).
    scheduling_node_state_t state;      // State of the scheduling node.
    int* upstream;                      // Array of upstream scheduling node ids.
    interval_t* upstream_delay;         // Minimum delay on connections from upstream scheduling nodes.
    			                        // Here, NEVER encodes no delay. 0LL is a microstep delay.
    int num_upstream;                   // Size of the array of upstream scheduling nodes and delays.
    int* downstream;                    // Array of downstream scheduling node ids.
    int num_downstream;                 // Size of the array of downstream scheduling nodes.
    execution_mode_t mode;              // FAST or REALTIME.
} scheduling_node_t;

/**
 * Data structure which is common to both the remote standalone RTI and the local RTI used in enclaved execution.
 * rti_remote_t and rti_local_t will "inherit" from this data structure. The first field is an array of pointers 
 * to scheduling nodes. These will be scheduling nodes for the local RTI and federates for the remote RTI 
 */
typedef struct rti_common_t {
    // The scheduling nodes.
    scheduling_node_t **scheduling_nodes;

    // Number of scheduling nodes
    int32_t number_of_scheduling_nodes;

    // RTI's decided stop tag for the scheduling nodes
    tag_t max_stop_tag;

    // Number of scheduling nodes handling stop
    int num_scheduling_nodes_handling_stop;

    // Boolean indicating that tracing is enabled.
    bool tracing_enabled;
    
    // Pointer to a tracing object
    trace_t* trace;

    // Lock for making thread-safe access to the shared state.
    lf_mutex_t* mutex;
} rti_common_t;

typedef struct {
    tag_t tag;           // NEVER if there is no tag advance grant.
    bool is_provisional; // True for PTAG, false for TAG.
} tag_advance_grant_t;

/**
 * @brief Initialize the fields of the rti_common struct. It also stores
 * the pointer to the struct and uses it internally.
 * 
 * @param The rti_common_t struct to initialize. 
 */
void initialize_rti_common(rti_common_t * rti_common);

/**
 * An scheduling node calls this function after it completed a tag. 
 * The function updates the completed tag and check if the downstream scheduling nodes 
 * are eligible for receiving TAGs.
 * 
 * @param e The scheduling node.
 * @param completed The completed tag of the scheduling node.
 */
// FIXME: Prepended with underscore due to conflict with code-generated function...
void _logical_tag_complete(scheduling_node_t* e, tag_t completed);

/** 
 * Initialize the scheduling node with the specified ID.
 * 
 * @param e The scheduling node.
 * @param id The scheduling node ID.
 */
void initialize_scheduling_node(scheduling_node_t* e, uint16_t id);

/**
 * For all scheduling nodes downstream of the specified node, determine
 * whether they should be notified of a TAG or PTAG and notify them if so.
 *
 * This assumes the caller holds the mutex.
 *
 * @param e The upstream node.
 * @param visited An array of booleans used to determine whether a node has
 *  been visited (initially all false).
 */
void notify_downstream_advance_grant_if_safe(scheduling_node_t* e, bool visited[]);

/**
 * Notify a tag advance grant (TAG) message to the specified scheduling node.
 * Do not notify it if a previously sent PTAG was greater or if a
 * previously sent TAG was greater or equal.
 *
 * This function will keep a record of this TAG in the node's last_granted
 * field.
 *
 * This function assumes that the caller holds the mutex lock.
 * 
 * @param e The scheduling node.
 * @param tag The tag to grant.
 */
void notify_tag_advance_grant(scheduling_node_t* e, tag_t tag);

/**
 * @brief Either send to a federate or unblock an enclave to give it a tag.
 * This function requires two different implementations, one for enclaves
 * and one for federates.
 * 
 * This assumes the caller holds the mutex.
 * 
 * @param e The scheduling node.
 */
void notify_advance_grant_if_safe(scheduling_node_t* e);

/**
 * Nontify a provisional tag advance grant (PTAG) message to the specified scheduling node.
 * Do not notify it if a previously sent PTAG or TAG was greater or equal.
 *
 * This function will keep a record of this PTAG in the node's last_provisionally_granted
 * field.
 *
 * This function assumes that the caller holds the mutex lock.
 *
 * @param e The scheduling node.
 * @param tag The tag to grant.
 */
void notify_provisional_tag_advance_grant(scheduling_node_t* e, tag_t tag);

/**
 * Determine whether the specified scheduling node is eligible for a tag advance grant,
 * (TAG) and, if so, return the details. This is called upon receiving a LTC, NET
 * or resign from an upstream node.
 *
 * This function calculates the minimum M over
 * all upstream scheduling nodes of the "after" delay plus the most recently
 * received LTC from that node. If M is greater than the
 * most recent TAG to e or greater than or equal to the most
 * recent PTAG, then return TAG(M).
 *
 * If the above conditions do not result in returning a TAG, then find the
 * minimum M of the earliest possible future message from upstream federates.
 * This is calculated by transitively looking at the most recently received
 * NET calls from upstream scheduling nodes.
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
 * @param e The scheduling node.
 * @return If granted, return the tag value and whether it is provisional. 
 *  Otherwise, return the NEVER_TAG.
 */
tag_advance_grant_t tag_advance_grant_if_safe(scheduling_node_t* e);


/**
 * @brief Update the next event tag of an scheduling node.
 *
 * This will notify downstream scheduling nodes with a TAG or PTAG if appropriate.
 *
 * This function assumes that the caller is holding the mutex.
 *
 * @param e The scheduling node.
 * @param next_event_tag The next event tag for e.
 */
void update_scheduling_node_next_event_tag_locked(scheduling_node_t* e, tag_t next_event_tag);

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
 * @param e The scheduling node.
 * @param candidate A candidate tag (for the first invocation,
 *  this should be fed->next_event).
 * @param visited An array of booleans indicating which federates
 *  have been visited (for the first invocation, this should be
 *  an array of falses of size _RTI.number_of_federates).
 * @return The earliest next event tag of the scheduling node e.
 */
tag_t transitive_next_event(scheduling_node_t *e, tag_t candidate, bool visited[]);

#endif // RTI_COMMON_H
#endif // STANDALONE_RTI || LF_ENCLAVES
