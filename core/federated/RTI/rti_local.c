#include "rti_local.h"
#include "rti_common.h"
#include "util.h"
#include "platform.h"
#include "trace.h"

// Static global pointer to the RTI object
static rti_local_t * rti_local;

lf_mutex_t rti_mutex;

// FIXME: Replace with e.g. NUM_ENCLAVES
// FIXME: Actually, we probably dont want to use all this static memory for it.
extern int upstream_matrix[3][3];
extern int downstream_matrix[3][3];
extern interval_t connection_delay_matrix[3][3];

// This function takes an adjacency array and dynamically allocates a connection array
static int create_connection_array(int* adjacency, int num_enclaves, int** result) {
    int num_connected = 0;
    for (int i = 0; i<num_enclaves; i++) {
        if (adjacency[i]) {
            num_connected += 1;
        }
    }
    *result = calloc(num_connected, sizeof(int));
    int idx=0;
    for (int i = 0; i<num_enclaves; i++) {
        if (adjacency[i]) {
            (*result)[idx++] = i;
        }
    }
    return num_connected;
}

static void create_delay_array(interval_t* delays, int* connections, int num_connections, interval_t** result) {
    *result = calloc(num_connections, sizeof(instant_t));
    for (int i = 0; i< num_connections; i++) {
        (*result)[i] = delays[connections[i]];
    }
}

void initialize_local_rti(environment_t *envs, int num_envs) {
    rti_local = malloc(sizeof(rti_local_t));
    if (rti_local == NULL) lf_print_error_and_exit("Out of memory");

    initialize_rti_common(&rti_local->base);
    lf_mutex_init(&rti_mutex);
    rti_local->base.mutex = &rti_mutex;
    rti_local->base.number_of_reactor_nodes = num_envs;
    rti_local->base.trace = envs[0].trace;
    rti_local->base.tracing_enabled = (envs[0].trace != NULL);

    // Allocate memory for the enclave_info objects
    rti_local->base.reactor_nodes = (reactor_node_info_t**)calloc(num_envs, sizeof(reactor_node_info_t*));
    for (int i = 0; i < num_envs; i++) {
        enclave_info_t *enclave_info = (enclave_info_t *) malloc(sizeof(enclave_info_t));
        initialize_enclave_info(enclave_info, i, &envs[i]);
        rti_local->base.reactor_nodes[i] = (reactor_node_info_t *) enclave_info;

        // Encode the connection topology into the enclave_info object        
        enclave_info->base.num_downstream = create_connection_array(
            downstream_matrix[i], num_envs, &enclave_info->base.downstream);

        enclave_info->base.num_upstream = create_connection_array(
            upstream_matrix[i], num_envs, &enclave_info->base.upstream);

        create_delay_array(
            connection_delay_matrix[i], enclave_info->base.upstream, enclave_info->base.num_upstream, 
            &enclave_info->base.upstream_delay);

        // FIXME: Why is it always granted?
        enclave_info->base.state = GRANTED;
    }
}

void initialize_enclave_info(enclave_info_t* enclave, int idx, environment_t * env) {
    initialize_reactor_node(&enclave->base, idx);

    env->enclave_info = enclave;
    enclave->env = env;
    
    // Initialize the next event condition variable.
    lf_cond_init(&enclave->next_event_condition, &rti_mutex);
}

