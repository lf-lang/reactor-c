/**
 * @file
 * @author Erling Jellum (erling.r.jellum@ntnu.no)
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Chadlia Jerad (chadlia.jerad@ensi-uma.tn)
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
*/

#ifdef LF_ENCLAVES
#include "rti_local.h"
#include "rti_common.h"
#include "util.h"
#include "platform.h"
#include "environment.h"
#include "trace.h"
#include "reactor.h"


// The following are macros for to be used in the enter and exit
// of the local RTI API. We extract this to macros to make sure that 
// mutexes are acquired and released in the correct order.
#define LTC_LOCKED_PROLOGUE(enclave) \
    do { \
        lf_mutex_unlock(&enclave->env->mutex); \
    } while(0)

#define LTC_LOCKED_EPILOGUE(enclave) \
    do { \
        lf_mutex_lock(&enclave->env->mutex); \
    } while(0)

#define NET_LOCKED_PROLOGUE(enclave) \
    do { \
        lf_mutex_unlock(&enclave->env->mutex); \
        lf_mutex_lock(rti_local->base.mutex); \
    } while(0)

#define NET_LOCKED_EPILOGUE(enclave) \
    do { \
        lf_mutex_unlock(rti_local->base.mutex); \
        lf_mutex_lock(&enclave->env->mutex); \
    } while(0)
#define UPDATE_OTHER_PROLOGUE() \
    do { \
        lf_mutex_lock(&rti_mutex); \
    } while(0)

#define UPDATE_OTHER_EPILOGUE() \
    do { \
        lf_mutex_unlock(&rti_mutex); \
    } while(0)


// Static global pointer to the RTI object.
static rti_local_t * rti_local;


lf_mutex_t rti_mutex;

void initialize_local_rti(environment_t *envs, int num_envs) {
    rti_local = malloc(sizeof(rti_local_t));
    if (rti_local == NULL) lf_print_error_and_exit("Out of memory");

    initialize_rti_common(&rti_local->base);
    lf_mutex_init(&rti_mutex);
    rti_local->base.mutex = &rti_mutex;
    rti_local->base.number_of_scheduling_nodes = num_envs;
    rti_local->base.tracing_enabled = (envs[0].trace != NULL);

    // Allocate memory for the enclave_info objects
    rti_local->base.scheduling_nodes = (scheduling_node_t**)calloc(num_envs, sizeof(scheduling_node_t*));
    for (int i = 0; i < num_envs; i++) {
        enclave_info_t *enclave_info = (enclave_info_t *) malloc(sizeof(enclave_info_t));
        initialize_enclave_info(enclave_info, i, &envs[i]);
        rti_local->base.scheduling_nodes[i] = (scheduling_node_t *) enclave_info;

        // Encode the connection topology into the enclave_info object.
        enclave_info->base.num_downstream = _lf_get_downstream_of(i, &enclave_info->base.downstream);
        enclave_info->base.num_upstream = _lf_get_upstream_of(i, &enclave_info->base.upstream);
        _lf_get_upstream_delay_of(i, &enclave_info->base.upstream_delay);

        enclave_info->base.state = GRANTED;
    }
}

void initialize_enclave_info(enclave_info_t* enclave, int idx, environment_t * env) {
    initialize_scheduling_node(&enclave->base, idx);

    env->enclave_info = enclave;
    enclave->env = env;
    
    // Initialize the next event condition variable.
    lf_cond_init(&enclave->next_event_condition, &rti_mutex);
}

