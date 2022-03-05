
#include "scheduler.h"

static reaction_t**** reactions_by_worker_by_level;
static size_t** num_reactions_by_worker_by_level;
static size_t* max_num_workers_by_level;
static size_t* num_workers_by_level;
static size_t num_levels;
static size_t max_num_workers;

// The following apply to the current level.
static size_t current_level;
static size_t num_workers_busy;
static size_t* num_reactions_by_worker;
static reaction_t*** reactions_by_worker;
static size_t num_workers;

// A counter of the number of reactions triggered. No function should depend on the precise
// correctness of this value. Race conditions when accessing this value are acceptable.
static size_t reactions_triggered_counter = 0;

/**
 * @brief Set the level to be executed now. This function assumes that concurrent calls to it are
 * impossible.
 * 
 * @param level The new current level.
 */
static void set_level(size_t level) {
    assert(level < num_levels);
    assert(0 <= level);
    current_level = level;
    num_workers_busy = num_workers_by_level[level];
    num_reactions_by_worker = num_reactions_by_worker_by_level[level];
    reactions_by_worker = reactions_by_worker_by_level[level];
    num_workers = num_workers_by_level[level];
}

void worker_assignments_init(size_t number_of_workers, sched_params_t* params) {
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
        for (size_t worker = 0; worker < max_num_workers; worker++) {
            reactions_by_worker_by_level[level][worker] = (reaction_t**) malloc(
                sizeof(reaction_t*) * (1 + ((worker < num_workers) ? num_reactions : 0))
            );  // Warning: This wastes space.
            reactions_by_worker_by_level[level][worker][0] = NULL;
        }
    }
    set_level(0);
}

void worker_assignments_free() {
    for (size_t level = 0; level < num_levels; level++) {
        for (size_t worker = 0; worker < max_num_workers_by_level[level]; worker++) {
            free(reactions_by_worker_by_level[level][worker]);
        }
        free(reactions_by_worker_by_level[level]);
        free(num_reactions_by_worker_by_level[level]);
    }
    free(max_num_workers_by_level);
    free(num_workers_by_level);
}

/**
 * @brief Get a reaction for the given worker to execute. If no such reaction exists, consider the
 * given worker to be NOT BUSY. Workers that are not busy must not continue to request reactions.
 * 
 * @param worker A worker requesting work.
 * @return reaction_t* A reaction to execute, or NULL if no such reaction exists.
 */
reaction_t* worker_assignments_get(size_t worker) {
    assert(worker >= 0);
    // assert(worker < num_workers);  // There are edge cases where this doesn't hold.
    assert(num_reactions_by_worker[worker] >= 0);
    // The following is correct because of the NULL that precedes each reactions array. Note that it
    // temporarily leaves num_reactions_by_worker[worker] in an inconsistent state.
    // printf("DEBUG: %ld %ld\n", worker, num_reactions_by_worker[worker]);
    return reactions_by_worker[worker][num_reactions_by_worker[worker]--];
}

/**
 * @brief Record that worker is finished working on the current level.
 * 
 * @param worker The number of a worker.
 * @return true If this is the last worker to finish working on the current level.
 * @return false If at least one other worker is still working on the current level.
 */
bool worker_assignments_finished_with_level(size_t worker) {
    assert(worker >= 0);
    // assert(worker < num_workers);  // There are edge cases where this doesn't hold.
    assert(num_workers_busy > 0 || worker >= num_workers);
    num_reactions_by_worker[worker] = 0;
    bool ret = !lf_atomic_add_fetch(&num_workers_busy, -(worker < num_workers));
    return ret;
}

/**
 * @brief Trigger the given reaction.
 * 
 * @param reaction A reaction to be executed in the current tag.
 */
void worker_assignments_put(reaction_t* reaction) {
    size_t level = LEVEL(reaction->index);
    assert(level > current_level || current_level == 0);
    assert(level < num_levels);
    size_t worker = (reactions_triggered_counter++) % num_workers_by_level[level];
    assert(worker >= 0 && worker <= num_workers);
    size_t num_preceding_reactions_plus_one = lf_atomic_add_fetch(
        &num_reactions_by_worker_by_level[level][worker],
        1
    );
    printf("%p -> %ld @ %ld\n", reaction, worker, level);
    reactions_by_worker_by_level[level][worker][num_preceding_reactions_plus_one] = reaction;
}

/**
 * @brief Get the number of workers that should currently be working.
 * 
 * @return size_t The number of workers that should currently be working.
 */
size_t get_num_workers_busy() {
    return num_workers_busy;
}

/**
 * @brief Increment the level currently being processed by the workers.
 * 
 * @return true If the level was already at the maximum and was reset to zero.
 * @return false Otherwise.
 */
bool try_increment_level() {
    assert(num_workers_busy == 0);
    if (current_level + 1 == num_levels) {
        set_level(0);
        return true;
    }
    set_level(current_level + 1);
    return false;
}
