/**
 * Least-slack-time (LST) non-preemptive scheduler for the
 * threaded runtime of the C target of Lingua Franca.
*/

#include "priority_driven_base.c"

#define HASHMAP(token) start_time_hashmap ## _ ## token
#define K reaction_t*
#define V interval_t
#define HASH_OF(key) (size_t) key
#include "../../utils/hashmap.c"
#undef HASHMAP
#undef K
#undef V
#undef HASH_OF
#define HASHMAP(token) execution_time_hashmap ## _ ## token
#define K reaction_t*
#define V interval_t
#define HASH_OF(key) (size_t) key
#include "../../utils/hashmap.c"
#undef HASHMAP
#undef K
#undef V
#undef HASH_OF

static start_time_hashmap_t* start_times;
static execution_time_hashmap_t* execution_times;

//////////////////////////// Private helpers ////////////////////////////////

static size_t compute_hashmap_size(sched_params_t* params) {
    size_t ret = 0;
    for (size_t i = 0; i < params->max_reactions_per_level_size; i++) {
        ret += params->max_reactions_per_level[i];
    }
    return ret << 2;  // If the hashmap approaches full, the O(1) approximation is wrong
}

static pqueue_pri_t cram_bits(index_t level, interval_t deadline, interval_t execution_time) {
    interval_t slack_time_plus_now = deadline - execution_time;
    pqueue_pri_t ret = level << 48;  // This opposes the convention used elsewhere.
    // It's fine not to subtract the current time. The relationship to slack time just needs to be
    // monotonic.
    ret += slack_time_plus_now;
    return ret;
}

static pqueue_pri_t get_lst_priority(void* reaction) {
    reaction_t* casted = (reaction_t*) reaction;
    return cram_bits(
        LEVEL(casted->index),
        casted->deadline,
        execution_times_get_or_else(execution_times, casted)
    );
}

///////////////////// Scheduler Worker API (public) /////////////////////////

void lf_sched_init(size_t number_of_workers, sched_params_t* params) {
    lf_sched_init_base(number_of_workers, params, get_lst_priority);
    size_t hashmap_size = compute_hashmap_size(params);
    start_times = start_time_hashmap_new(hashmap_size, NULL);
    execution_times = execution_time_hashmap_new(hashmap_size, NULL);
}

void lf_sched_free() { lf_sched_free_base(); }

reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    reaction_t* ret = lf_sched_get_ready_reaction_base(worker_number);
    start_time_hashmap_put(start_times, ret, get_physical_time());
    return ret;
}

void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    interval_t now = get_physical_time();
    execution_time_hashmap_put(
        execution_times, done_reaction, (
            3 * execution_time_hashmap_get_or_else(execution_times, done_reaction, 0)
            + now - start_time_hashmap_get_or_else(start_times, done_reaction, now)
        ) << 2
    );
    lf_sched_done_with_reaction_base(worker_number, done_reaction);
}

void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    lf_sched_trigger_reaction_base(reaction, worker_number);
}
