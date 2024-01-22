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

// FIXME: Should scheduling_nodes tracing use the same mechanism as federates? 
//        It needs to account a federate having itself a number of scheduling_nodes.
//        Currently, all calls to tracepoint_from_federate() and 
//        tracepoint_to_federate() are in rti_lib.c

#define IS_IN_ZERO_DELAY_CYCLE 1
#define IS_IN_CYCLE 2

void invalidate_min_delays_upstream(scheduling_node_t* node) {
    if(node->min_delays != NULL) free(node->min_delays);
    node->min_delays = NULL;
    node->num_min_delays = 0;
    node->flags = 0; // All flags cleared because they get set lazily.
}

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
    invalidate_min_delays_upstream(e);
}

void _logical_tag_complete(scheduling_node_t* enclave, tag_t completed) {
    // FIXME: Consolidate this message with NET to get NMR (Next Message Request).
    // Careful with handling startup and shutdown.
    lf_mutex_lock(rti_common->mutex);

    enclave->completed = completed;

    LF_PRINT_LOG("RTI received from federate/enclave %d the latest tag complete (LTC) " PRINTF_TAG ".",
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
    // Update the shortest paths, if necessary.
    update_min_delays_upstream(e);

    // Next, find the tag of the earliest possible incoming message from upstream enclaves or
    // federates, which will be the smallest upstream NET plus the least delay.
    // This could be NEVER_TAG if the RTI has not seen a NET from some upstream node.
    tag_t t_d = FOREVER_TAG;
    for (int i = 0; i < e->num_min_delays; i++) {
        // Node e->min_delays[i].id is upstream of e with min delay e->min_delays[i].min_delay.
        scheduling_node_t* upstream = rti_common->scheduling_nodes[e->min_delays[i].id];
        // If we haven't heard from the upstream node, then assume it can send an event at the start time.
        if (lf_tag_compare(upstream->next_event, NEVER_TAG) == 0) {
            tag_t start_tag = {.time = start_time, .microstep = 0};
            upstream->next_event = start_tag;
        }
        // The min_delay here is a tag_t, not an interval_t because it may account for more than
        // one connection. No delay at all is represented by (0,0). A delay of 0 is represented
        // by (0,1). If the time part of the delay is greater than 0, then we want to ignore
        // the microstep in upstream->next_event because that microstep will have been lost.
        // Otherwise, we want preserve it and add to it. This is handled by lf_tag_add().
        tag_t earliest_tag_from_upstream = lf_tag_add(upstream->next_event, e->min_delays[i].min_delay);

        /* Following debug message is too verbose for normal use:
        LF_PRINT_DEBUG("RTI: Earliest next event upstream of fed/encl %d at fed/encl %d has tag " PRINTF_TAG ".",
                e->id,
                upstream->id,
                earliest_tag_from_upstream.time - start_time, earliest_tag_from_upstream.microstep);
        */
        if (lf_tag_compare(earliest_tag_from_upstream, t_d) < 0) {
            t_d = earliest_tag_from_upstream;
        }
    }
    return t_d;
}

tag_t eimt_strict(scheduling_node_t* e) {
    // Find the tag of the earliest possible incoming message from immediately upstream
    // enclaves or federates that are not part of a zero-delay cycle.
    // This will be the smallest upstream NET plus the least delay.
    // This could be NEVER_TAG if the RTI has not seen a NET from some upstream node.
    tag_t t_d = FOREVER_TAG;
    for (int i = 0; i < e->num_upstream; i++) {
        scheduling_node_t* upstream = rti_common->scheduling_nodes[e->upstream[i]];
        // Skip this node if it is part of a zero-delay cycle.
        if (is_in_zero_delay_cycle(upstream)) continue;
        // If we haven't heard from the upstream node, then assume it can send an event at the start time.
        if (lf_tag_compare(upstream->next_event, NEVER_TAG) == 0) {
            tag_t start_tag = {.time = start_time, .microstep = 0};
            upstream->next_event = start_tag;
        }
        // Need to consider nodes that are upstream of the upstream node because those
        // nodes may send messages to the upstream node.
        tag_t earliest = earliest_future_incoming_message_tag(upstream);
        // If the next event of the upstream node is earlier, then use that.
        if (lf_tag_compare(upstream->next_event, earliest) < 0) {
            earliest = upstream->next_event;
        }
        tag_t earliest_tag_from_upstream = lf_delay_tag(earliest, e->upstream_delay[i]);
        LF_PRINT_DEBUG("RTI: Strict EIMT of fed/encl %d at fed/encl %d has tag " PRINTF_TAG ".",
                e->id,
                upstream->id,
                earliest_tag_from_upstream.time - start_time, earliest_tag_from_upstream.microstep);
        if (lf_tag_compare(earliest_tag_from_upstream, t_d) < 0) {
            t_d = earliest_tag_from_upstream;
        }
    }
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
    // Non-ZDC version of the above. This is a tag that must be strictly greater than
    // that of the next granted PTAG.
    tag_t t_d_strict = eimt_strict(e);

    LF_PRINT_LOG("RTI: Earliest next event upstream of node %d has tag " PRINTF_TAG ".",
            e->id, t_d.time - start_time, t_d.microstep);

    // Given an EIMT (earliest incoming message tag) there are these possible scenarios:
    //  1) The EIMT is greater than the NET we want to advance to. Grant a TAG.
    //  2) The EIMT is equal to the NET and the strict EIMT is greater than the net
    //     and the federate is part of a zero-delay cycle (ZDC).  Grant a PTAG.
    //  3) Otherwise, grant nothing and wait for further updates.

    if ( // Scenario (1) above
        lf_tag_compare(t_d, e->next_event) > 0                      // EIMT greater than NET
        && lf_tag_compare(e->next_event, NEVER_TAG) > 0             // NET is not NEVER_TAG
        && lf_tag_compare(t_d, e->last_provisionally_granted) >= 0  // The grant is not redundant
                                                                    // (equal is important to override any previous
                                                                    // PTAGs).
        && lf_tag_compare(t_d, e->last_granted) > 0                 // The grant is not redundant.
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
    } else if( // Scenario (2) above
        lf_tag_compare(t_d, e->next_event) == 0                     // EIMT equal to NET
        && is_in_zero_delay_cycle(e)                                // The node is part of a ZDC
        && lf_tag_compare(t_d_strict, e->next_event) > 0            // The strict EIMT is greater than the NET
        && lf_tag_compare(t_d, e->last_provisionally_granted) > 0   // The grant is not redundant
        && lf_tag_compare(t_d, e->last_granted) > 0                 // The grant is not redundant.
    ) { 
        // Some upstream node may send an event that has the same tag as this node's next event,
        // so we can only grant a PTAG.
        LF_PRINT_LOG("RTI: Earliest upstream message time for fed/encl %d is " PRINTF_TAG
            " (adjusted by after delay). Granting provisional tag advance (PTAG) for " PRINTF_TAG,
            e->id,
            t_d.time - start_time, t_d.microstep,
            e->next_event.time - lf_time_start(),
            e->next_event.microstep);
        result.tag = e->next_event;
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
    } else {
        // Even though there was no grant, mark the tag as if there was.
        e->last_granted = next_event_tag;
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

// Local function used recursively to find minimum delays upstream.
// Return in count the number of non-FOREVER_TAG entries in path_delays[].
static void _update_min_delays_upstream(
        scheduling_node_t* end,
        scheduling_node_t* intermediate,
        tag_t path_delays[],
        size_t* count) {
    // On first call, intermediate will be NULL, so the path delay is initialized to zero.
    tag_t delay_from_intermediate_so_far = ZERO_TAG;
    if (intermediate == NULL) {
        intermediate = end;
    } else {
        // Not the first call, so intermediate is upstream of end.
        delay_from_intermediate_so_far = path_delays[intermediate->id];
    }
    if (intermediate->state == NOT_CONNECTED) {
        // Enclave or federate is not connected.
        // No point in checking upstream scheduling_nodes.
        return;
    }
    // Check nodes upstream of intermediate (or end on first call).
    // NOTE: It would be better to iterate through these sorted by minimum delay,
    // but for most programs, the gain might be negligible since there are relatively few
    // upstream nodes.
    for (int i = 0; i < intermediate->num_upstream; i++) {
        // Add connection delay to path delay so far.
        tag_t path_delay = lf_delay_tag(delay_from_intermediate_so_far, intermediate->upstream_delay[i]);
        // If the path delay is less than the so-far recorded path delay from upstream, update upstream.
        if (lf_tag_compare(path_delay, path_delays[intermediate->upstream[i]]) < 0) {
            if (path_delays[intermediate->upstream[i]].time == FOREVER) {
                // Found a finite path.
                *count = *count + 1;
            }
            path_delays[intermediate->upstream[i]] = path_delay;
            // Since the path delay to upstream has changed, recursively update those upstream of it.
            // Do not do this, however, if the upstream node is the end node because this means we have
            // completed a cycle.
            if (end->id != intermediate->upstream[i]) {
                _update_min_delays_upstream(end, rti_common->scheduling_nodes[intermediate->upstream[i]], path_delays, count);
            } else {
                // Found a cycle.
                end->flags = end->flags | IS_IN_CYCLE;
                // Is it a zero-delay cycle?
                if (lf_tag_compare(path_delay, ZERO_TAG) == 0 && intermediate->upstream_delay[i] < 0) {
                    end->flags = end->flags | IS_IN_ZERO_DELAY_CYCLE;
                } else {
                    // Clear the flag.
                    end->flags = end->flags & ~IS_IN_ZERO_DELAY_CYCLE;
                }
            }
        }
    }
}

void update_min_delays_upstream(scheduling_node_t* node) {
    // Check whether cached result is valid.
    if (node->min_delays == NULL) {

        // This is not Dijkstra's algorithm, but rather one optimized for sparse upstream nodes.
        // There must be a name for this algorithm.

        // Array of results on the stack:
        tag_t path_delays[rti_common->number_of_scheduling_nodes];
        // This will be the number of non-FOREVER entries put into path_delays.
        size_t count = 0;

        for (int i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
            path_delays[i] = FOREVER_TAG;
        }
        _update_min_delays_upstream(node, NULL, path_delays, &count);

        // Put the results onto the node's struct.
        node->num_min_delays = count;
        node->min_delays = (minimum_delay_t*)calloc(count, sizeof(minimum_delay_t));
        LF_PRINT_DEBUG("++++ Node %hu is in ZDC: %d", node->id, is_in_zero_delay_cycle(node));
        int k = 0;
        for (int i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
            if (lf_tag_compare(path_delays[i], FOREVER_TAG) < 0) {
                // Node i is upstream.
                if (k >= count) {
                    lf_print_error_and_exit("Internal error! Count of upstream nodes %zu for node %d is wrong!", count, i);
                }
                minimum_delay_t min_delay = {.id = i, .min_delay = path_delays[i]};
                node->min_delays[k++] = min_delay;
                // N^2 debug statement could be a problem with large benchmarks.
                // LF_PRINT_DEBUG("++++    Node %hu is upstream with delay" PRINTF_TAG "\n", i, path_delays[i].time, path_delays[i].microstep);
            }
        }
    }
}

bool is_in_zero_delay_cycle(scheduling_node_t* node) {
    update_min_delays_upstream(node);
    return node->flags & IS_IN_ZERO_DELAY_CYCLE;
}

bool is_in_cycle(scheduling_node_t* node) {
    update_min_delays_upstream(node);
    return node->flags & IS_IN_CYCLE;
}

#endif
