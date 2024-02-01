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
 * @author{Peter Donovan <peterdonovan@berkeley.edu>}
 */
#include "lf_types.h"
#if defined SCHEDULER && SCHEDULER == SCHED_ADAPTIVE
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>

#include "environment.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "environment.h"
#include "util.h"

#ifndef MAX_REACTION_LEVEL
#define MAX_REACTION_LEVEL INITIAL_REACT_QUEUE_SIZE
#endif

void try_advance_level(environment_t* env, volatile size_t* next_reaction_level);

/////////////////// Forward declarations /////////////////////////
extern bool fast;
static void worker_states_lock(lf_scheduler_t* scheduler, size_t worker);
static void worker_states_unlock(lf_scheduler_t* scheduler, size_t worker);
static void data_collection_init(lf_scheduler_t* scheduler, sched_params_t* params);
static void data_collection_free(lf_scheduler_t* scheduler);
static void data_collection_start_level(lf_scheduler_t* scheduler, size_t level);
static void data_collection_end_level(lf_scheduler_t* scheduler, size_t level, size_t num_workers);
static void data_collection_end_tag(
    lf_scheduler_t* scheduler,
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level
);
/**
 * The level counter is a number that changes whenever the current level changes.
 *
 * This number must have a very long period in the sense that if it changes and is checked at a time
 * in the future that is selected from some "reasonable" distribution, the probability that it will
 * have returned to the same value must be negligible.
 */


/////////////////// Scheduler Variables and Structs /////////////////////////
typedef struct {
    /** An array of condition variables, each corresponding to a group of workers. */
    lf_cond_t* worker_conds;
    /** The cumsum of the sizes of the groups of workers corresponding to each successive cond. */
    size_t* cumsum_of_worker_group_sizes;
    /** The number of non-waiting threads. */
    volatile size_t num_loose_threads;
    /** The number of threads that were awakened for the purpose of executing the current level. */
    volatile size_t num_awakened;
    /** Whether the mutex is held by each worker via this module's API. */
    bool* mutex_held;
} worker_states_t;


typedef struct {
    /** The queued reactions. */
    reaction_t**** reactions_by_worker_by_level;
    /** The number of queued reactions currently assigned to each worker at each level. */
    size_t** num_reactions_by_worker_by_level;
    /** The maximum number of workers that could possibly be kept simultaneously busy at each level. */
    size_t* max_num_workers_by_level;
    /** The number of workers that will be used to execute each level. */
    size_t* num_workers_by_level;
    /** The number of levels. */
    size_t num_levels;
    /** The maximum number of workers that can be used to execute any level. */
    size_t max_num_workers;
    /** The following values apply to the current level. */
    size_t current_level;
    /** The number of reactions each worker still has to execute, indexed by worker. */
    size_t* num_reactions_by_worker;
    /** The reactions to be executed, indexed by assigned worker. */
    reaction_t*** reactions_by_worker;
    /** The total number of workers active, including those who have finished their work. */
    size_t num_workers;
} worker_assignments_t;

typedef struct {
    interval_t* start_times_by_level;
    interval_t** execution_times_by_num_workers_by_level;
    interval_t* execution_times_mins;
    size_t* execution_times_argmins;
    size_t data_collection_counter;
    bool collecting_data;
    size_t* possible_nums_workers;
    size_t num_levels;
} data_collection_t;

typedef struct custom_scheduler_data_t {
    worker_states_t* worker_states;
    worker_assignments_t* worker_assignments;
    data_collection_t* data_collection;
    bool init_called;
    bool should_stop;
    size_t level_counter;
} custom_scheduler_data_t;

///////////////////////// Scheduler Private Functions ///////////////////////////

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

///////////////////////// Private Worker Assignments Functions ///////////////////////////

/**
 * @brief Set the level to be executed now. This function assumes that concurrent calls to it are
 * impossible.
 * @param level The new current level.
 */
static void set_level(lf_scheduler_t* scheduler, size_t level) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    assert(level < worker_assignments->num_levels);
    assert(0 <= level);
    data_collection_end_level(scheduler, worker_assignments->current_level, worker_assignments->num_workers);
    worker_assignments->current_level = level;
    worker_assignments->num_reactions_by_worker = worker_assignments->num_reactions_by_worker_by_level[level];
    worker_assignments->reactions_by_worker = worker_assignments->reactions_by_worker_by_level[level];
    worker_assignments->num_workers = worker_assignments->num_workers_by_level[level];
    // TODO: Experiment with not recording that the level is starting in the case that there is
    // nothing to execute. We need not optimize for the case when there is nothing to execute
    // because that case is not merely optimized, but is optimized out (we do not bother with
    // executing nothing).
    data_collection_start_level(scheduler, worker_assignments->current_level);
}

