/*************
Copyright (c) 2022, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/**
 * Management of worker wakefulness and locking.
 * @author{Peter Donovan <peterdonovan@berkeley.edu>}
 */

#ifndef WORKER_STATES
#define WORKER_STATES

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include "scheduler.h"
#include "../platform.h"

/** An array of condition variables, each corresponding to a group of workers. */
static lf_cond_t* worker_conds;
/** The cumsum of the sizes of the groups of workers corresponding to each successive cond. */
static size_t* cumsum_of_worker_group_sizes;
/** The number of non-waiting threads. */
static volatile size_t num_loose_threads;
/** The number of threads that were awakened for the purpose of executing the current level. */
static volatile size_t num_awakened;
/** Whether the mutex is held by each worker via this module's API. */
static bool* mutex_held;

/** See worker_assignments.h for documentation. */
extern size_t current_level;
extern size_t** num_reactions_by_worker_by_level;
extern size_t max_num_workers;

/** See reactor_threaded.c for documentation. */
extern lf_mutex_t mutex;

/** See reactor_common.c for documentation. */
extern bool fast;

/**
 * The level counter is a number that changes whenever the current level changes.
 *
 * This number must have a very long period in the sense that if it changes and is checked at a time
 * in the future that is selected from some "reasonable" distribution, the probability that it will
 * have returned to the same value must be negligible.
 */
static size_t level_counter = 0;

/**
 * @brief Return the index of the condition variable used by worker.
 * 
 * This function is nondecreasing, and the least element of its image is zero.
 * 
 * @param worker A worker number.
 * @return size_t The index of the condition variable used by worker.
 */
static size_t cond_of(size_t worker) {
    // Note: __builtin_clz with GCC might be preferred, or fls (?).
    int ret = 0;
    while (worker) {
        ret++;
        worker >>= 1;
    }
    return ret;
}

static void worker_states_init(size_t number_of_workers) {
    size_t greatest_worker_number = number_of_workers - 1;
    size_t num_conds = cond_of(greatest_worker_number) + 1;
    worker_conds = (lf_cond_t*) malloc(sizeof(lf_cond_t) * num_conds);
    cumsum_of_worker_group_sizes = (size_t*) calloc(num_conds, sizeof(size_t));
    mutex_held = (bool*) calloc(number_of_workers, sizeof(bool));
    for (int i = 0; i < number_of_workers; i++) {
        cumsum_of_worker_group_sizes[cond_of(i)]++;
    }
    for (int i = 1; i < num_conds; i++) {
        cumsum_of_worker_group_sizes[i] += cumsum_of_worker_group_sizes[i - 1];
    }
    for (int i = 0; i < num_conds; i++) {
        lf_cond_init(worker_conds + i);
    }
    num_loose_threads = number_of_workers;
}

static void worker_states_free() {
    // FIXME: Why do the condition variables and mutexes not need to be freed?
    free(worker_conds);
    free(mutex_held);
}

/**
 * @brief Awaken the workers scheduled to work on the current level.
 * 
 * @param worker The calling worker.
 * @param num_to_awaken The number of workers to awaken.
 * @return A snapshot of the level counter after awakening the workers.
 */
static void worker_states_awaken_locked(size_t worker, size_t num_to_awaken) {
    assert(num_to_awaken <= max_num_workers);
    if ((worker == 0) && (num_to_awaken <= 1)) {
        num_loose_threads = 1;
        return;
    }
    size_t greatest_worker_number_to_awaken = num_to_awaken - 1;
    size_t max_cond = cond_of(greatest_worker_number_to_awaken);
    if (!mutex_held[worker]) {
        mutex_held[worker] = true;
        lf_mutex_lock(&mutex);
    }
    // The predicate of the condition variable depends on num_awakened and level_counter, so
    // this is a critical section.
    num_loose_threads = cumsum_of_worker_group_sizes[max_cond];
    num_loose_threads += worker >= num_loose_threads;
    num_awakened = num_loose_threads;
    level_counter++;
    for (int cond = 0; cond <= max_cond; cond++) {
        lf_cond_broadcast(worker_conds + cond);
    }
}

/** Lock the global mutex if needed. */
static void worker_states_lock(size_t worker) {
    assert(num_loose_threads > 0);
    assert(num_loose_threads <= max_num_workers);
    size_t lt = num_loose_threads;
    if (lt > 1 || !fast) {  // FIXME: Lock should be partially optimized out even when !fast
        lf_mutex_lock(&mutex);
        assert(!mutex_held[worker]);
        mutex_held[worker] = true;
    }
}

/** Unlock the global mutex if needed. */
static void worker_states_unlock(size_t worker) {
    if (!mutex_held[worker]) return;
    mutex_held[worker] = false;
    lf_mutex_unlock(&mutex);
}

/**
 * @brief Record that worker is finished working on the current level.
 *
 * @param worker The number of a worker.
 * @return true If this is the last worker to finish working on the current level.
 * @return false If at least one other worker is still working on the current level.
 */
static bool worker_states_finished_with_level_locked(size_t worker) {
    assert(worker >= 0);
    assert(num_loose_threads > 0);
    assert(num_reactions_by_worker[worker] != 1);
    assert(((int64_t) num_reactions_by_worker[worker]) <= 0);
    // Why use an atomic operation when we are supposed to be "as good as locked"? Because I took a
    // shortcut, and the shortcut was imperfect.
    size_t ret = lf_atomic_add_fetch(&num_loose_threads, -1);
    assert(ret <= max_num_workers);  // Check for underflow
    return !ret;
}

/**
 * @brief Make the given worker go to sleep.
 *
 * This should be called by the given worker when the worker will do nothing for the remainder of
 * the execution of the current level.
 *
 * @param worker The number of the calling worker.
 * @param level_counter_snapshot The value of the level counter at the time of the decision to
 * sleep.
 */
static void worker_states_sleep_and_unlock(size_t worker, size_t level_counter_snapshot) {
    assert(worker < max_num_workers);
    assert(num_loose_threads <= max_num_workers);
    if (!mutex_held[worker]) {
        lf_mutex_lock(&mutex);
    }
    mutex_held[worker] = false;  // This will be true soon, upon call to lf_cond_wait.
    size_t cond = cond_of(worker);
    if (
        ((level_counter_snapshot == level_counter) || worker >= num_awakened)
    ) {
        do {
            lf_cond_wait(worker_conds + cond, &mutex);
        } while (level_counter_snapshot == level_counter || worker >= num_awakened);
    }
    assert(!mutex_held[worker]);  // This thread holds the mutex, but it did not report that.
    lf_mutex_unlock(&mutex);
}

#endif
