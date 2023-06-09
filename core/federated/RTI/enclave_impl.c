#include "enclave.h"

// References to the enclave RTI.    
extern enclave_rti_t * _e_rti;

void notify_tag_advance_grant(enclave_t* e, tag_t tag) {
    if (e->state == NOT_CONNECTED
            || lf_tag_compare(tag, e->last_granted) <= 0
            || lf_tag_compare(tag, e->last_provisionally_granted) < 0
    ) {
        return;
    }
    if (_e_rti.tracing_enabled) {
        tracepoint_rti_to_federate(_e_rti.trace, send_TAG, e->id, &tag);
    }
    e->last_granted = tag;
    lf_cond_signal(&e->next_event_condition);
}


