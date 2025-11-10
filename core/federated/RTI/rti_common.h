/**
 * @file rti_common.h
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Erling Jellum
 * @author Chadlia Jerad
 * @brief Common declarations for runtime infrastructure (RTI) for scheduling enclaves
 * and distributed Lingua Franca programs.
 * @ingroup RTI
 */
#if defined STANDALONE_RTI || defined LF_ENCLAVES
#ifndef RTI_COMMON_H
#define RTI_COMMON_H

#include <errno.h> // Defines perror(), errno
#include <assert.h>
#include "low_level_platform.h" // Platform-specific types and functions
#include "util.h"               // Defines print functions (e.g., lf_print).
#include "tag.h"                // Time-related types and functions.
#include "tracepoint.h"         // Tracing related functions

/**
 * @brief Mode of execution of a federate.
 * @ingroup RTI
 */
typedef enum execution_mode_t { FAST, REALTIME } execution_mode_t;

/**
 * @brief State of the scheduling node during execution.
 * @ingroup RTI
 */
typedef enum scheduling_node_state_t {
  /** @brief The scheduling node has not connected. */
  NOT_CONNECTED,
  /** @brief Most recent MSG_TYPE_NEXT_EVENT_TAG has been granted. */
  GRANTED,
  /** @brief Waiting for upstream scheduling nodes. */
  PENDING
} scheduling_node_state_t;

/**
 * @brief Struct for minimum delays from upstream nodes.
 * @ingroup RTI
 */
typedef struct minimum_delay_t {
  /** @brief ID of the upstream node. */
  int id;
  /** @brief Minimum delay from upstream. */
  tag_t min_delay;
} minimum_delay_t;

/**
 * @brief Information about the scheduling nodes coordinated by the RTI.
 * @ingroup RTI
 *
 * The abstract scheduling node could either be an enclave or a federate.
 * The information includes its runtime state,
 * mode of execution, and connectivity with other scheduling nodes.
 * The list of upstream and downstream scheduling nodes does not include
 * those that are connected via a "physical" connection (one
 * denoted with ~>) because those connections do not impose
 * any scheduling constraints.
 */
typedef struct scheduling_node_t {
  /** @brief ID of this scheduling node. */
  uint16_t id;
  /** @brief The largest logical tag completed by the scheduling node (or NEVER if no LTC has been received). */
  tag_t completed;
  /** @brief The maximum TAG that has been granted so far (or NEVER if none granted). */
  tag_t last_granted;
  /** @brief The maximum PTAG that has been provisionally granted (or NEVER if none granted). */
  tag_t last_provisionally_granted;
  /** @brief Most recent NET received from the scheduling node (or NEVER if none received). */
  tag_t next_event;
  /** @brief Most recent DNET. */
  tag_t last_DNET;
  /** @brief State of the scheduling node. */
  scheduling_node_state_t state;
  /** @brief Array of immediate upstream scheduling node ids. */
  uint16_t* immediate_upstreams;
  /** @brief Minimum delay on connections from immediate upstream scheduling nodes. NEVER encodes no delay. 0LL is a
   * microstep delay. */
  interval_t* immediate_upstream_delays;
  /** @brief Size of the array of immediate upstream scheduling nodes and delays. */
  uint16_t num_immediate_upstreams;
  /** @brief Array of immediate downstream scheduling node ids. */
  uint16_t* immediate_downstreams;
  /** @brief Size of the array of immediate downstream scheduling nodes. */
  uint16_t num_immediate_downstreams;
  /** @brief FAST or REALTIME. */
  execution_mode_t mode;
  /** @brief One of IS_IN_ZERO_DELAY_CYCLE, IS_IN_CYCLE. */
  int flags;
} scheduling_node_t;

/**
 * @brief Common RTI data structure for both remote standalone RTI and local RTI used in enclaved execution.
 * @ingroup RTI
 *
 * rti_remote_t and rti_local_t will "inherit" from this data structure. The first field is an array of pointers
 * to scheduling nodes. These will be scheduling nodes for the local RTI and federates for the remote RTI.
 */
typedef struct rti_common_t {
  /** @brief The scheduling nodes. */
  scheduling_node_t** scheduling_nodes;
  /** @brief Number of scheduling nodes. */
  uint16_t number_of_scheduling_nodes;
  /** @brief Matrix of minimum delays between pairs of nodes. Rows represent upstream nodes and columns represent
   * downstream nodes. FOREVER_TAG means there is no path, and ZERO_TAG means there is no delay. This could be NULL if
   * the matrix is not being used, so accesses should test for NULL first. */
  tag_t* min_delays;
  /** @brief RTI's decided stop tag for the scheduling nodes. */
  tag_t max_stop_tag;
  /** @brief Number of scheduling nodes handling stop. */
  int num_scheduling_nodes_handling_stop;
  /** @brief Boolean indicating that tracing is enabled. */
  bool tracing_enabled;
  /** @brief Boolean indicating that DNET is enabled. */
  bool dnet_disabled;
  /** @brief The RTI mutex for making thread-safe access to the shared state. */
  lf_mutex_t* mutex;
} rti_common_t;

