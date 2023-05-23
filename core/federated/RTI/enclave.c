#include "enclave.h"

extern enclave_RTI_t _RTI;

// Global variables defined in tag.c:
extern instant_t start_time;

// RTI mutex, which is the main lock  
extern lf_mutex_t rti_mutex;

// FIXME: rename "federate" everywhere in this file.

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
    e->requested_stop = false;

    // Initialize the next event condition variable.
    lf_cond_init(&e->next_event_condition, &rti_mutex);
}

void logical_tag_complete(enclave_t* enclave, tag_t completed) {
    // FIXME: Consolidate this message with NET to get NMR (Next Message Request).
    // Careful with handling startup and shutdown.
    lf_mutex_lock(&rti_mutex);

    enclave->completed = completed;
    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_LTC, enclave->id, &(enclave->completed));
    }

    LF_PRINT_LOG("RTI received from federate %d the Logical Tag Complete (LTC) (%lld, %u).",
                enclave->id, enclave->completed.time - start_time, enclave->completed.microstep);

    // See if we can remove any of the recorded in-transit messages for this.
    // FIXME: Should this be here?
    // clean_in_transit_message_record_up_to_tag(enclave->in_transit_message_tags, enclave->completed);

    // Check downstream federates to see whether they should now be granted a TAG.
    for (int i = 0; i < enclave->num_downstream; i++) {
        // FIXME: Shouldn't use enclave_t here.
        enclave_t* downstream = _RTI.enclaves[enclave->downstream[i]];
        // Notify downstream federate if appropriate.
        notify_advance_grant_if_safe((enclave_t*)downstream);
        bool* visited = (bool*)calloc(_RTI.number_of_enclaves, sizeof(bool)); // Initializes to 0.
        // Notify enclaves downstream of downstream if appropriate.
        notify_downstream_advance_grant_if_safe((enclave_t*)downstream, visited);
        free(visited);
    }

    lf_mutex_unlock(&rti_mutex);
}

tag_advance_grant_t tag_advance_grant_if_safe(enclave_t* e) {
    tag_advance_grant_t result = {.tag = NEVER_TAG, .is_provisional = false};

    // Find the earliest LTC of upstream federates (M).
    tag_t min_upstream_completed = FOREVER_TAG;

    for (int j = 0; j < e->num_upstream; j++) {
        enclave_t* upstream = _RTI.enclaves[e->upstream[j]];

        // Ignore this federate if it has resigned.
        if (upstream->state == NOT_CONNECTED) continue;

        tag_t candidate = lf_delay_tag(upstream->completed, e->upstream_delay[j]);

        if (lf_tag_compare(candidate, min_upstream_completed) < 0) {
            min_upstream_completed = candidate;
        }
    }
    LF_PRINT_LOG("Minimum upstream LTC for fed %d is (%lld, %u) "
            "(adjusted by after delay).",
            e->id,
            min_upstream_completed.time - start_time, min_upstream_completed.microstep);
    if (lf_tag_compare(min_upstream_completed, e->last_granted) > 0
        && lf_tag_compare(min_upstream_completed, e->next_event) >= 0 // The federate has to advance its tag
    ) {
        result.tag = min_upstream_completed;
        return result;
    }

    // Can't make progress based only on upstream LTCs.
    // If all (transitive) upstream federates of the federate
    // have earliest event tags such that the
    // federate can now advance its tag, then send it a TAG message.
    // Find the earliest event time of each such upstream federate,
    // adjusted by delays on the connections.

    // To handle cycles, need to create a boolean array to keep
    // track of which upstream federates have been visited.
    bool* visited = (bool*)calloc(_RTI.number_of_enclaves, sizeof(bool)); // Initializes to 0.

    // Find the tag of the earliest possible incoming message from
    // upstream federates.
    tag_t t_d = FOREVER_TAG;
    LF_PRINT_DEBUG("NOTE: FOREVER is displayed as (%lld, %u) and NEVER as (%lld, %u)",
            FOREVER_TAG.time - start_time, FOREVER_TAG.microstep,
            NEVER - start_time, 0u);

    for (int j = 0; j < e->num_upstream; j++) {
        enclave_t* upstream = _RTI.enclaves[e->upstream[j]];

        // Ignore this federate if it has resigned.
        if (upstream->state == NOT_CONNECTED) continue;

        // Find the (transitive) next event tag upstream.
        tag_t upstream_next_event = transitive_next_event(
                upstream, upstream->next_event, visited);

        LF_PRINT_DEBUG("Earliest next event upstream of fed %d at fed %d has tag (%lld, %u).",
                e->id,
                upstream->id,
                upstream_next_event.time - start_time, upstream_next_event.microstep);

        // Adjust by the "after" delay.
        // Note that "no delay" is encoded as NEVER,
        // whereas one microstep delay is encoded as 0LL.
        tag_t candidate = lf_delay_tag(upstream_next_event, e->upstream_delay[j]);

        if (lf_tag_compare(candidate, t_d) < 0) {
            t_d = candidate;
        }
    }
    free(visited);

    LF_PRINT_LOG("Earliest next event upstream has tag (%lld, %u).",
            t_d.time - start_time, t_d.microstep);

    if (
        lf_tag_compare(t_d, e->next_event) > 0       // The federate has something to do.
        && lf_tag_compare(t_d, e->last_provisionally_granted) >= 0  // The grant is not redundant
                                                                      // (equal is important to override any previous
                                                                      // PTAGs).
        && lf_tag_compare(t_d, e->last_granted) > 0  // The grant is not redundant.
    ) {
        // All upstream federates have events with a larger tag than fed, so it is safe to send a TAG.
        LF_PRINT_LOG("Earliest upstream message time for fed %d is " PRINTF_TAG
                "(adjusted by after delay). Granting tag advance for " PRINTF_TAG,
                e->id,
                t_d.time - lf_time_start(), t_d.microstep,
                e->next_event.time - lf_time_start(),
                e->next_event.microstep);
        result.tag = e->next_event;
    } else if (
        lf_tag_compare(t_d, e->next_event) == 0      // The federate has something to do.
        && lf_tag_compare(t_d, e->last_provisionally_granted) > 0  // The grant is not redundant.
        && lf_tag_compare(t_d, e->last_granted) > 0  // The grant is not redundant.
    ) {
        // Some upstream federate has an event that has the same tag as fed's next event, so we can only provisionally
        // grant a TAG (via a PTAG).
        LF_PRINT_LOG("Earliest upstream message time for fed %d is " PRINTF_TAG
            " (adjusted by after delay). Granting provisional tag advance.",
            e->id,
            t_d.time - start_time, t_d.microstep);
        result.tag = t_d;
        result.is_provisional = true;
    }
    return result;
}