/** @brief Return the total number of reactions enqueued on the current level. */
static size_t get_num_reactions(lf_scheduler_t* scheduler) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    size_t total_num_reactions = 0;
    for (size_t i = 0; i < worker_assignments->num_workers; i++) {
        total_num_reactions += worker_assignments->num_reactions_by_worker[i];
    }
    // TODO: if num_workers was > total_num_reactions, report this to data_collection?
    return total_num_reactions;
}

static void worker_assignments_init(lf_scheduler_t* scheduler, size_t number_of_workers, sched_params_t* params) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    worker_assignments->num_levels = params->num_reactions_per_level_size;
    worker_assignments->max_num_workers = number_of_workers;
    worker_assignments->reactions_by_worker_by_level = (reaction_t****) malloc(sizeof(reaction_t***) * worker_assignments->num_levels);
    worker_assignments->num_reactions_by_worker_by_level = (size_t**) malloc(sizeof(size_t*) * worker_assignments->num_levels);
    worker_assignments->num_workers_by_level = (size_t*) malloc(sizeof(size_t) * worker_assignments->num_levels);
    worker_assignments->max_num_workers_by_level = (size_t*) malloc(sizeof(size_t) * worker_assignments->num_levels);
    for (size_t level = 0; level < worker_assignments->num_levels; level++) {
        size_t num_reactions = params->num_reactions_per_level[level];
        size_t num_workers = num_reactions < worker_assignments->max_num_workers ? num_reactions : worker_assignments->max_num_workers;
        worker_assignments->max_num_workers_by_level[level] = num_workers;
        worker_assignments->num_workers_by_level[level] = worker_assignments->max_num_workers_by_level[level];
        worker_assignments->reactions_by_worker_by_level[level] = (reaction_t***) malloc(
            sizeof(reaction_t**) * worker_assignments->max_num_workers
        );
        worker_assignments->num_reactions_by_worker_by_level[level] = (size_t*) calloc(worker_assignments->max_num_workers, sizeof(size_t));
        for (size_t worker = 0; worker < worker_assignments->max_num_workers_by_level[level]; worker++) {
            worker_assignments->reactions_by_worker_by_level[level][worker] = (reaction_t**) malloc(
                sizeof(reaction_t*) * num_reactions
            );  // Warning: This wastes space.
        }
    }
    set_level(scheduler, 0);
}

static void worker_assignments_free(lf_scheduler_t* scheduler) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    for (size_t level = 0; level < worker_assignments->num_levels; level++) {
        for (size_t worker = 0; worker < worker_assignments->max_num_workers_by_level[level]; worker++) {
            free(worker_assignments->reactions_by_worker_by_level[level][worker]);
        }
        free(worker_assignments->reactions_by_worker_by_level[level]);
        free(worker_assignments->num_reactions_by_worker_by_level[level]);
    }
    free(worker_assignments->max_num_workers_by_level);
    free(worker_assignments->num_workers_by_level);
}

/**
 * @brief Return a reaction that has been assigned to the given worker, or NULL if no such reaction
 * exists.
 * @param worker The number of a worker needing work.
 */
static reaction_t* get_reaction(lf_scheduler_t* scheduler, size_t worker) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
#ifndef FEDERATED
    int index = lf_atomic_add_fetch(worker_assignments->num_reactions_by_worker + worker, -1);
    if (index >= 0) {
        return worker_assignments->reactions_by_worker[worker][index];
    }
    worker_assignments->num_reactions_by_worker[worker] = 0;
    return NULL;
#else
    // This is necessary for federated programs because reactions may be inserted into the current
    // level.
    int old_num_reactions;
    int current_num_reactions = worker_assignments->num_reactions_by_worker[worker];
    int index;
    do {
        old_num_reactions = current_num_reactions;
        if (old_num_reactions <= 0) return NULL;
    } while (
        (current_num_reactions = lf_val_compare_and_swap(
            worker_assignments->num_reactions_by_worker + worker,
            old_num_reactions,
            (index = old_num_reactions - 1)
        )) != old_num_reactions
    );
    return worker_assignments->reactions_by_worker[worker][index];