typedef struct {
  /** @brief The tag value to grant, or NEVER if there is no tag advance grant. */
  tag_t tag;
  /** @brief True for PTAG, false for TAG. */
  bool is_provisional;
} tag_advance_grant_t;

/**
 * @brief Initialize the fields of the rti_common struct.
 * @ingroup RTI
 *
 * It also stores the pointer to the struct and uses it internally.
 *
 * @param rti_common The rti_common_t struct to initialize.
 */
void initialize_rti_common(rti_common_t* rti_common);

/**
 * @brief Update the completed tag for the specified node.
 * @ingroup RTI
 *
 * This checks whether any downstream nodes become eligible to receive TAG
 * or PTAG, and sends those signals if appropriate.
 *
 * The function is prepended with an underscore because a function called
 * `logical_tag_complete` is code-generated by the compiler.
 *
 * @param e The scheduling node.
 * @param completed The completed tag of the scheduling node.
 */
void _logical_tag_complete(scheduling_node_t* e, tag_t completed);

/**
 * @brief Initialize the scheduling node with the specified ID.
 * @ingroup RTI
 *
 * @param e The scheduling node.
 * @param id The scheduling node ID.
 */
void initialize_scheduling_node(scheduling_node_t* e, uint16_t id);

/**
 * @brief For all scheduling nodes downstream of the specified node, determine
 * whether they should be notified of a TAG or PTAG and notify them if so.
 * @ingroup RTI
 *
 * This assumes the caller holds the RTI mutex.
 *
 * @param e The upstream node.
 * @param visited An array of booleans used to determine whether a node has
 *  been visited (initially all false).
 */
void notify_downstream_advance_grant_if_safe(scheduling_node_t* e, bool visited[]);

/**
 * @brief Notify a tag advance grant (TAG) message to the specified scheduling node.
 * @ingroup RTI
 *
 * Do not notify it if a previously sent PTAG was greater or if a
 * previously sent TAG was greater or equal.
 *
 * This function will keep a record of this TAG in the node's last_granted
 * field.
 *
 * This function assumes that the caller holds the RTI mutex.
 *
 * @param e The scheduling node.
 * @param tag The tag to grant.
 */
void notify_tag_advance_grant(scheduling_node_t* e, tag_t tag);

/**
 * @brief Either send to a federate or unblock an enclave to give it a tag.
 * @ingroup RTI
 *
 * This function requires two different implementations, one for enclaves
 * and one for federates.
 *
 * This assumes the caller holds the RTI mutex.
 *
 * @param e The scheduling node.
 */
void notify_advance_grant_if_safe(scheduling_node_t* e);

/**
 * @brief Notify a provisional tag advance grant (PTAG) message to the specified scheduling node.
 * @ingroup RTI
 *
 * Do not notify it if a previously sent PTAG or TAG was greater or equal.
 *
 * This function will keep a record of this PTAG in the node's last_provisionally_granted
 * field.
 *
 * This function assumes that the caller holds the RTI mutex.
 *
 * @param e The scheduling node.
 * @param tag The tag to grant.
 */
void notify_provisional_tag_advance_grant(scheduling_node_t* e, tag_t tag);

/**
 * @brief Determine whether the specified scheduling node is eligible for a tag advance grant,
 * (TAG) and, if so, return the details.
 * @ingroup RTI
 *
 * This is called upon receiving a LTC, NET or resign from an upstream node.
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
 * the RTI an LTC (latest tag confirmed), or when a transitive upstream
 * federate sends a NET (Next Event Tag) message.
 * It is also called when an upstream federate resigns from the federation.
 *
 * This function assumes that the caller holds the RTI mutex.
 *
 * @param e The scheduling node.
 * @return If granted, return the tag value and whether it is provisional.
 *  Otherwise, return the NEVER_TAG.
 */
tag_advance_grant_t tag_advance_grant_if_safe(scheduling_node_t* e);

/**
 * @brief Update the next event tag of an scheduling node.
 * @ingroup RTI
 *
 * This will notify downstream scheduling nodes with a TAG or PTAG if appropriate.
 *
 * This function assumes that the caller is holding the RTI mutex.
 *
 * @param e The scheduling node.
 * @param next_event_tag The next event tag for e.
 */
