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
    if(node->all_upstreams != NULL) {
        free(node->all_upstreams);
        for (int i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
            rti_common->min_delays[i*rti_common->number_of_scheduling_nodes + node->id] = FOREVER_TAG;
        }
    }
    if(node->all_downstreams != NULL) free(node->all_downstreams);
    node->all_upstreams = NULL;
    node->num_all_upstreams = 0;
    node->all_downstreams = NULL;
    node->num_all_downstreams = 0;
    node->flags = 0; // All flags cleared because they get set lazily.
}

void initialize_scheduling_node(scheduling_node_t* e, uint16_t id) {
    e->id = id;
    e->completed = NEVER_TAG;
    e->last_granted = NEVER_TAG;
    e->last_provisionally_granted = NEVER_TAG;
    e->next_event = NEVER_TAG;
    e->last_DNET = NEVER_TAG;
    e->state = NOT_CONNECTED;
    e->immediate_upstreams = NULL;
    e->immediate_upstream_delays = NULL;
    e->num_immediate_upstreams = 0;
    e->immediate_downstreams = NULL;
    e->num_immediate_downstreams = 0;
    e->mode = REALTIME;
    invalidate_min_delays_upstream(e);
}

void _logical_tag_complete(scheduling_node_t* enclave, tag_t completed) {
    // FIXME: Consolidate this message with NET to get NMR (Next Message Request).
    // Careful with handling startup and shutdown.
    LF_MUTEX_LOCK(rti_common->mutex);

    enclave->completed = completed;

    LF_PRINT_LOG("RTI received from federate/enclave %d the latest tag complete (LTC) " PRINTF_TAG ".",
                enclave->id, enclave->completed.time - start_time, enclave->completed.microstep);

    // Check downstream scheduling_nodes to see whether they should now be granted a TAG.
    for (int i = 0; i < enclave->num_immediate_downstreams; i++) {
        scheduling_node_t *downstream = rti_common->scheduling_nodes[enclave->immediate_downstreams[i]];
        // Notify downstream enclave if appropriate.
        notify_advance_grant_if_safe(downstream);
        bool *visited = (bool *)calloc(rti_common->number_of_scheduling_nodes, sizeof(bool)); // Initializes to 0.
        // Notify scheduling_nodes downstream of downstream if appropriate.
        notify_downstream_advance_grant_if_safe(downstream, visited);
        free(visited);
    }

    LF_MUTEX_UNLOCK(rti_common->mutex);
}