#endif
}

/**
 * @brief Get a reaction for the given worker to execute. If no such reaction exists, claim the
 * mutex.
 * @param worker A worker requesting work.
 * @return reaction_t* A reaction to execute, or NULL if no such reaction exists.
 */
static reaction_t* worker_assignments_get_or_lock(lf_scheduler_t* scheduler, size_t worker) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    assert(worker >= 0);
    // assert(worker < num_workers);  // There are edge cases where this doesn't hold.
    assert(worker_assignments->num_reactions_by_worker[worker] >= 0);
    reaction_t* ret;
    while (true) {
        if ((ret = get_reaction(scheduler, worker))) return ret;
        if (worker < worker_assignments->num_workers) {
            for (
                size_t victim = (worker + 1) % worker_assignments->num_workers;
                victim != worker;
                victim = (victim + 1) % worker_assignments->num_workers
            ) {
                if ((ret = get_reaction(scheduler, victim))) return ret;
            }
        }
        worker_states_lock(scheduler, worker);
        if (!worker_assignments->num_reactions_by_worker[worker]) {
            return NULL;
        }
        worker_states_unlock(scheduler, worker);
    }
}

/**
 * @brief Trigger the given reaction.
 * @param reaction A reaction to be executed in the current tag.
 */
static void worker_assignments_put(lf_scheduler_t* scheduler, reaction_t* reaction) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    size_t level = LF_LEVEL(reaction->index);
    assert(reaction != NULL);
#ifndef FEDERATED
    assert(level > worker_assignments->current_level || worker_assignments->current_level == 0);
#endif
    assert(level < worker_assignments->num_levels);
    // Source: https://xorshift.di.unimi.it/splitmix64.c
    // TODO: This is probably not the most efficient way to get the randomness that we need because
    // it is designed to give an entire word of randomness, whereas we only need
    // ~log2(num_workers_by_level[level]) bits of randomness.
    uint64_t hash = (uint64_t) reaction;
    hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9;
    hash = (hash ^ (hash >> 27)) * 0x94d049bb133111eb;
    hash = hash ^ (hash >> 31);
    size_t worker = hash % worker_assignments->num_workers_by_level[level];
    size_t num_preceding_reactions = lf_atomic_fetch_add(
        &worker_assignments->num_reactions_by_worker_by_level[level][worker],
        1
    );
    worker_assignments->reactions_by_worker_by_level[level][worker][num_preceding_reactions] = reaction;
}


///////////////////////// Private Worker States Functions ///////////////////////////

static void worker_states_init(lf_scheduler_t* scheduler, size_t number_of_workers) {
    worker_states_t* worker_states =scheduler->custom_data->worker_states;
    size_t greatest_worker_number = number_of_workers - 1;
    size_t num_conds = cond_of(greatest_worker_number) + 1;
    worker_states->worker_conds = (lf_cond_t*) malloc(sizeof(lf_cond_t) * num_conds);
    worker_states->cumsum_of_worker_group_sizes = (size_t*) calloc(num_conds, sizeof(size_t));
    worker_states->mutex_held = (bool*) calloc(number_of_workers, sizeof(bool));
    for (int i = 0; i < number_of_workers; i++) {
        worker_states->cumsum_of_worker_group_sizes[cond_of(i)]++;
    }
    for (int i = 1; i < num_conds; i++) {
        worker_states->cumsum_of_worker_group_sizes[i] += worker_states->cumsum_of_worker_group_sizes[i - 1];
    }
    for (int i = 0; i < num_conds; i++) {
        lf_cond_init(worker_states->worker_conds + i, &scheduler->env->mutex);
    }
    worker_states->num_loose_threads = scheduler->number_of_workers;
}

static void worker_states_free(lf_scheduler_t* scheduler) {
    // FIXME: Why do the condition variables and mutexes not need to be freed?
    worker_states_t* worker_states =scheduler->custom_data->worker_states;
    free(worker_states->worker_conds);
    free(worker_states->mutex_held);
}

/**
 * @brief Awaken the workers scheduled to work on the current level.
 *
 * @param worker The calling worker.
 * @param num_to_awaken The number of workers to awaken.
 * @return A snapshot of the level counter after awakening the workers.
 */
