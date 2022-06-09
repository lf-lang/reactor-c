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
 * Assign reactions to workers.
 * @author{Peter Donovan <peterdonovan@berkeley.edu>}
 */

#ifndef WORKER_ASSIGNMENTS
#define WORKER_ASSIGNMENTS

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include "scheduler.h"

/** The queued reactions. */
static reaction_t**** reactions_by_worker_by_level;
/** The number of queued reactions currently assigned to each worker at each level. */
static size_t** num_reactions_by_worker_by_level;
/** The maximum number of workers that could possibly be kept simultaneously busy at each level. */
static size_t* max_num_workers_by_level;
/** The number of workers that will be used to execute each level. */
static size_t* num_workers_by_level;
/** The number of levels. */
static size_t num_levels;
/** The maximum number of workers that can be used to execute any level. */
static size_t max_num_workers;

/** The following values apply to the current level. */
static size_t current_level;
/** The number of reactions each worker still has to execute, indexed by worker. */
static size_t* num_reactions_by_worker;
/** The reactions to be executed, indexed by assigned worker. */
static reaction_t*** reactions_by_worker;
/** The total number of workers active, including those who have finished their work. */
static size_t num_workers;

#include "data_collection.h"

static void worker_states_lock(size_t worker);
static void worker_states_unlock(size_t worker);

/**
 * @brief Set the level to be executed now. This function assumes that concurrent calls to it are
 * impossible.
 * @param level The new current level.
 */
static void set_level(size_t level) {
    assert(level < num_levels);
    assert(0 <= level);
    data_collection_end_level(current_level, num_workers);
    current_level = level;
    num_reactions_by_worker = num_reactions_by_worker_by_level[level];
    reactions_by_worker = reactions_by_worker_by_level[level];
    num_workers = num_workers_by_level[level];
    // TODO: Experiment with not recording that the level is starting in the case that there is
    // nothing to execute. We need not optimize for the case when there is nothing to execute
    // because that case is not merely optimized, but is optimized out (we do not bother with
    // executing nothing).
    data_collection_start_level(current_level);
}

/** @brief Return the total number of reactions enqueued on the current level. */
static size_t get_num_reactions() {
    size_t total_num_reactions = 0;
    for (size_t i = 0; i < num_workers; i++) {
        total_num_reactions += num_reactions_by_worker[i];
    }
    // TODO: if num_workers was > total_num_reactions, report this to data_collection?
    return total_num_reactions;
}

static void worker_assignments_init(size_t number_of_workers, sched_params_t* params) {
    num_levels = params->num_reactions_per_level_size;
    max_num_workers = number_of_workers;
    reactions_by_worker_by_level = (reaction_t****) malloc(sizeof(reaction_t***) * num_levels);
    num_reactions_by_worker_by_level = (size_t**) malloc(sizeof(size_t*) * num_levels);
    num_workers_by_level = (size_t*) malloc(sizeof(size_t) * num_levels);
    max_num_workers_by_level = (size_t*) malloc(sizeof(size_t) * num_levels);
    for (size_t level = 0; level < num_levels; level++) {
        size_t num_reactions = params->num_reactions_per_level[level];
        size_t num_workers = num_reactions < max_num_workers ? num_reactions : max_num_workers;
        max_num_workers_by_level[level] = num_workers;
        num_workers_by_level[level] = max_num_workers_by_level[level];
        reactions_by_worker_by_level[level] = (reaction_t***) malloc(
            sizeof(reaction_t**) * max_num_workers
        );
        num_reactions_by_worker_by_level[level] = (size_t*) calloc(max_num_workers, sizeof(size_t));
        for (size_t worker = 0; worker < max_num_workers_by_level[level]; worker++) {
            reactions_by_worker_by_level[level][worker] = (reaction_t**) malloc(
                sizeof(reaction_t*) * num_reactions
            );  // Warning: This wastes space.
        }
    }
    data_collection_init(params);
    set_level(0);
}

static void worker_assignments_free() {
    for (size_t level = 0; level < num_levels; level++) {
        for (size_t worker = 0; worker < max_num_workers_by_level[level]; worker++) {
            free(reactions_by_worker_by_level[level][worker]);
        }
        free(reactions_by_worker_by_level[level]);
        free(num_reactions_by_worker_by_level[level]);
    }
    free(max_num_workers_by_level);
    free(num_workers_by_level);
    data_collection_free();
}

/**
 * @brief Return a reaction that has been assigned to the given worker, or NULL if no such reaction
 * exists.
 * @param worker The number of a worker needing work.
 */
static reaction_t* get_reaction(size_t worker) {
#ifndef FEDERATED
    int index = lf_atomic_add_fetch(num_reactions_by_worker + worker, -1);
    if (index >= 0) {
        return reactions_by_worker[worker][index];
    }
    num_reactions_by_worker[worker] = 0;
    return NULL;
#else
    // This is necessary for federated programs because reactions may be inserted into the current
    // level.
    int old_num_reactions;
    int current_num_reactions = num_reactions_by_worker[worker];
    int index;
    do {
        old_num_reactions = current_num_reactions;
        if (old_num_reactions <= 0) return NULL;
    } while (
        (current_num_reactions = lf_val_compare_and_swap(
            num_reactions_by_worker + worker,
            old_num_reactions,
            (index = old_num_reactions - 1)
        )) != old_num_reactions
    );
    return reactions_by_worker[worker][index];
#endif
}

/**
 * @brief Get a reaction for the given worker to execute. If no such reaction exists, claim the
 * mutex.
 * @param worker A worker requesting work.
 * @return reaction_t* A reaction to execute, or NULL if no such reaction exists.
 */
static reaction_t* worker_assignments_get_or_lock(size_t worker) {
    assert(worker >= 0);
    // assert(worker < num_workers);  // There are edge cases where this doesn't hold.
    assert(num_reactions_by_worker[worker] >= 0);
    reaction_t* ret;
    while (true) {
        if ((ret = get_reaction(worker))) return ret;
        if (worker < num_workers) {
            for (
                size_t victim = (worker + 1) % num_workers;
                victim != worker;
                victim = (victim + 1) % num_workers
            ) {
                if ((ret = get_reaction(victim))) return ret;
            }
        }
        worker_states_lock(worker);
        if (!num_reactions_by_worker[worker]) {
            return NULL;
        }
        worker_states_unlock(worker);
    }
}

/**
 * @brief Trigger the given reaction.
 * @param reaction A reaction to be executed in the current tag.
 */
static void worker_assignments_put(reaction_t* reaction) {
    size_t level = LEVEL(reaction->index);
    assert(reaction != NULL);
#ifndef FEDERATED
    assert(level > current_level || current_level == 0);
#endif
    assert(level < num_levels);
    // Source: https://xorshift.di.unimi.it/splitmix64.c
    // TODO: This is probably not the most efficient way to get the randomness that we need because
    // it is designed to give an entire word of randomness, whereas we only need
    // ~log2(num_workers_by_level[level]) bits of randomness.
    uint64_t hash = (uint64_t) reaction;
    hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9;
    hash = (hash ^ (hash >> 27)) * 0x94d049bb133111eb;
    hash = hash ^ (hash >> 31);
    size_t worker = hash % num_workers_by_level[level];
    size_t num_preceding_reactions = lf_atomic_fetch_add(
        &num_reactions_by_worker_by_level[level][worker],
        1
    );
    reactions_by_worker_by_level[level][worker][num_preceding_reactions] = reaction;
}

#endif