void update_scheduling_node_next_event_tag_locked(scheduling_node_t* e, tag_t next_event_tag);

/**
 * @brief Given a node (enclave or federate), find the tag of the earliest possible incoming
 * message (EIMT) from upstream enclaves or federates, which will be the smallest upstream NET
 * plus the least delay.
 * @ingroup RTI
 *
 * This could be NEVER_TAG if the RTI has not seen a NET from some upstream node.
 *
 * @param e The target node.
 * @return The earliest possible incoming message tag.
 */
tag_t earliest_future_incoming_message_tag(scheduling_node_t* e);

/**
 * @brief Given a node (enclave or federate), find the earliest incoming message tag (EIMT) from
 * any immediately upstream node that is not part of zero-delay cycle (ZDC).
 * @ingroup RTI
 *
 * These tags are treated strictly by the RTI when deciding whether to grant a PTAG.
 * Since the upstream node is not part of a ZDC, there is no need to block on the input
 * from that node since we can simply wait for it to complete its tag without chance of
 * introducing a deadlock.  This will return FOREVER_TAG if there are no non-ZDC upstream nodes.
 *
 * @param e The target node.
 * @return The earliest possible incoming message tag from a non-ZDC upstream node.
 */
tag_t eimt_strict(scheduling_node_t* e);

/**
 * @brief If necessary, update the `min_delays` and the fields that indicate cycles.
 * @ingroup RTI
 *
 * These fields will be updated only if they have not been previously updated or if invalidate_min_delays
 * has been called since they were last updated.
 */
void update_min_delays();

/**
 * @brief Find the tag g that is the latest tag that satisfies lf_tag_add(g, minimum_delay) < next_event_tag.
 * @ingroup RTI
 *
 * This function behaves like the tag subtraction, next_event_tag - minimum_delay.
 * minimum_delay cannot be NEVER.
 *
 * This function is called in function downstream_next_event_tag.
 * @param next_event_tag The next event tag of a downstream node.
 * @param minimum_delay The minimum delay between the target upstream node and the downstream node.
 */
tag_t get_dnet_candidate(tag_t next_event_tag, tag_t minimum_delay);

/**
 * @brief Determine whether the specified scheduling node is needed to receive a downstream next event tag (DNET),
 * and, if so, return the details.
 * @ingroup RTI
 *
 * This function is called upon receiving a NET from one of the specified node's downstream nodes.
 *
 * This function calculates the minimum tag M over
 * all downstream scheduling nodes of the most recent NET from that node minus the "after delay" (see function
 * get_dnet_candidate). If M is earlier than the startup tag, then set the result as the NEVER_TAG.
 *
 * @param node The target node that may receive a new DNET.
 * @param node_sending_new_net_id The ID of the node that sends a new NET. If this node's new NET does not
 * change the DNET value, we can exit this function immediately. If it does, we have to look up the target node's
 * downstream federates to compute the exact new DNET value.
 * @return If needed, return the tag value. Otherwise, return the NEVER_TAG.
 */
tag_t downstream_next_event_tag(scheduling_node_t* node, uint16_t node_sending_new_net_id);

/**
 * @brief Notify a downstream next event tag (DNET) signal to the specified scheduling node.
 * @ingroup RTI
 *
 * @param e The target node.
 * @param tag The downstream next event tag for e.
 */
void notify_downstream_next_event_tag(scheduling_node_t* e, tag_t tag);

/**
 * @brief Return true if the node is in a zero-delay cycle.
 * @ingroup RTI
 *
 * @param node The node.
 */
bool is_in_zero_delay_cycle(scheduling_node_t* node);

/**
 * @brief Return true if the node is in a cycle (possibly a zero-delay cycle).
 * @ingroup RTI
 *
 * @param node The node.
 */
bool is_in_cycle(scheduling_node_t* node);

/**
 * @brief Invalidate the `min_delays`, `num_min_delays`, and the fields that indicate cycles
 * of all nodes.
 * @ingroup RTI
 *
 * This should be called whenever the structure of the connections have changed.
 */
void invalidate_min_delays();

/**
 * @brief Free dynamically allocated memory on the scheduling nodes and the scheduling node array itself.
 * @ingroup RTI
 *
 * @param scheduling_nodes The scheduling nodes.
 * @param number_of_scheduling_nodes The number of scheduling nodes.
 */
void free_scheduling_nodes(scheduling_node_t** scheduling_nodes, uint16_t number_of_scheduling_nodes);

#endif // RTI_COMMON_H
#endif // STANDALONE_RTI || LF_ENCLAVES
