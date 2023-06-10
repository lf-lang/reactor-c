#include "rti_local.h"
#include "rti_common.h"
#include "util.h"
#include "platform.h"

// Static global pointer to the RTI object
static rti_local_t * rti_local;

void initialize_local_rti(environment_t **envs, int num_envs) {
    rti_local = malloc(sizeof(rti_local_t));
    if (rti_local == NULL) lf_print_error_and_exit("Out of memory");

    initialize_rti_common(&rti_local->base);
    rti_local->base.number_of_reactor_nodes = num_envs;

    // Allocate memory for the enclave_info objects
    rti.base.reactor_nodes = (reactor_node_info_t**)calloc(num_envs, sizeof(reactor_node_info_t*));
    for (int i = 0; i < num_envs; i++) {
        enclave_info_t *enclave_info = (enclave_info_t *) malloc(sizeof(enclave_info_t));
        initialize_enclave_info(enclave_info, envs[i]);
        rti_local.base.reactor_nodes[i] = (reactor_node_info_t *) enclave_info;
    }

}

void initialize_enclave_info(enclave_info_t* enclave, environment_t * env) {
    initialize_reactor_node(&enclave->reactor);

    enclave->env = env;
    
    // Initialize the next event condition variable.
    lf_cond_init(&e->next_event_condition, &rti_local.base.mutex);
}

tag_advance_grant_t rti_next_event_tag(enclave_info_t* e, tag_t next_event_tag) {
    // TODO: To support federated scheduling enclaves we must here potentially
    //  make calls to federate.c in order to send messages to the remote_rti
    tag_advance_grant_t result;
    // Early exit if we only have a single enclave. 
    // FIXME: Should we do some macro implementation of this function in that case?
    if (rti_local.base.number_of_reactor_nodes == 1) {
        result.tag = next_event_tag;
        result.is_provisional = false;
        return result;
    }

    // First, update the enclave data structure to record this next_event_tag,
    // and notify any downstream reactor_nodes, and unblock them if appropriate.
    lf_mutex_lock(&rti_local.base.mutex);

    // FIXME: If last_granted is already greater than next_event_tag, return next_event_tag.

    tag_t previous_tag = e->base.last_granted;
    tag_t previous_ptag = e->base.last_provisionally_granted;

    update_reactor_node_next_event_tag_locked(&e->base, next_event_tag);

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
    
    // Early exit if we only have a single enclave
    // FIXME: Do some macro implementation of the function in that case.
    if (rti_local.base.number_of_reactor_nodes == 1) {
        result.tag = next_event_tag;
        result.is_provisional = false;
        return result;
    }

    logical_tag_complete(&e->base, completed)
}

void rti_request_stop(tag_t stop_tag) {
    lf_assert(false, "Not implemented yet");
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
    // FIXME: Only signal the cond var if the enclave is in fact waiting
    lf_cond_signal(&((enclave_info_t *)e)->next_event_condition);
}
