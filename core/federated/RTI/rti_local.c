#include "rti_local.h"
#include "rti_common.h"
#include "util.h"

static rti_local_t * rti_local;

void initialize_local_rti() {
    rti_local = malloc(sizeof(rti_local_t));
    if (rti_local == NULL) lf_print_error_and_exit("Out of memory");

    initialize_rti_common(&rti_local->base);
}

void initialize_enclave_info(enclave_info_t* enclave) {
    initialize_reactor_node(&enclave->reactor);
}

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
    lf_cond_signal(&e->next_event_condition);
}



tag_advance_grant_t rti_next_event_tag(enclave_info_t* e, tag_t next_event_tag) {
    // TODO: To support federated scheduling enclaves we must here potentially
    //  make calls to federate.c in order to send messages to the remote_rti
    return next_event_tag(&e->base, next_event_tag)
}

void rti_logical_tag_complete(enclave_info_t* enclave, tag_t completed) {
    // TODO: To support federated scheduling enclaves we must here potentially
    //  make calls to federate.c in order to send messages to the remote_rti
    logical_tag_complete(&e->base, completed)
}