tag_t rti_next_event_tag(enclave_info_t* e, tag_t next_event_tag) {
    // FIXME: What other checks must we do here? See the federated
    LF_PRINT_LOG("RTI: enclave %u sends NET of " PRINTF_TAG " ",
    e->base.id, next_event_tag.time - lf_time_start(), next_event_tag.microstep);
    
    if (rti_local->base.number_of_reactor_nodes == 1) {
        return next_event_tag;
    }

    lf_mutex_unlock(&e->env->mutex);

    // TODO: To support federated scheduling enclaves we must here potentially
    //  make calls to federate.c in order to send messages to the remote_rti
    tag_advance_grant_t result;
    // Early exit if we only have a single enclave. 
    // FIXME: Should we do some macro implementation of this function in that case?

    // First, update the enclave data structure to record this next_event_tag,
    // and notify any downstream reactor_nodes, and unblock them if appropriate.
    lf_mutex_lock(rti_local->base.mutex);

    tag_t previous_tag = e->base.last_granted;
    tag_t previous_ptag = e->base.last_provisionally_granted;

    update_reactor_node_next_event_tag_locked(&e->base, next_event_tag);
    
    // Return early if we already have been granted past the NET
    if (lf_tag_compare(e->base.last_granted, next_event_tag) >= 0) {
        LF_PRINT_LOG("RTI: enclave %u has already been granted a TAG to" PRINTF_TAG ". Returning with a TAG to" PRINTF_TAG " ",
        e->base.id, e->base.last_granted.time - lf_time_start(), e->base.last_granted.microstep,
        next_event_tag.time - lf_time_start(), next_event_tag.microstep);
        lf_mutex_unlock(rti_local->base.mutex);
        lf_mutex_lock(&e->env->mutex);
        return next_event_tag;
    }
    
    // FIXME: This check is a little out-of-place here. But it is needed
    if (e->base.num_upstream == 0) {
        LF_PRINT_LOG("RTI: enclave %u has no upstream granting TAG to" PRINTF_TAG " ",
        e->base.id, e->base.next_event.time - lf_time_start(), e->base.next_event.microstep);
        e->base.last_granted = e->base.next_event;
    }

    while(true) {
        // Determine whether the above call notified e of a TAG or PTAG.
        // If so, return that value.
        if (lf_tag_compare(previous_tag, e->base.last_granted) < 0) {
            result.tag = e->base.last_granted;
            result.is_provisional = false;
            break;
        }
        // FIXME: Lets try not doing any PTAG stuff. I think it is possible in absense of ZDC
        if (lf_tag_compare(previous_ptag, e->base.last_provisionally_granted) < 0) {
            result.tag = e->base.last_provisionally_granted;
            result.is_provisional = true;
            break;
        }

        // If not, block.
    LF_PRINT_LOG("RTI: enclave %u sleeps waiting for TAG to" PRINTF_TAG " ",
        e->base.id, e->base.next_event.time - lf_time_start(), e->base.next_event.microstep);
        
        lf_cond_wait(&e->next_event_condition);
    }
    lf_mutex_unlock(&rti_mutex);
    lf_mutex_lock(&e->env->mutex);
    LF_PRINT_LOG("RTI: enclave %u returns with TAG to" PRINTF_TAG " ",
        e->base.id, e->base.next_event.time - lf_time_start(), e->base.next_event.microstep);
    return result.tag;
}

void rti_logical_tag_complete(enclave_info_t* enclave, tag_t completed) {
    // TODO: To support federated scheduling enclaves we must here potentially
    //  make calls to federate.c in order to send messages to the remote_rti
    
    // Early exit if we only have a single enclave
    // FIXME: Do some macro implementation of the function in that case.
    if (rti_local->base.number_of_reactor_nodes == 1) {
        return;
    }

    _logical_tag_complete(&enclave->base, completed);
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
        tracepoint_rti_to_federate(rti_local->base.trace, send_TAG, e->id, &tag);
    }
    e->last_granted = tag;
    // FIXME: Only signal the cond var if the enclave is in fact waiting
    lf_cond_signal(&((enclave_info_t *)e)->next_event_condition);
}
// FIXME: For now I just ignore the PTAGs, the hyptohesis is that it is redundant without ZDC
void notify_provisional_tag_advance_grant(reactor_node_info_t* e, tag_t tag) {
    
    LF_PRINT_LOG("RTI: enclave %u callback with PTAG " PRINTF_TAG " ",
        e->id, tag.time - lf_time_start(), tag.microstep);
    // e->last_granted = tag;
    // lf_cond_signal(&((enclave_info_t *)e)->next_event_condition);
}