static void worker_states_awaken_locked(lf_scheduler_t* scheduler, size_t worker, size_t num_to_awaken) {
    worker_states_t* worker_states = scheduler->custom_data->worker_states;
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    assert(num_to_awaken <= worker_assignments->max_num_workers);
    if ((worker == 0) && (num_to_awaken <= 1)) {
        worker_states->num_loose_threads = 1;
        return;
    }
    size_t greatest_worker_number_to_awaken = num_to_awaken - 1;
    size_t max_cond = cond_of(greatest_worker_number_to_awaken);
    if (!worker_states->mutex_held[worker]) {
        worker_states->mutex_held[worker] = true;
        lf_mutex_lock(&scheduler->env->mutex);
    }
    // The predicate of the condition variable depends on num_awakened and level_counter, so
    // this is a critical section.
    worker_states->num_loose_threads = worker_states->cumsum_of_worker_group_sizes[max_cond];
    worker_states->num_loose_threads += worker >= worker_states->num_loose_threads;
    worker_states->num_awakened = worker_states->num_loose_threads;
    scheduler->custom_data->level_counter++;
    for (int cond = 0; cond <= max_cond; cond++) {
        lf_cond_broadcast(worker_states->worker_conds + cond);
    }
}

/** Lock the global mutex if needed. */
static void worker_states_lock(lf_scheduler_t* scheduler, size_t worker) {
    worker_states_t* worker_states = scheduler->custom_data->worker_states;
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    assert(worker_states->num_loose_threads > 0);
    assert(worker_states->num_loose_threads <= worker_assignments->max_num_workers);
    size_t lt = worker_states->num_loose_threads;
    if (lt > 1 || !fast) {  // FIXME: Lock should be partially optimized out even when !fast
        lf_mutex_lock(&scheduler->env->mutex);
        assert(!worker_states->mutex_held[worker]);
        worker_states->mutex_held[worker] = true;
    }
}

/** Unlock the global mutex if needed. */
static void worker_states_unlock(lf_scheduler_t* scheduler, size_t worker) {
    worker_states_t* worker_states = scheduler->custom_data->worker_states;
    if (!worker_states->mutex_held[worker]) return;
    worker_states->mutex_held[worker] = false;
    lf_mutex_unlock(&scheduler->env->mutex);
}

/**
 * @brief Record that worker is finished working on the current level.
 *
 * @param worker The number of a worker.
 * @return true If this is the last worker to finish working on the current level.
 * @return false If at least one other worker is still working on the current level.
 */
static bool worker_states_finished_with_level_locked(lf_scheduler_t* scheduler, size_t worker) {
    worker_states_t* worker_states = scheduler->custom_data->worker_states;
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    assert(worker >= 0);
    assert(worker_states->num_loose_threads > 0);
    assert(worker_assignments->num_reactions_by_worker[worker] != 1);
    assert(((int64_t) worker_assignments->num_reactions_by_worker[worker]) <= 0);
    // Why use an atomic operation when we are supposed to be "as good as locked"? Because I took a
    // shortcut, and the shortcut was imperfect.
    size_t ret = lf_atomic_add_fetch(&worker_states->num_loose_threads, -1);
    assert(ret <= worker_assignments->max_num_workers);  // Check for underflow
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
static void worker_states_sleep_and_unlock(lf_scheduler_t* scheduler, size_t worker, size_t level_counter_snapshot) {
    worker_states_t* worker_states = scheduler->custom_data->worker_states;
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    assert(worker < worker_assignments->max_num_workers);
    assert(worker_states->num_loose_threads <= worker_assignments->max_num_workers);
    if (!worker_states->mutex_held[worker]) {
        lf_mutex_lock(&scheduler->env->mutex);
    }
    worker_states->mutex_held[worker] = false;  // This will be true soon, upon call to lf_cond_wait.
    size_t cond = cond_of(worker);
    if (
        ((level_counter_snapshot == scheduler->custom_data->level_counter) || worker >= worker_states->num_awakened)
    ) {
        do {
            lf_cond_wait(worker_states->worker_conds + cond);
        } while (level_counter_snapshot == scheduler->custom_data->level_counter || worker >= worker_states->num_awakened);
    }
    assert(!worker_states->mutex_held[worker]);  // This thread holds the mutex, but it did not report that.
    lf_mutex_unlock(&scheduler->env->mutex);
}

/**
 * @brief Increment the level currently being executed, and the tag if necessary.
 * @param worker The number of the calling worker.
 */
static void advance_level_and_unlock(lf_scheduler_t* scheduler, size_t worker) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    worker_states_t* worker_states = scheduler->custom_data->worker_states;
    size_t max_level = worker_assignments->num_levels - 1;
    while (true) {
        if (worker_assignments->current_level == max_level) {
            data_collection_end_tag(
                scheduler,
                worker_assignments->num_workers_by_level, 
                worker_assignments->max_num_workers_by_level);
            set_level(scheduler, 0);
            if (_lf_sched_advance_tag_locked(scheduler)) {
                scheduler->custom_data->should_stop = true;
                worker_states_awaken_locked(scheduler, worker, worker_assignments->max_num_workers);
                worker_states_unlock(scheduler, worker);
                return;
            }
        } else {
            try_advance_level(scheduler->env, &worker_assignments->current_level);
            set_level(scheduler, worker_assignments->current_level);
        }
        size_t total_num_reactions = get_num_reactions(scheduler);
        if (total_num_reactions) {
            size_t num_workers_to_awaken = LF_MIN(total_num_reactions, worker_assignments->num_workers);
            assert(num_workers_to_awaken > 0);
            worker_states_awaken_locked(scheduler, worker, num_workers_to_awaken);
            worker_states_unlock(scheduler, worker);
            return;
        }
    }
}

