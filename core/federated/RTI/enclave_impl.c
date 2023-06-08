#include "enclave.h"

// References to the enclave RTI.    
extern enclave_RTI_t * _E_RTI;

void notify_tag_advance_grant(enclave_t* e, tag_t tag) {
    if (e->state == NOT_CONNECTED
            || lf_tag_compare(tag, e->last_granted) <= 0
            || lf_tag_compare(tag, e->last_provisionally_granted) < 0
    ) {
        return;
    }
    if (_E_RTI->tracing_enabled) {
        tracepoint_RTI_to_federate(send_TAG, e->id, &tag);
    }
    e->last_granted = tag;
    lf_cond_signal(&e->next_event_condition);
}


