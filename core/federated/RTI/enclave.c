#include "enclave.h"

/**
 * Reference to enclave_rti_t instance.
 */
enclave_rti_t* _e_rti;

// Global variables defined in tag.c:
extern instant_t start_time;

// RTI mutex, which is the main lock  
extern lf_mutex_t rti_mutex;

// FIXME: For log and debug message in this file, what sould be kept: 'enclave', 
//        'federate', or 'enlcave/federate'? Currently its is 'enclave/federate'.
// FIXME: Should enclaves tracing use the same mechanism as federates? 
//        It needs to account a federate having itself a number of enclaves.
//        Currently, all calls to tracepoint_from_federate() and 
//        tracepoint_to_federate() are in rti_lib.c

void initialize_enclave(enclave_t* e, uint16_t id) {
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

    // Initialize the next event condition variable.
    lf_cond_init(&e->next_event_condition, &rti_mutex);
}

void logical_tag_complete(enclave_t* enclave, tag_t completed) {
    // FIXME: Consolidate this message with NET to get NMR (Next Message Request).
    // Careful with handling startup and shutdown.
    lf_mutex_lock(&rti_mutex);

    enclave->completed = completed;

    LF_PRINT_LOG("RTI received from federate/enclave %d the Logical Tag Complete (LTC) " PRINTF_TAG ".",
                enclave->id, enclave->completed.time - start_time, enclave->completed.microstep);

    // Check downstream enclaves to see whether they should now be granted a TAG.
    for (int i = 0; i < enclave->num_downstream; i++) {
        enclave_t *downstream = _e_rti->enclaves[enclave->downstream[i]];
        // Notify downstream enclave if appropriate.
        notify_advance_grant_if_safe(downstream);
        bool *visited = (bool *)calloc(_e_rti->number_of_enclaves, sizeof(bool)); // Initializes to 0.
        // Notify enclaves downstream of downstream if appropriate.
        notify_downstream_advance_grant_if_safe(downstream, visited);
        free(visited);
    }

    lf_mutex_unlock(&rti_mutex);
}

