#include "rti_local.h"
#include "rti_common.h"
#include "util.h"
#include "platform.h"

// Static global pointer to the RTI object
static rti_local_t * rti_local;

void initialize_local_rti() {
    rti_local = malloc(sizeof(rti_local_t));
    if (rti_local == NULL) lf_print_error_and_exit("Out of memory");

    initialize_rti_common(&rti_local->base);
}

void initialize_enclave_info(enclave_info_t* enclave) {
    initialize_reactor_node(&enclave->reactor);
    
    // Initialize the next event condition variable.
    lf_cond_init(&e->next_event_condition, &rti_local.base.mutex);
}

tag_advance_grant_t rti_next_event_tag(enclave_info_t* e, tag_t next_event_tag) {
    // TODO: To support federated scheduling enclaves we must here potentially
    //  make calls to federate.c in order to send messages to the remote_rti
    tag_advance_grant_t result;

    // First, update the enclave data structure to record this next_event_tag,
    // and notify any downstream reactor_nodes, and unblock them if appropriate.
    lf_mutex_lock(&rti_local.base.mutex);

    // FIXME: If last_granted is already greater than next_event_tag, return next_event_tag.

    tag_t previous_tag = e->base.last_granted;
    tag_t previous_ptag = e->base.last_provisionally_granted;

    update_enclave_next_event_tag_locked(&e->base, next_event_tag);

    while(true) {
        // Determine whether the above call notified e of a TAG or PTAG.
        // If so, return that value.
        if (lf_tag_compare(previous_tag, e->base.last_granted) < 0) {
            result.tag = e->base.last_granted;
            result.is_provisional = false;
            lf_mutex_unlock(&rti_local.base.mutex);
            return result;
        }
        if (lf_tag_compare(previous_ptag, e->base.last_provisionally_granted) < 0) {
            result.tag = e->base.last_provisionally_granted;
            result.is_provisional = true;
            lf_mutex_unlock(&rti_local.base.mutex);
            return result;
        }

        // If not, block.
        lf_cond_wait(&e->next_event_condition);
    }
}

void rti_logical_tag_complete(enclave_info_t* enclave, tag_t completed) {
    // TODO: To support federated scheduling enclaves we must here potentially
    //  make calls to federate.c in order to send messages to the remote_rti
    logical_tag_complete(&e->base, completed)
}

///////////////////////////////////////////////////////////////////////////////
// The local RTIs implementation of the notify functions
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief This function will send a TAG to the enclave `e`. It might be
 * waiting 
 * 
 * @param e 
 * @param tag 
 */
void notify_tag_advance_grant(reactor_node_info_t* e, tag_t tag) {
    if (e->state == NOT_CONNECTED
            || lf_tag_compare(tag, e->last_granted) <= 0
            || lf_tag_compare(tag, e->last_provisionally_granted) < 0
    ) {
        return;
    }
    if (rti_local->base.tracing_enabled) {
        tracepoint_RTI_to_federate(send_TAG, e->id, &tag);
    }
    e->last_granted = tag;
    lf_cond_signal(&((enclave_info_t *)e)->next_event_condition);
}
