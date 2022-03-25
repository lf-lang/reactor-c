/*************
Copyright (c) 2022, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

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

extern bool fast;

static bool init_called = false;
static bool should_stop = false;

///////////////////////// Scheduler Private Functions ///////////////////////////

/**
 * @brief Increment the level currently being executed, and the tag if need be.
 * 
 * Sleep thereafter if that is what the current worker ought to do.
 * @param worker The number of the calling worker.
 */
static void advance_level_and_unlock(size_t worker) {
    if (try_advance_level()) {
        if (!fast && !mutex_held[worker]) {
            mutex_held[worker] = true;
            lf_mutex_lock(&mutex);
        }
        if (_lf_sched_advance_tag_locked()) {
            should_stop = true;
            worker_states_never_sleep_again(worker);
            worker_states_unlock(worker);
            return;
        }
    }
    size_t num_workers_to_awaken = num_workers;
    assert(num_workers_to_awaken > 0);
    worker_states_awaken_locked(worker, num_workers_to_awaken);
    worker_states_unlock(worker);
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
    while (!(ret = worker_assignments_get_or_lock(worker_number))) {
        size_t level_counter_snapshot = level_counter;
        if (worker_states_finished_with_level_locked(worker_number)) {
            // printf("%d !\n", worker_number);
            advance_level_and_unlock(worker_number);
        } else {
            worker_states_sleep_and_unlock(worker_number, level_counter_snapshot);
        }
        if (should_stop) {
            return NULL;
        }
    }
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
