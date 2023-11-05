/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @author Erling Jellum (erling.r.jellum@ntnu.no)
 * @author Chadlia Jerad (chadlia.jerad@ensi-uma.tn)
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 */
#if defined STANDALONE_RTI || defined LF_ENCLAVES
#include "rti_common.h"

/**
 * Local reference to rti_common_t instance.
 */
static rti_common_t* rti_common = NULL;

// Global variables defined in tag.c:
extern instant_t start_time;


void initialize_rti_common(rti_common_t * _rti_common) {
    rti_common = _rti_common;
    rti_common->max_stop_tag = NEVER_TAG;
    rti_common->number_of_scheduling_nodes = 0;
    rti_common->num_scheduling_nodes_handling_stop = 0;
}

// FIXME: For log and debug message in this file, what sould be kept: 'enclave', 
//        'federate', or 'enlcave/federate'? Currently its is 'enclave/federate'.
// FIXME: Should scheduling_nodes tracing use the same mechanism as federates? 
//        It needs to account a federate having itself a number of scheduling_nodes.
//        Currently, all calls to tracepoint_from_federate() and 
//        tracepoint_to_federate() are in rti_lib.c

void initialize_scheduling_node(scheduling_node_t* e, uint16_t id) {
    e->id = id;
    e->completed = NEVER_TAG;
    e->last_granted = NEVER_TAG;
    e->last_provisionally_granted = NEVER_TAG;
    e->next_event = NEVER_TAG;
    e->state = NOT_CONNECTED;
    e->upstream = NULL;
    e->upstream_delay = NULL;
    e->num_upstream = 0;
    e->downstream = NULL;
    e->num_downstream = 0;
    e->mode = REALTIME;

}

void _logical_tag_complete(scheduling_node_t* enclave, tag_t completed) {
    // FIXME: Consolidate this message with NET to get NMR (Next Message Request).
    // Careful with handling startup and shutdown.
    lf_mutex_lock(rti_common->mutex);

    enclave->completed = completed;

    LF_PRINT_LOG("RTI received from federate/enclave %d the Logical Tag Complete (LTC) " PRINTF_TAG ".",
                enclave->id, enclave->completed.time - start_time, enclave->completed.microstep);

    // Check downstream scheduling_nodes to see whether they should now be granted a TAG.
    for (int i = 0; i < enclave->num_downstream; i++) {
        scheduling_node_t *downstream = rti_common->scheduling_nodes[enclave->downstream[i]];
        // Notify downstream enclave if appropriate.
        notify_advance_grant_if_safe(downstream);
        bool *visited = (bool *)calloc(rti_common->number_of_scheduling_nodes, sizeof(bool)); // Initializes to 0.
        // Notify scheduling_nodes downstream of downstream if appropriate.
        notify_downstream_advance_grant_if_safe(downstream, visited);
        free(visited);
    }

    lf_mutex_unlock(rti_common->mutex);
}

tag_t earliest_future_incoming_message_tag(scheduling_node_t* e) {
    // First, we need to find the shortest path (minimum delay) path to each upstream node
    // and then find the minimum of the node's recorded NET plus the minimum path delay.
    // The following array will have the minimum path delay.
    // It needs to be initialized to FOREVER_TAG, except for the element corresponding to
    // this fed/enclave, which is initialized to (0,0).
    tag_t *path_delays = (tag_t *)malloc(rti_common->number_of_scheduling_nodes * sizeof(tag_t));
    for (int i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
        path_delays[i] = FOREVER_TAG;
    }
    path_delays[e->id] = ZERO_TAG;
    shortest_path_upstream(e, NULL, path_delays);

    // Next, find the tag of the earliest possible incoming message from upstream enclaves or
    // federates, which will be the smallest upstream NET plus the least delay.
    // This could be NEVER_TAG if the RTI has not seen a NET from some upstream node.
    tag_t t_d = FOREVER_TAG;
    for (int i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
        if (e->id != i && path_delays[i].time < FOREVER) {
            // Node i is upstream of e. Note that it could be that i == e.
            scheduling_node_t* upstream = rti_common->scheduling_nodes[i];
            tag_t earliest_tag_from_upstream = lf_tag_add(upstream->next_event, path_delays[i]);
            LF_PRINT_DEBUG("RTI: Earliest next event upstream of fed/encl %d at fed/encl %d has tag " PRINTF_TAG ".",
                    e->id,
                    upstream->id,
                    earliest_tag_from_upstream.time - start_time, earliest_tag_from_upstream.microstep);
            if (lf_tag_compare(earliest_tag_from_upstream, t_d) < 0) {
                t_d = earliest_tag_from_upstream;
            }
        }
    }
    free(path_delays);
    return t_d;
}