///////////////////////// Private Data Collection Functions ///////////////////////////


/**
 * A monotonically increasing sequence of numbers of workers, the first and last elements of which
 * are too large or small to represent valid states of the system (i.e., state transitions to them
 * are instantaneously reflected).
 */

#define START_EXPERIMENTS 8
#define SLOW_EXPERIMENTS 256
#define EXECUTION_TIME_MEMORY 15

/** @brief Initialize the possible_nums_workers array. */
static void possible_nums_workers_init(lf_scheduler_t* scheduler) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    // Start with 0 and end with two numbers strictly greater than max_num_workers. This must start
    // at 4 because the first two and last two entries are not counted.
    size_t pnw_length = 4;
    size_t temp = worker_assignments->max_num_workers;
    while ((temp >>= 1)) pnw_length++;
    data_collection->possible_nums_workers = (size_t*) malloc(pnw_length * sizeof(size_t));
    temp = 1;
    data_collection->possible_nums_workers[0] = 0;
    for (int i = 1; i < pnw_length; i++) {
        data_collection->possible_nums_workers[i] = temp;
        temp *= 2;
    }
    assert(temp > worker_assignments->max_num_workers);
}

/**
 * @brief Return a random integer in the interval [-1, +1] representing whether the number of
 * workers used on a certain level should increase, decrease, or remain the same, with a probability
 * distribution possibly dependent on the parameters.
 * @param current_state The index currently used by this level in the possible_nums_workers array.
 * @param execution_time An estimate of the execution time of the level in the case for which we
 * would like to optimize.
 */
static int get_jitter(size_t current_state, interval_t execution_time) {
    static const size_t parallelism_cost_max = 114688;
    // The following handles the case where the current level really is just fluff:
    // No parallelism needed, no work to be done.
    if (execution_time < 16384 && current_state == 1) return 0;
    int left_score = 16384;  // Want: For execution time = 65536, p(try left) = p(try right)
    int middle_score = 65536;
    int right_score = 65536;
    if (execution_time < parallelism_cost_max) left_score += parallelism_cost_max - execution_time;
    int result = rand() % (left_score + middle_score + right_score);
    if (result < left_score) return -1;
    if (result < left_score + middle_score) return 0;
    return 1;
}

/** @brief Get the number of workers resulting from a random state transition. */
static size_t get_nums_workers_neighboring_state(lf_scheduler_t* scheduler, size_t current_state, interval_t execution_time) {
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    size_t jitter = get_jitter(current_state, execution_time);
    if (!jitter) return current_state;
    size_t i = 1;
    // TODO: There are more efficient ways to do this.
    while (data_collection->possible_nums_workers[i] < current_state) i++;
    return data_collection->possible_nums_workers[i + jitter];
}

