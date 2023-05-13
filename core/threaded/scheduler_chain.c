#include "scheduler_chain.h"

void lf_sched_chain_add_or_schedule(_lf_sched_chain_t *chain, reaction_t *upstream, reaction_t *downstream) {

    // Figure out if
    // 1) We should schedule the downstream we got now
    // 2) We should schedule a downstream stored in the `proposed_next` field of the chain struct
    bool schedule_now = false;
    bool schedule_proposed_now = false;

    if (chain->is_candidate) {
        if (chain->proposed_next == NULL ) {
            if (downstream->last_enabling_reaction == upstream) {
                LF_PRINT_DEBUG("Worker %u: Proposing reaction %s for a chain.", worker_number, downstream->name);
                _lf_sched_instance->_lf_sched_chain[worker_number].proposed_next = downstream;
            } else {
                schedule_now = true;
            }
        } else {
            schedule_now = true;
            schedule_proposed_now = true;
        }
    } else {
        schedule_now = true;
    }

    if (schedule_proposed_now) {
        _lf_sched_(chain->proposed_next);
        chain->is_candidate = false;
        chain->proposed_next = NULL;
    }

    if (schedule_now) {
        _lf_sched_insert_reaction(downstream);
        chain->is_candidate = false;
    }
}
reaction_t * lf_sched_chain_get_next(_lf_sched_chain_t *chain);