
#ifndef WORKER_ASSIGNMENTS
#define WORKER_ASSIGNMENTS

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include "scheduler.h"

static reaction_t**** reactions_by_worker_by_level;
static size_t** num_reactions_by_worker_by_level;
static size_t* max_num_workers_by_level;
static size_t* num_workers_by_level;
static size_t num_levels;
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
 * 
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
    data_collection_start_level(current_level);
}
/** Return the total number of reactions enqueued on the current level. */
static size_t get_num_reactions() {
    size_t total_num_reactions = 0;
    for (size_t i = 0; i < num_workers; i++) {
        total_num_reactions += num_reactions_by_worker[i];
    }
    // TODO: if num_workers was > total_num_reactions, report this to data_collection
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

static reaction_t* get_reaction(size_t worker) {
    int index = lf_atomic_add_fetch(num_reactions_by_worker + worker, -1);
    if (index >= 0) {
        return reactions_by_worker[worker][index];
    }
    num_reactions_by_worker[worker] = 0;
    return NULL;
}

/**
 * @brief Get a reaction for the given worker to execute. If no such reaction exists, claim the
 * mutex.
 *
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
            for (size_t victim = (worker + 1) % num_workers; victim != worker; victim = (victim + 1) % num_workers) {
                if ((ret = get_reaction(victim))) return ret;
            }
        }
        // printf("%ld <- %p @ %lld\n", worker, ret, LEVEL(ret->index));
        worker_states_lock(worker);
        if (!num_reactions_by_worker[worker]) {
            return NULL;
        }
        worker_states_unlock(worker);
    }
}

/**
 * @brief Trigger the given reaction.
 * 
 * @param reaction A reaction to be executed in the current tag.
 */
static void worker_assignments_put(reaction_t* reaction) {
    size_t level = LEVEL(reaction->index);
    assert(reaction != NULL);
    assert(level > current_level || current_level == 0);
    assert(level < num_levels);
    // TODO: Find some robust way to compute a hash from the reaction. This method here is wrong.
    size_t worker = ((size_t) reaction) % num_workers_by_level[level];
    size_t num_preceding_reactions = lf_atomic_fetch_add(
        &num_reactions_by_worker_by_level[level][worker],
        1
    );
    // printf("%p -> %ld @ %ld[%ld]\n", reaction, worker, level, num_preceding_reactions);
    reactions_by_worker_by_level[level][worker][num_preceding_reactions] = reaction;
}

#endif