static void data_collection_init(lf_scheduler_t* scheduler, sched_params_t* params) {
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    data_collection->num_levels = params->num_reactions_per_level_size;
    data_collection->start_times_by_level = (interval_t*) calloc(data_collection->num_levels, sizeof(interval_t));
    data_collection->execution_times_by_num_workers_by_level = (interval_t**) calloc(
        data_collection->num_levels, sizeof(interval_t*)
    );
    data_collection->execution_times_mins = (interval_t*) calloc(data_collection->num_levels, sizeof(interval_t));
    data_collection->execution_times_argmins = (size_t*) calloc(data_collection->num_levels, sizeof(size_t));
    for (size_t i = 0; i < data_collection->num_levels; i++) {
        data_collection->execution_times_argmins[i] = worker_assignments->max_num_workers;
        data_collection->execution_times_by_num_workers_by_level[i] = (interval_t*) calloc(
            worker_assignments->max_num_workers + 1,  // Add 1 for 1-based indexing
            sizeof(interval_t)
        );
    }
    possible_nums_workers_init(scheduler);
}

// FIXME: This dependes on worker_assignments not being freed yet
static void data_collection_free(lf_scheduler_t* scheduler) {
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    free(data_collection->start_times_by_level);
    for (size_t i = 0; i < data_collection->num_levels; i++) {
        free(data_collection->execution_times_by_num_workers_by_level[i]);
    }
    free(data_collection->execution_times_by_num_workers_by_level);
    free(data_collection->possible_nums_workers);
}

/** @brief Record that the execution of the given level is beginning. */
static void data_collection_start_level(lf_scheduler_t* scheduler, size_t level) {
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    if (data_collection->collecting_data) data_collection->start_times_by_level[level] = lf_time_physical();
}

/** @brief Record that the execution of the given level has completed. */
static void data_collection_end_level(lf_scheduler_t* scheduler, size_t level, size_t num_workers) {
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    if (data_collection->collecting_data && data_collection->start_times_by_level[level]) {
        interval_t dt = lf_time_physical() - data_collection->start_times_by_level[level];
        if (!data_collection->execution_times_by_num_workers_by_level[level][num_workers]) {
            data_collection->execution_times_by_num_workers_by_level[level][num_workers] = LF_MAX(
                dt,
                2 * data_collection->execution_times_by_num_workers_by_level[level][data_collection->execution_times_argmins[level]]
            );
        }
        interval_t* prior_et = &data_collection->execution_times_by_num_workers_by_level[level][num_workers];
        *prior_et = (*prior_et * EXECUTION_TIME_MEMORY + dt) / (EXECUTION_TIME_MEMORY + 1);
    }
}

static size_t restrict_to_range(size_t start_inclusive, size_t end_inclusive, size_t value) {
    assert(start_inclusive <= end_inclusive);
    if (value < start_inclusive) return start_inclusive;
    if (value > end_inclusive) return end_inclusive;
    return value;
}

/**
 * @brief Update num_workers_by_level in-place.
 * @param num_workers_by_level The number of workers that should be used to execute each level.
 * @param max_num_workers_by_level The maximum possible number of workers that could reasonably be
 * assigned to each level.
 * @param jitter Whether the possibility of state transitions to numbers of workers that are not
 * (yet) empirically optimal is desired.
 */
static void compute_number_of_workers(
    lf_scheduler_t* scheduler,
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level,
    bool jitter
) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    for (size_t level = 0; level < data_collection->num_levels; level++) {
        interval_t this_execution_time = data_collection->execution_times_by_num_workers_by_level[level][
            num_workers_by_level[level]
        ];
        size_t ideal_number_of_workers;
        size_t max_reasonable_num_workers = max_num_workers_by_level[level];
        ideal_number_of_workers = data_collection->execution_times_argmins[level];
        int range = 1;
        if (jitter) {
            ideal_number_of_workers = get_nums_workers_neighboring_state(
                scheduler, ideal_number_of_workers, this_execution_time
            );
        }
        int minimum_workers = 1;
        num_workers_by_level[level] = restrict_to_range(
            minimum_workers, max_reasonable_num_workers, ideal_number_of_workers
        );
    }
}

/**
 * @brief Update minimum and argmin (wrt number of workers used) execution times according the most
 * recent execution times recorded.
 * @param num_workers_by_level The number of workers most recently used to execute each level.
 */
static void compute_costs(lf_scheduler_t* scheduler, size_t* num_workers_by_level) {
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    for (size_t level = 0; level < data_collection->num_levels; level++) {
        interval_t score = data_collection->execution_times_by_num_workers_by_level[level][
            num_workers_by_level[level]
        ];
        if (
            !data_collection->execution_times_mins[level]
            | (score < data_collection->execution_times_mins[level])
            | (num_workers_by_level[level] == data_collection->execution_times_argmins[level])
        ) {
            data_collection->execution_times_mins[level] = score;
            data_collection->execution_times_argmins[level] = num_workers_by_level[level];
        }
    }
}

