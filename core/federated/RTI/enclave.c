#include "enclave.h"

// FIXME: This should not be here.
#include "rti_lib.h"

extern RTI_instance_t _RTI;

// Global variables defined in tag.c:
extern instant_t start_time;

// FIXME: rename "federate" everywhere in this file.

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
        // FIXME: Shouldn't use federate_t here.
        federate_t* downstream = &_RTI.federates[enclave->downstream[i]];
        // FIXME: Does this need to have two implementations?:
        send_advance_grant_if_safe(downstream);
        bool* visited = (bool*)calloc(_RTI.number_of_federates, sizeof(bool)); // Initializes to 0.
        // FIXME: Does this need to have two implementations?:
        send_downstream_advance_grants_if_safe(downstream, visited);
        free(visited);
    }

    lf_mutex_unlock(&rti_mutex);
}
