/**
 * This is a non-priority-driven scheduler. See scheduler.h for documentation.
 */


#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>

#include "scheduler.h"
#include "../utils/pqueue_support.h"
#include "scheduler_sync_tag_advance.c"
#include "worker_assignments.h"
#include "worker_states.h"

#ifndef MAX_REACTION_LEVEL
#define MAX_REACTION_LEVEL INITIAL_REACT_QUEUE_SIZE
#endif

static bool init_called = false;
static bool should_stop = false;

extern lf_mutex_t mutex;

///////////////////////// Scheduler Private Functions ///////////////////////////

/**
 * @brief Increment the level currently being executed, and the tag if need be.
 * 
 * Sleep thereafter if that is what the current worker ought to do.
 * @param worker The number of the calling worker.
 */
static void advance_level(size_t worker) {
    lf_mutex_lock(&mutex);  // FIXME: do this elsewhere?
    if (try_increment_level()) {
        // There is no need to actually acquire a lock here. Due to the use of an atomic to
        // determine which worker advances level, only one worker can be doing this.
        if (_lf_sched_advance_tag_locked()) {
            should_stop = true;
            worker_states_never_sleep_again();
            lf_mutex_unlock(&mutex);
            return;
        }
    }
    size_t num_workers_busy = get_num_workers_busy();
    size_t level_snapshot = worker_states_awaken_locked(num_workers_busy);
    lf_mutex_unlock(&mutex);
    if (num_workers_busy < worker && num_workers_busy) {
        printf("DEBUG: %ld sleeps; others should be busy.\n", worker);
        worker_states_sleep(worker, level_snapshot);
    }
}

///////////////////// Scheduler Init and Destroy API /////////////////////////

void lf_sched_init(size_t number_of_workers, sched_params_t* params) {
    // TODO: Instead of making this a no-op, crash the program. If this gets called twice, then that
    // is a bug that should be fixed.
    if (init_called) return;
    worker_states_init(number_of_workers);
    worker_assignments_init(number_of_workers, params);
    init_called = true;
}

void lf_sched_free() {
    worker_states_free();
    worker_assignments_free();
}

///////////////////////// Scheduler Worker API ///////////////////////////////

reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    assert(worker_number >= 0);
    reaction_t* ret;
    while (!(ret = worker_assignments_get(worker_number))) {
        printf("DEBUG: %d failed to get.\n", worker_number);
        size_t level_counter_snapshot = level_counter;
        if (worker_assignments_finished_with_level(worker_number)) {
            // TODO: Advance level all the way to the next level with at least one reaction?
            advance_level(worker_number);
            printf("%d !\n", worker_number);
        } else {
            //printf("DEBUG: %d sleeps; finished early.\n", worker_number);
            worker_states_sleep(worker_number, level_counter_snapshot);
        }
        if (should_stop) {
            //printf("DEBUG: %d should stop.\n", worker_number);
            return NULL;
        }
        //printf("DEBUG: %d will try to get.\n", worker_number);
    }
    printf("%d <- %p @ %ld\n", worker_number, ret, LEVEL(ret->index));
    return (reaction_t*) ret;
}

void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    assert(worker_number >= 0);
    assert(done_reaction->status != inactive);
    done_reaction->status = inactive;
}

void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    assert(worker_number >= -1);
    if (reaction == NULL) return; // FIXME: When does this happen again? In federated execution?
    if (!lf_bool_compare_and_swap(&reaction->status, inactive, queued)) return;
    worker_assignments_put(reaction);
}