tag_t rti_next_event_tag_locked(enclave_info_t* e, tag_t next_event_tag) {
    LF_PRINT_LOG("RTI: enclave %u sends NET of " PRINTF_TAG " ",
    e->base.id, next_event_tag.time - lf_time_start(), next_event_tag.microstep);
    
    // Return early if there are only a single enclave in the program.
    if (rti_local->base.number_of_scheduling_nodes == 1) {
        return next_event_tag;
    }

    NET_LOCKED_PROLOGUE(e);
    tracepoint_federate_to_rti(e->env->trace, send_NET, e->base.id, &next_event_tag);
    // First, update the enclave data structure to record this next_event_tag,
    // and notify any downstream scheduling_nodes, and unblock them if appropriate.
    tag_advance_grant_t result;

    tag_t previous_tag = e->base.last_granted;
    tag_t previous_ptag = e->base.last_provisionally_granted;

    update_scheduling_node_next_event_tag_locked(&e->base, next_event_tag);
    
    // Return early if we already have been granted past the NET.
    if (lf_tag_compare(e->base.last_granted, next_event_tag) >= 0) {
        LF_PRINT_LOG("RTI: enclave %u has already been granted a TAG to" PRINTF_TAG ". Returning with a TAG to" PRINTF_TAG " ",
        e->base.id, e->base.last_granted.time - lf_time_start(), e->base.last_granted.microstep,
        next_event_tag.time - lf_time_start(), next_event_tag.microstep);
        tracepoint_federate_from_rti(e->env->trace, receive_TAG, e->base.id, &next_event_tag);
        NET_LOCKED_EPILOGUE(e);
        return next_event_tag;
    }
    
    // If this enclave has no upstream, then we give a TAG till forever straight away.
    if (e->base.num_upstream == 0) {
        LF_PRINT_LOG("RTI: enclave %u has no upstream. Giving it a to the NET", e->base.id);
        e->base.last_granted = next_event_tag;
    }

    while(true) {
        // Determine whether the above call notified a TAG.
        // If so, return that value. Note that we dont care about PTAGs as we
        // have disallowed zero-delay enclave loops.
        if (lf_tag_compare(previous_tag, e->base.last_granted) < 0) {
            result.tag = e->base.last_granted;
            result.is_provisional = false;
            break;
        }
        // If not, block.
        LF_PRINT_LOG("RTI: enclave %u sleeps waiting for TAG to" PRINTF_TAG " ",
        e->base.id, e->base.next_event.time - lf_time_start(), e->base.next_event.microstep);
        lf_cond_wait(&e->next_event_condition);
    }

    // At this point we have gotten a new TAG.
    LF_PRINT_LOG("RTI: enclave %u returns with TAG to" PRINTF_TAG " ",
        e->base.id, e->base.next_event.time - lf_time_start(), e->base.next_event.microstep);
    tracepoint_federate_from_rti(e->env->trace, receive_TAG, e->base.id, &result.tag);
    NET_LOCKED_EPILOGUE(e);
    return result.tag;
}

void rti_logical_tag_complete_locked(enclave_info_t* enclave, tag_t completed) {
    if (rti_local->base.number_of_scheduling_nodes == 1) {
        return;
    }
    
    LTC_LOCKED_PROLOGUE(enclave);
    tracepoint_federate_to_rti(enclave->env->trace, send_LTC, enclave->base.id, &completed);
    _logical_tag_complete(&enclave->base, completed);
    LTC_LOCKED_EPILOGUE(enclave);
}

void rti_update_other_net_locked(enclave_info_t* src, enclave_info_t * target, tag_t net) {
    UPDATE_OTHER_PROLOGUE();
    tracepoint_federate_to_federate(src->env->trace, send_TAGGED_MSG, src->base.id, target->base.id, &net);

    // If our proposed NET is less than the current NET, update it.
    if (lf_tag_compare(net, target->base.next_event) < 0) {
        target->base.next_event = net;
    }

    UPDATE_OTHER_EPILOGUE();
}

///////////////////////////////////////////////////////////////////////////////
// The local RTIs implementation of the notify functions
///////////////////////////////////////////////////////////////////////////////

void notify_tag_advance_grant(scheduling_node_t* e, tag_t tag) {
    if (e->state == NOT_CONNECTED
            || lf_tag_compare(tag, e->last_granted) <= 0
            || lf_tag_compare(tag, e->last_provisionally_granted) < 0
    ) {
        return;
    }
    if (rti_local->base.tracing_enabled) {
        tracepoint_rti_to_federate(e->env->trace, send_TAG, e->id, &tag);
    }
    e->last_granted = tag;
    // TODO: Here we can consider adding a flag to the RTI struct and only signal the cond var if we have
    // sleeping enclaves.
    lf_cond_signal(&((enclave_info_t *)e)->next_event_condition);
}

// We currently ignore the PTAGs, because they are only relevant with zero
// delay enclave loops.
void notify_provisional_tag_advance_grant(scheduling_node_t* e, tag_t tag) {
    LF_PRINT_LOG("RTI: enclave %u callback with PTAG " PRINTF_TAG " ",
        e->id, tag.time - lf_time_start(), tag.microstep);
}
#endif //LF_ENCLAVES