void notify_downstream_advance_grant_if_safe(enclave_t* e, bool visited[]) {
    visited[e->id] = true;
    for (int i = 0; i < e->num_downstream; i++) {
        enclave_t* downstream = (enclave_t*)_RTI.enclaves[e->downstream[i]];
        if (visited[downstream->id]) continue;
        notify_advance_grant_if_safe(downstream);
        notify_downstream_advance_grant_if_safe(downstream, visited);
    }
}

void update_enclave_next_event_tag_locked(enclave_t* e, tag_t next_event_tag) {
    e->next_event = next_event_tag;

    LF_PRINT_DEBUG(
       "RTI: Updated the recorded next event tag for enclave %d to " PRINTF_TAG,
       e->id,
       next_event_tag.time - lf_time_start(),
       next_event_tag.microstep
    );

    // Check to see whether we can reply now with a tag advance grant.
    // If the federate has no upstream federates, then it does not wait for
    // nor expect a reply. It just proceeds to advance time.
    if (e->num_upstream > 0) {
        notify_advance_grant_if_safe(e);
    }
    // Check downstream federates to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which upstream federates have been visited.
    bool* visited = (bool*)calloc(_RTI.number_of_enclaves, sizeof(bool)); // Initializes to 0.
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
        // Federate has stopped executing or we have visited it before.
        // No point in checking upstream federates.
        return candidate;
    }

    visited[e->id] = true;
    tag_t result = e->next_event;

    // If the candidate is less than this federate's next_event, use the candidate.
    if (lf_tag_compare(candidate, result) < 0) {
        result = candidate;
    }

    // The result cannot be earlier than the start time.
    if (result.time < start_time) {
        // Earliest next event cannot be before the start time.
        result = (tag_t){.time = start_time, .microstep = 0u};
    }

    // Check upstream federates to see whether any of them might send
    // an event that would result in an earlier next event.
    for (int i = 0; i < e->num_upstream; i++) {
        tag_t upstream_result = transitive_next_event(
                _RTI.enclaves[e->upstream[i]], result, visited);

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