tag_t earliest_future_incoming_message_tag(scheduling_node_t* e) {
    // First, we need to find the shortest path (minimum delay) path to each upstream node
    // and then find the minimum of the node's recorded NET plus the minimum path delay.
    // Update the shortest paths, if necessary.
    update_min_delays_upstream(e);
    update_all_downstreams(e);

    // Next, find the tag of the earliest possible incoming message from upstream enclaves or
    // federates, which will be the smallest upstream NET plus the least delay.
    // This could be NEVER_TAG if the RTI has not seen a NET from some upstream node.
    tag_t t_d = FOREVER_TAG;
    int n = rti_common->number_of_scheduling_nodes;
    for (int i = 0; i < e->num_all_upstreams; i++) {
        // Node e->all_upstreams[i] is upstream of e with
        // min delay rti_common->min_delays[e->all_upstreams[i]*n + e->id]
        scheduling_node_t* upstream = rti_common->scheduling_nodes[e->all_upstreams[i]];
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
        tag_t earliest_tag_from_upstream = lf_tag_add(upstream->next_event, rti_common->min_delays[e->all_upstreams[i]*n + e->id]);

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
    for (int i = 0; i < e->num_immediate_upstreams; i++) {
        scheduling_node_t* upstream = rti_common->scheduling_nodes[e->immediate_upstreams[i]];
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
        tag_t earliest_tag_from_upstream = lf_delay_tag(earliest, e->immediate_upstream_delays[i]);
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

    for (int j = 0; j < e->num_immediate_upstreams; j++) {
        scheduling_node_t *upstream = rti_common->scheduling_nodes[e->immediate_upstreams[j]];

        // Ignore this enclave/federate if it is not connected.
        if (upstream->state == NOT_CONNECTED) continue;

        // Adjust by the "after" delay.
        // Note that "no delay" is encoded as NEVER,
        // whereas one microstep delay is encoded as 0LL.
        tag_t candidate = lf_delay_strict(upstream->completed, e->immediate_upstream_delays[j]);

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
    for (int i = 0; i < e->num_immediate_downstreams; i++) {
        scheduling_node_t* downstream = rti_common->scheduling_nodes[e->immediate_downstreams[i]];
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
    if (e->num_immediate_upstreams > 0) {
        notify_advance_grant_if_safe(e);
    } else {
        // Even though there was no grant, mark the tag as if there was.
        e->last_granted = next_event_tag;
    }
    // Check downstream scheduling_nodes to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which downstream scheduling_nodes have been visited.
    // FIXME: As we have all_downstreams field now, we don't need the function notify_downstream_davnace_grnat_if_safe.
    update_all_downstreams(e);
    bool *visited = (bool *)calloc(rti_common->number_of_scheduling_nodes, sizeof(bool)); // Initializes to 0.
    notify_downstream_advance_grant_if_safe(e, visited);
    free(visited);

    // Send DNET to the node's upstreams if needed
    for (int i = 0; i < e->num_all_upstreams; i++) {
        int target_upstream_id = e->all_upstreams[i];
        if (target_upstream_id == e->id) {
            // FIXME: This shouldn't be entered, but currently, it's entered.
            continue;
        }
        send_downstream_next_event_tag_if_needed(rti_common->scheduling_nodes[target_upstream_id], next_event_tag);
    }
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
    for (int i = 0; i < intermediate->num_immediate_upstreams; i++) {
        // Add connection delay to path delay so far. Because tag addition is not commutative,
        // the calculation order should be carefully handled. Specifically, we should calculate
        // intermediate->upstream_delay[i] + delay_from_intermediate_so_far,
        // NOT delay_from_intermediate_so_far + intermediate->upstream_delay[i].
        // Before calculating path delay, convert intermediate->upstream_delay[i] to a tag
        // cause there is no function that adds a tag to an interval.
        tag_t connection_delay = lf_delay_tag(ZERO_TAG, intermediate->immediate_upstream_delays[i]);
        tag_t path_delay = lf_tag_add(connection_delay, delay_from_intermediate_so_far);
        // If the path delay is less than the so-far recorded path delay from upstream, update upstream.
        if (lf_tag_compare(path_delay, path_delays[intermediate->immediate_upstreams[i]]) < 0) {
            if (path_delays[intermediate->immediate_upstreams[i]].time == FOREVER) {
                // Found a finite path.
                *count = *count + 1;
            }
            path_delays[intermediate->immediate_upstreams[i]] = path_delay;
            // Since the path delay to upstream has changed, recursively update those upstream of it.
            // Do not do this, however, if the upstream node is the end node because this means we have
            // completed a cycle.
            if (end->id != intermediate->immediate_upstreams[i]) {
                _update_min_delays_upstream(end, rti_common->scheduling_nodes[intermediate->immediate_upstreams[i]], path_delays, count);
            } else {
                // Found a cycle.
                end->flags = end->flags | IS_IN_CYCLE;
                // Is it a zero-delay cycle?
                if (lf_tag_compare(path_delay, ZERO_TAG) == 0 && intermediate->immediate_upstream_delays[i] < 0) {
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
    if (node->all_upstreams == NULL) {

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

        // Put the results onto the matrix.
        node->num_all_upstreams = count;
        node->all_upstreams = (uint16_t*)calloc(count, sizeof(uint16_t));
        LF_PRINT_DEBUG("++++ Node %hu is in ZDC: %d", node->id, is_in_zero_delay_cycle(node));
        int k = 0;
        for (int i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
            if (lf_tag_compare(path_delays[i], FOREVER_TAG) < 0) {
                // Node i is upstream.
                if (k >= count) {
                    lf_print_error_and_exit("Internal error! Count of upstream nodes %zu for node %d is wrong!", count, i);
                }
                rti_common->min_delays[node->id + i*rti_common->number_of_scheduling_nodes] = path_delays[i];
                node->all_upstreams[k++] = i;
                // N^2 debug statement could be a problem with large benchmarks.
                // LF_PRINT_DEBUG("++++    Node %hu is upstream with delay" PRINTF_TAG "\n", i, path_delays[i].time, path_delays[i].microstep);
            }
        }
    }
}

void update_all_downstreams(scheduling_node_t* node) {
    if (node->all_downstreams == NULL) {
        bool visited[rti_common->number_of_scheduling_nodes];
        for (int i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
            visited[i] = false;
        }

        uint16_t queue[rti_common->number_of_scheduling_nodes];
        int front = 0, rear = 0;

        visited[node->id] = true;
        queue[rear++] = node->id;

        size_t count = 0;
        while (front != rear) {
            int current_id = queue[front++];
            scheduling_node_t* current_node = rti_common->scheduling_nodes[current_id];
            for (uint16_t i = 0; i < current_node->num_immediate_downstreams; i++) {
                uint16_t downstream_id = current_node->immediate_downstreams[i];
                if (visited[downstream_id] == false) {
                    visited[downstream_id] = true;
                    queue[rear++] = downstream_id;
                    count++;
                }
            }
        }

        int k = 0;
        node->all_downstreams = (uint16_t*)calloc(count, sizeof(uint16_t));
        node->num_all_downstreams = count;
        for (uint16_t i = 0; i < rti_common->number_of_scheduling_nodes; i++) {
            if (visited[i] == true && i != node->id) {
                if (k >= count) {
                    lf_print_error_and_exit("Internal error! Count of downstream nodes %zu for node %d is wrong!", count, i);
                }
                node->all_downstreams[k++] = i;
            }
        }
    }
}

void send_downstream_next_event_tag_if_needed(scheduling_node_t* node, tag_t new_NET) {
    if (is_in_zero_delay_cycle(node)) {
        return;
    }
    tag_t DNET = FOREVER_TAG;
    if (lf_tag_compare(node->last_DNET, new_NET) >= 0) {
        DNET = new_NET;
    } else {
        for (int i = 0; i < node->num_all_downstreams; i++) {
            uint16_t target_downstream_id = node->all_downstreams[i];
            scheduling_node_t* target_dowstream = rti_common->scheduling_nodes[target_downstream_id];

            if (is_in_zero_delay_cycle(target_dowstream)) {
                // This node is an upstream of ZDC. Do not send DNET to this node.
                return;
            }

            int index = node->id * rti_common->number_of_scheduling_nodes + target_downstream_id;
            tag_t DNET_candidate = lf_tag_subtract(target_dowstream->next_event, rti_common->min_delays[index]);

            if (lf_tag_compare(DNET, DNET_candidate) > 0) {
                DNET = DNET_candidate;
            }
        }
        if (DNET.time < start_time) {
            // DNET is NEVER.
            DNET = NEVER_TAG;
        }
    }
    if (lf_tag_compare(node->last_DNET, DNET) != 0
    && lf_tag_compare(node->completed, DNET) < 0
    && lf_tag_compare(node->next_event, DNET) <= 0) {
        send_downstream_next_event_tag(node, DNET);
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