tag_advance_grant_t tag_advance_grant_if_safe(enclave_t* e) {
    tag_advance_grant_t result = {.tag = NEVER_TAG, .is_provisional = false};

    // Find the earliest LTC of upstream enclaves (M).
    tag_t min_upstream_completed = FOREVER_TAG;

    for (int j = 0; j < e->num_upstream; j++) {
        enclave_t *upstream = _e_rti->enclaves[e->upstream[j]];

        // Ignore this enclave if it no longer connected.
        if (upstream->state == NOT_CONNECTED) continue;

        // Adjust by the "after" delay.
        // Note that "no delay" is encoded as NEVER,
        // whereas one microstep delay is encoded as 0LL.
        tag_t candidate = lf_delay_strict(upstream->completed, e->upstream_delay[j]);

        if (lf_tag_compare(candidate, min_upstream_completed) < 0) {
            min_upstream_completed = candidate;
        }
    }
    LF_PRINT_LOG("Minimum upstream LTC for federate/enclave %d is " PRINTF_TAG 
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
    // If all (transitive) upstream enclaves of the enclave
    // have earliest event tags such that the
    // enclave can now advance its tag, then send it a TAG message.
    // Find the earliest event time of each such upstream enclave,
    // adjusted by delays on the connections.

    // To handle cycles, need to create a boolean array to keep
    // track of which upstream enclave have been visited.
    bool *visited = (bool *)calloc(_e_rti->number_of_enclaves, sizeof(bool)); // Initializes to 0.

    // Find the tag of the earliest possible incoming message from
    // upstream enclaves.
    tag_t t_d_nonzero_delay = FOREVER_TAG;
    // The tag of the earliest possible incoming message from a zero-delay connection.
    // Delayed connections are not guarded from STP violations by the MLAA; this property is
    // acceptable because delayed connections impose no deadlock risk and in some cases (startup)
    // this property is necessary to avoid deadlocks. However, it requires some special care here
    // when potentially sending a PTAG because we must not send a PTAG for a tag at which data may
    // still be received over nonzero-delay connections.
    tag_t t_d_zero_delay = FOREVER_TAG;
    LF_PRINT_DEBUG("NOTE: FOREVER is displayed as " PRINTF_TAG " and NEVER as " PRINTF_TAG,
                   FOREVER_TAG.time - start_time, FOREVER_TAG.microstep,
                   NEVER_TAG.time - start_time, 0);

    for (int j = 0; j < e->num_upstream; j++) {
        enclave_t *upstream = _e_rti->enclaves[e->upstream[j]];

        // Ignore this enclave if it is no longer connected.
        if (upstream->state == NOT_CONNECTED) continue;

        // Find the (transitive) next event tag upstream.
        tag_t upstream_next_event = transitive_next_event(
                upstream, upstream->next_event, visited);

        LF_PRINT_DEBUG("Earliest next event upstream of fed/encl %d at fed/encl %d has tag " PRINTF_TAG ".",
                e->id,
                upstream->id,
                upstream_next_event.time - start_time, upstream_next_event.microstep);

        // Adjust by the "after" delay.
        // Note that "no delay" is encoded as NEVER,
        // whereas one microstep delay is encoded as 0LL.
        tag_t candidate = lf_delay_strict(upstream_next_event, e->upstream_delay[j]);

        if (e->upstream_delay[j] == NEVER) {
            if (lf_tag_compare(candidate, t_d_zero_delay) < 0) {
                t_d_zero_delay = candidate;
            }
        } else {
            if (lf_tag_compare(candidate, t_d_nonzero_delay) < 0) {
                t_d_nonzero_delay = candidate;
            }
        }
    }
    free(visited);
    tag_t t_d = (lf_tag_compare(t_d_zero_delay, t_d_nonzero_delay) < 0) ? t_d_zero_delay : t_d_nonzero_delay;

    LF_PRINT_LOG("Earliest next event upstream has tag " PRINTF_TAG ".",
            t_d.time - start_time, t_d.microstep);

    if (
        lf_tag_compare(t_d, e->next_event) > 0       // The enclave has something to do.
        && lf_tag_compare(t_d, e->last_provisionally_granted) >= 0  // The grant is not redundant
                                                                      // (equal is important to override any previous
                                                                      // PTAGs).
        && lf_tag_compare(t_d, e->last_granted) > 0  // The grant is not redundant.
    ) {
        // All upstream enclaves have events with a larger tag than fed, so it is safe to send a TAG.
        LF_PRINT_LOG("Earliest upstream message time for fed/encl %d is " PRINTF_TAG
                "(adjusted by after delay). Granting tag advance for " PRINTF_TAG,
                e->id,
                t_d.time - lf_time_start(), t_d.microstep,
                e->next_event.time - lf_time_start(),
                e->next_event.microstep);
        result.tag = e->next_event;
    } else if (
        lf_tag_compare(t_d_zero_delay, e->next_event) == 0      // The enclave has something to do.
        && lf_tag_compare(t_d_zero_delay, t_d_nonzero_delay) < 0  // The statuses of nonzero-delay connections are known at tag t_d_zero_delay
        && lf_tag_compare(t_d_zero_delay, e->last_provisionally_granted) > 0  // The grant is not redundant.
        && lf_tag_compare(t_d_zero_delay, e->last_granted) > 0  // The grant is not redundant.
    ) {
        // Some upstream enclaves has an event that has the same tag as fed's next event, so we can only provisionally
        // grant a TAG (via a PTAG).
        LF_PRINT_LOG("Earliest upstream message time for fed/encl %d is " PRINTF_TAG
            " (adjusted by after delay). Granting provisional tag advance.",
            e->id,
            t_d_zero_delay.time - start_time, t_d_zero_delay.microstep);
        result.tag = t_d_zero_delay;
        result.is_provisional = true;
    }
    return result;
}

void notify_downstream_advance_grant_if_safe(enclave_t* e, bool visited[]) {
    visited[e->id] = true;
    for (int i = 0; i < e->num_downstream; i++) {
        enclave_t* downstream = _e_rti->enclaves[e->downstream[i]];
        if (visited[downstream->id]) continue;
        notify_advance_grant_if_safe(downstream);
        notify_downstream_advance_grant_if_safe(downstream, visited);
    }
}

void update_enclave_next_event_tag_locked(enclave_t* e, tag_t next_event_tag) {
    e->next_event = next_event_tag;

    LF_PRINT_DEBUG(
       "RTI: Updated the recorded next event tag for federate/enclave %d to " PRINTF_TAG,
       e->id,
       next_event_tag.time - lf_time_start(),
       next_event_tag.microstep
    );

    // Check to see whether we can reply now with a tag advance grant.
    // If the enclave has no upstream enclaves, then it does not wait for
    // nor expect a reply. It just proceeds to advance time.
    if (e->num_upstream > 0) {
        notify_advance_grant_if_safe(e);
    }
    // Check downstream enclaves to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which upstream enclaves have been visited.
    bool *visited = (bool *)calloc(_e_rti->number_of_enclaves, sizeof(bool)); // Initializes to 0.
    notify_downstream_advance_grant_if_safe(e, visited);
    free(visited);
}

tag_advance_grant_t next_event_tag(enclave_t* e, tag_t next_event_tag) {
    tag_advance_grant_t result;

    // First, update the enclave data structure to record this next_event_tag,
    // and notify any downstream enclaves, and unblock them if appropriate.
    lf_mutex_lock(&rti_mutex);

    // FIXME: If last_granted is already greater than next_event_tag, return next_event_tag.

    tag_t previous_tag = e->last_granted;
    tag_t previous_ptag = e->last_provisionally_granted;

    update_enclave_next_event_tag_locked(e, next_event_tag);

    while(true) {
        // Determine whether the above call notified e of a TAG or PTAG.
        // If so, return that value.
        if (lf_tag_compare(previous_tag, e->last_granted) < 0) {
            result.tag = e->last_granted;
            result.is_provisional = false;
            lf_mutex_unlock(&rti_mutex);
            return result;
        }
        if (lf_tag_compare(previous_ptag, e->last_provisionally_granted) < 0) {
            result.tag = e->last_provisionally_granted;
            result.is_provisional = true;
            lf_mutex_unlock(&rti_mutex);
            return result;
        }

        // If not, block.
        lf_cond_wait(&e->next_event_condition);
    }
}

void notify_advance_grant_if_safe(enclave_t* e) {
    tag_advance_grant_t grant = tag_advance_grant_if_safe(e);
    if (lf_tag_compare(grant.tag, NEVER_TAG) != 0) {
        if (grant.is_provisional) {
            notify_provisional_tag_advance_grant(e, grant.tag);
        } else {
            notify_tag_advance_grant(e, grant.tag);
        }
    }
}

tag_t transitive_next_event(enclave_t* e, tag_t candidate, bool visited[]) {
    if (visited[e->id] || e->state == NOT_CONNECTED) {
        // Enclave has stopped executing or we have visited it before.
        // No point in checking upstream enclaves.
        return candidate;
    }

    visited[e->id] = true;
    tag_t result = e->next_event;

    // If the candidate is less than this enclave's next_event, use the candidate.
    if (lf_tag_compare(candidate, result) < 0) {
        result = candidate;
    }

    // The result cannot be earlier than the start time.
    if (result.time < start_time) {
        // Earliest next event cannot be before the start time.
        result = (tag_t){.time = start_time, .microstep = 0u};
    }

    // Check upstream enclaves to see whether any of them might send
    // an event that would result in an earlier next event.
    for (int i = 0; i < e->num_upstream; i++) {
        tag_t upstream_result = transitive_next_event(
            _e_rti->enclaves[e->upstream[i]], result, visited);

        // Add the "after" delay of the connection to the result.
        upstream_result = lf_delay_tag(upstream_result, e->upstream_delay[i]);

        // If the adjusted event time is less than the result so far, update the result.
        if (lf_tag_compare(upstream_result, result) < 0) {
            result = upstream_result;
        }
    }
    if (lf_tag_compare(result, e->completed) < 0) {
        result = e->completed;
    }
    return result;
}