tag_advance_grant_t tag_advance_grant_if_safe(scheduling_node_t* e) {
    tag_advance_grant_t result = {.tag = NEVER_TAG, .is_provisional = false};

    // Find the earliest LTC of upstream scheduling_nodes (M).
    tag_t min_upstream_completed = FOREVER_TAG;

    for (int j = 0; j < e->num_upstream; j++) {
        scheduling_node_t *upstream = rti_common->scheduling_nodes[e->upstream[j]];

        // Ignore this enclave/federate if it is not connected.
        if (upstream->state == NOT_CONNECTED) continue;

        // Adjust by the "after" delay.
        // Note that "no delay" is encoded as NEVER,
        // whereas one microstep delay is encoded as 0LL.
        tag_t candidate = lf_delay_strict(upstream->completed, e->upstream_delay[j]);

        if (lf_tag_compare(candidate, min_upstream_completed) < 0) {
            min_upstream_completed = candidate;
        }
    }
    LF_PRINT_LOG("RTI: Minimum upstream LTC for federate/enclave %d is " PRINTF_TAG 
            "(adjusted by after delay).",
            e->id,
            min_upstream_completed.time - start_time, min_upstream_completed.microstep);
    if (lf_tag_compare(min_upstream_completed, e->last_granted) > 0
        && lf_tag_compare(min_upstream_completed, e->next_event) >= 0 // The enclave has to advance its tag
    ) {
        result.tag = min_upstream_completed;
        return result;
    }

    // Can't make progress based only on upstream LTCs.
    // If all (transitive) upstream scheduling_nodes of the enclave
    // have earliest event tags such that the
    // enclave can now advance its tag, then send it a TAG message.
    // Find the tag of the earliest event that may be later received from an upstream enclave
    // or federate (which includes any after delays on the connections).
    tag_t t_d = earliest_future_incoming_message_tag(e);

    LF_PRINT_DEBUG("NOTE: FOREVER is displayed as " PRINTF_TAG " and NEVER as " PRINTF_TAG,
                   FOREVER_TAG.time - start_time, FOREVER_TAG.microstep,
                   NEVER_TAG.time - start_time, 0);

    LF_PRINT_LOG("RTI: Earliest next event upstream of node %d has tag " PRINTF_TAG ".",
            e->id, t_d.time - start_time, t_d.microstep);

    if (
        lf_tag_compare(t_d, e->next_event) > 0       // The enclave has something to do.
        && lf_tag_compare(t_d, e->last_provisionally_granted) >= 0  // The grant is not redundant
                                                                      // (equal is important to override any previous
                                                                      // PTAGs).
        && lf_tag_compare(t_d, e->last_granted) > 0  // The grant is not redundant.
    ) {
        // No upstream node can send events that will be received with a tag less than or equal to
        // e->next_event, so it is safe to send a TAG.
        LF_PRINT_LOG("RTI: Earliest upstream message time for fed/encl %d is " PRINTF_TAG
                "(adjusted by after delay). Granting tag advance (TAG) for " PRINTF_TAG,
                e->id,
                t_d.time - lf_time_start(), t_d.microstep,
                e->next_event.time - lf_time_start(),
                e->next_event.microstep);
        result.tag = e->next_event;
    } else if (
        // FIXME: if e->is_in_zero_delay_cycle
        lf_tag_compare(t_d, e->next_event) == 0      // The enclave has something to do/
        && lf_tag_compare(t_d, e->last_provisionally_granted) > 0  // The grant is not redundant.
        && lf_tag_compare(t_d, e->last_granted) > 0  // The grant is not redundant.
    ) {
        // Some upstream node may send an event that has the same tag as this node's next event,
        // so we can only grant a PTAG.
        LF_PRINT_LOG("RTI: Earliest upstream message time for fed/encl %d is " PRINTF_TAG
            " (adjusted by after delay). Granting provisional tag advance (PTAG).",
            e->id,
            t_d.time - start_time, t_d.microstep);
        result.tag = t_d;
        result.is_provisional = true;
    }
    return result;
}