/**
 * @brief Record that the execution of a tag has completed.
 * @param num_workers_by_level The number of workers used to execute each level of the tag.
 * @param max_num_workers_by_level The maximum number of workers that could reasonably be used to
 * execute each level, for any tag.
 */
static void data_collection_end_tag(
    lf_scheduler_t* scheduler,
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level
) {
    worker_assignments_t * worker_assignments = scheduler->custom_data->worker_assignments;
    data_collection_t* data_collection = scheduler->custom_data->data_collection;
    if (data_collection->collecting_data && data_collection->start_times_by_level[0]) {
        compute_costs(scheduler, num_workers_by_level);
    }
    data_collection->data_collection_counter++;
    size_t period = 2 + 128 * (data_collection->data_collection_counter > SLOW_EXPERIMENTS);
    size_t state = data_collection->data_collection_counter % period;
    if (state == 0) {
        compute_number_of_workers(
            scheduler,
            num_workers_by_level,
            max_num_workers_by_level,
            data_collection->data_collection_counter > START_EXPERIMENTS
        );
        data_collection->collecting_data = true;
    } else if (state == 1) {
        compute_number_of_workers(scheduler, num_workers_by_level, max_num_workers_by_level, false);
        data_collection->collecting_data = false;
    }
}


///////////////////// Scheduler Init and Destroy API /////////////////////////
void lf_sched_init(environment_t* env, size_t number_of_workers, sched_params_t* params) {
    assert(env != GLOBAL_ENVIRONMENT);

    // TODO: Instead of making this a no-op, crash the program. If this gets called twice, then that
    // is a bug that should be fixed.
    if(!init_sched_instance(env, &env->scheduler, number_of_workers, params)) {
        // Already initialized
        return;
    }
    
    lf_scheduler_t* scheduler = env->scheduler;
    scheduler->custom_data = (custom_scheduler_data_t *) calloc(1, sizeof(custom_scheduler_data_t));
    LF_ASSERT(scheduler->custom_data, "Out of memory");
    scheduler->custom_data->worker_states = (worker_states_t *) calloc(1, sizeof(worker_states_t));
    LF_ASSERT(scheduler->custom_data->worker_states, "Out of memory");
    scheduler->custom_data->worker_assignments = (worker_assignments_t *) calloc(1, sizeof(worker_assignments_t));
    LF_ASSERT(scheduler->custom_data->worker_assignments, "Out of memory");
    scheduler->custom_data->data_collection = (data_collection_t *) calloc(1, sizeof(data_collection_t));
    LF_ASSERT(scheduler->custom_data->data_collection, "Out of memory");

    worker_states_init(scheduler, number_of_workers);
    worker_assignments_init(scheduler, number_of_workers, params);
    
    data_collection_init(scheduler, params);
}

void lf_sched_free(lf_scheduler_t* scheduler) {
    worker_states_free(scheduler);
    worker_assignments_free(scheduler);
    data_collection_free(scheduler);
    free(scheduler->custom_data);
    lf_semaphore_destroy(scheduler->semaphore);
}

///////////////////////// Scheduler Worker API ///////////////////////////////

reaction_t* lf_sched_get_ready_reaction(lf_scheduler_t* scheduler, int worker_number) {
    assert(worker_number >= 0);
    reaction_t* ret;
    while (true) {
        size_t level_counter_snapshot = scheduler->custom_data->level_counter;
        ret = worker_assignments_get_or_lock(scheduler, worker_number);
        if (ret) return ret;
        if (worker_states_finished_with_level_locked(scheduler, worker_number)) {
            advance_level_and_unlock(scheduler, worker_number);
        } else {
            worker_states_sleep_and_unlock(scheduler, worker_number, level_counter_snapshot);
        }
        if (scheduler->custom_data->should_stop) {
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

void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number) {
    assert(worker_number >= -1);
    if (!lf_bool_compare_and_swap(&reaction->status, inactive, queued)) return;
    worker_assignments_put(scheduler, reaction);
}
#endif // defined SCHEDULER && SCHEDULER == SCHED_ADAPTIVE
