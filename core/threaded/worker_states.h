
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "scheduler.h"
#include "../platform.h"

static size_t num_awakened = 0;
static lf_cond_t* worker_conds;

static bool worker_states_sleep_forbidden = false;

extern size_t current_level;
extern size_t** num_reactions_by_worker_by_level;
extern size_t max_num_workers;
extern lf_mutex_t mutex;

/**
 * The level counter is a number that changes whenever the current level changes.
 *
 * This number must have a very long period in the sense that if it changes, the probability that it
 * is checked at a time in the future that is selected from some "reasonable" distribution, the
 * probability that it will have returned to the same value is vanishingly small.
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

void worker_states_init(size_t number_of_workers) {
    size_t greatest_worker_number = number_of_workers - 1;
    size_t num_conds = cond_of(greatest_worker_number) + 1;
    worker_conds = (lf_cond_t*) malloc(sizeof(lf_cond_t) * num_conds);
    for (int i = 0; i < num_conds; i++) {
        lf_cond_init(worker_conds + i);
    }
}

void worker_states_free() {
    // FIXME: Why do the condition variables and mutexes not need to be freed?
    free(worker_conds);
}

/**
 * @brief Awaken the workers scheduled to work on the current level.
 * 
 * @param num_to_awaken The number of workers to awaken.
 * @return A snapshot of the level counter after awakening the workers.
 */
size_t worker_states_awaken_locked(size_t num_to_awaken) {
    assert(num_to_awaken <= max_num_workers);
    num_awakened = num_to_awaken;
    size_t greatest_worker_number_to_awaken = num_to_awaken - 1;
    size_t max_cond = cond_of(greatest_worker_number_to_awaken);
    for (int cond = 0; cond <= max_cond; cond++) {
        printf("DEBUG: broadcasting to cond %d.\n", cond);
        lf_cond_broadcast(worker_conds + cond);
    }
    size_t ret = ++level_counter;
    return ret;
}

/**
 * @brief Wake up all workers and forbid them from ever sleeping again.
 * 
 * This is intended to coordinate shutdown (without leaving any dangling threads behind).
 */
void worker_states_never_sleep_again() {
    worker_states_sleep_forbidden = true;
    lf_mutex_lock(&mutex);
    worker_states_awaken_locked(max_num_workers);
    lf_mutex_unlock(&mutex);
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
void worker_states_sleep(size_t worker, size_t level_counter_snapshot) {
    // printf("DEBUG: worker=%ld\n", worker);
    assert(worker < max_num_workers);
    size_t cond = cond_of(worker);
    printf("DEBUG: %ld tries to get mutex so it can sleep.\n", worker);
    lf_mutex_lock(&mutex);
    printf("DEBUG: %ld has mutex and will try to sleep.\n", worker);
    if ((level_counter_snapshot == level_counter) & !worker_states_sleep_forbidden) {
        do {
            printf("DEBUG: %ld is going to sleep with cond %ld.\n", worker, cond);
            lf_cond_wait(worker_conds + cond, &mutex);
        } while (worker >= num_awakened);
        printf("DEBUG: %ld is awakened.\n", worker);
    }
    lf_mutex_unlock(&mutex);
}
