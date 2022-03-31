
#ifndef WORKER_STATES
#define WORKER_STATES

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include "scheduler.h"
#include "../platform.h"

static lf_cond_t* worker_conds;
static size_t* cumsum_of_cond_of;

static bool worker_states_sleep_forbidden = false;

extern size_t current_level;
extern size_t** num_reactions_by_worker_by_level;
extern size_t max_num_workers;
extern lf_mutex_t mutex;

/** The number of non-waiting threads. */
static volatile size_t num_loose_threads;
static volatile size_t num_awakened;
/** Whether the mutex is held by each worker via this module's API. */
static bool* mutex_held;

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
    cumsum_of_cond_of = (size_t*) calloc(num_conds, sizeof(size_t));
    mutex_held = (bool*) malloc(sizeof(bool) * number_of_workers);
    for (int i = 0; i < number_of_workers; i++) {
        cumsum_of_cond_of[cond_of(i)]++;
    }
    for (int i = 1; i < num_conds; i++) {
        cumsum_of_cond_of[i] += cumsum_of_cond_of[i - 1];
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
    // printf("%ld +-> %ld\n", worker, num_to_awaken);
    if (!mutex_held[worker]) {
        mutex_held[worker] = true;
        lf_mutex_lock(&mutex);
    }
    num_loose_threads = cumsum_of_cond_of[max_cond];
    num_loose_threads += worker >= num_loose_threads;
    num_awakened = num_loose_threads;
    level_counter++;
    for (int cond = 0; cond <= max_cond; cond++) {
        lf_cond_broadcast(worker_conds + cond);
    }
}

/**
 * @brief Wake up all workers and forbid them from ever sleeping again.
 * 
 * This is intended to coordinate shutdown (without leaving any dangling threads behind).
 */
static void worker_states_never_sleep_again(size_t worker) {
    worker_states_sleep_forbidden = true;
    lf_mutex_lock(&mutex);
    worker_states_awaken_locked(worker, max_num_workers);
    lf_mutex_unlock(&mutex);
}

/** Lock the global mutex if needed. */
static void worker_states_lock(size_t worker) {
    assert(num_loose_threads > 0);
    assert(num_loose_threads <= max_num_workers);
    size_t lt = num_loose_threads;
    // printf("%ld sees %ld loose threads\n", worker, lt);
    if (lt > 1 || !fast) {  // FIXME: Lock should be partially optimized out even when !fast
        // printf("%ld locking mutex.\n", worker);
        lf_mutex_lock(&mutex);
        // printf("%ld locked mutex.\n", worker);
        assert(mutex_held[worker] == false);
        mutex_held[worker] = true;
    } else {
        // printf("%ld not locking mutex.\n", worker);
    }
}

/** Unlock the global mutex if needed. */
static void worker_states_unlock(size_t worker) {
    if (mutex_held[worker]) {
        // printf("%ld unlocking mutex.\n", worker);
        mutex_held[worker] = false;
        lf_mutex_unlock(&mutex);
    } else {
        // printf("%ld not unlocking mutex.\n", worker);
    }
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
    // shortcut, and it wasn't perfect.
    size_t ret = lf_atomic_add_fetch(&num_loose_threads, -1);
    assert(ret >= 0);
    // printf("worker=%ld, nlt=%ld\n", worker, ret);
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
    // printf("%ld sleep; nlt=%ld\n", worker, num_loose_threads);
    size_t cond = cond_of(worker);
    if ((level_counter_snapshot == level_counter) & !worker_states_sleep_forbidden) {
        do {
            lf_cond_wait(worker_conds + cond, &mutex);
        } while (level_counter_snapshot == level_counter || worker >= num_awakened);
    }
    // printf("%ld wake; nlt=%ld\n", worker, num_loose_threads);
    assert(mutex_held[worker] == false);  // This thread holds the mutex, but it did not report that.
    lf_mutex_unlock(&mutex);
}

#endif