void notify_downstream_advance_grant_if_safe(scheduling_node_t* e, bool visited[]) {
    visited[e->id] = true;
    for (int i = 0; i < e->num_downstream; i++) {
        scheduling_node_t* downstream = rti_common->scheduling_nodes[e->downstream[i]];
        if (visited[downstream->id]) continue;
        notify_advance_grant_if_safe(downstream);
        notify_downstream_advance_grant_if_safe(downstream, visited);
    }
}

void update_scheduling_node_next_event_tag_locked(scheduling_node_t* e, tag_t next_event_tag) {
    e->next_event = next_event_tag;

    LF_PRINT_DEBUG(
       "RTI: Updated the recorded next event tag for federate/enclave %d to " PRINTF_TAG,
       e->id,
       next_event_tag.time - lf_time_start(),
       next_event_tag.microstep
    );

    // Check to see whether we can reply now with a tag advance grant.
    // If the enclave has no upstream scheduling_nodes, then it does not wait for
    // nor expect a reply. It just proceeds to advance time.
    if (e->num_upstream > 0) {
        notify_advance_grant_if_safe(e);
    }
    // Check downstream scheduling_nodes to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which downstream scheduling_nodes have been visited.
    bool *visited = (bool *)calloc(rti_common->number_of_scheduling_nodes, sizeof(bool)); // Initializes to 0.
    notify_downstream_advance_grant_if_safe(e, visited);
    free(visited);
}


void notify_advance_grant_if_safe(scheduling_node_t* e) {
    tag_advance_grant_t grant = tag_advance_grant_if_safe(e);
    if (lf_tag_compare(grant.tag, NEVER_TAG) != 0) {
        if (grant.is_provisional) {
            notify_provisional_tag_advance_grant(e, grant.tag);
        } else {
            notify_tag_advance_grant(e, grant.tag);
        }
    }
}

void shortest_path_upstream(scheduling_node_t* end, scheduling_node_t* intermediate, tag_t path_delays[]) {
    // On first call, intermediate will be NULL, so the path delay is initialized to zero.
    tag_t delay_from_intermediate_so_far = ZERO_TAG;
    if (intermediate == NULL) {
        intermediate = end;
    } else {
        // Not the first call, so intermediate is upstream of end.
        delay_from_intermediate_so_far = path_delays[intermediate->id];
    }
    // NOTE: Except for this one check, the information calculated here is completely static
    // and could be precomputed at compile time.  But doing so would then cause transient
    // federates in feedback loops to potentially cause deadlocks by being absent.
    if (intermediate->state == NOT_CONNECTED) {
        // Enclave or federate is not connected.
        // No point in checking upstream scheduling_nodes.
        return;
    }
    // Check nodes upstream of intermediate (or end on first call).
    for (int i = 0; i < intermediate->num_upstream; i++) {
        // Add connection delay to path delay so far.
        tag_t path_delay = lf_delay_tag(delay_from_intermediate_so_far, intermediate->upstream_delay[i]);
        // If the path delay is less than the so-far recorded path delay from upstream, update upstream.
        if (lf_tag_compare(path_delay, path_delays[intermediate->upstream[i]]) < 0) {
            path_delays[intermediate->upstream[i]] = path_delay;
            // Since the path delay to upstream has changed, recursively update those upstream of it.
            // Do not do this, however, if the upstream node is the end node because this means we have
            // completed a cycle.
            if (end->id != intermediate->upstream[i]) {
                shortest_path_upstream(end, rti_common->scheduling_nodes[intermediate->upstream[i]], path_delays);
            }
        }
    }
}
#endif
