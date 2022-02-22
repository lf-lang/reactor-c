/**
 * Global Earliest Deadline First (GEDF) non-preemptive scheduler for the
 * threaded runtime of the C target of Lingua Franca.
*/

#include "priority_driven_base.c"

void lf_sched_init(size_t number_of_workers, sched_params_t* params) {
    lf_sched_init_base(number_of_workers, params, get_reaction_index);
}

void lf_sched_free() { lf_sched_free_base(); }

reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    return lf_sched_get_ready_reaction_base(worker_number);
}

void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    lf_sched_done_with_reaction_base(worker_number, done_reaction);
}

void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    lf_sched_trigger_reaction_base(reaction, worker_number);
}
