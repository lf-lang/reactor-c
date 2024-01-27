/**
 * @file
 * @author Benjamin Asch
 * @author Edward A. Lee
 * @copyright (c) 2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Definitions for watchdogs.
 */

#include <assert.h>
#include "watchdog.h"

extern int _lf_watchdog_count;
extern watchdog_t* _lf_watchdogs;

/**
 * @brief Initialize watchdog mutexes.
 * For any reactor with one or more watchdogs, the self struct should have a non-NULL
 * `reactor_mutex` field which points to an instance of `lf_mutex_t`.
 * This function initializes those mutexes.
 */
void _lf_initialize_watchdog_mutexes() {
    for (int i = 0; i < _lf_watchdog_count; i++) {
        watchdog_t * watchdog = &_lf_watchdogs[i];
        lf_mutex_init(&watchdog->lock);
        lf_cond_init(&watchdog->cond, &watchdog->lock);
        if (watchdog->base.reactor_mutex != NULL) {
            lf_mutex_init((lf_mutex_t*)(current_base->reactor_mutex));
        }
    }
}

/**
 * @brief Thread function for watchdog.
 * This function sleeps until physical time exceeds the expiration time of
 * the watchdog and then invokes the watchdog expiration handler function.
 * In normal usage, the expiration time is incremented while the thread is
 * sleeping, so the watchdog never expires and the handler function is never
 * invoked.
 * This function acquires the reaction mutex and releases it while sleeping.
 * The thread sleeps by waiting on a condition variable. This allows for other
 * threads terminating the watchdog safely. 
 * 
 * @param arg A pointer to the watchdog struct
 * @return NULL
 */
void* _lf_run_watchdog(void* arg) {
    watchdog_t* watchdog = (watchdog_t*)arg;

    // Acquire the global watchdog mutex. This is just used for coordinating
    // the graceful shutdown of the watchdog
    lf_mutex_lock(&watchdog->mutex);

    self_base_t* base = watchdog->base;
    assert(base->reactor_mutex != NULL);
    lf_mutex_lock((lf_mutex_t*)(base->reactor_mutex));
    instant_t physical_time = lf_time_physical();
    while (physical_time < watchdog->expiration) {
        interval_t T = watchdog->expiration - physical_time;
        lf_mutex_unlock((lf_mutex_t*)base->reactor_mutex);

        // Wait for expiration, or a signal to terminate
        if(lf_cond_timedwait(&watchdog->cond, watchdog->expiration) != LF_TIMEOUT) {
            // We got a signal to terminate. Release the mutex associated with
            // the watchdog and terminate the thread.
            lf_mutex_unlock(&watchdog->mutex);
            watchdog->thread_active = false;
            return NULL;
        }
        // We had a timeout. Lock the reactor-mutex and verify that we havent
        // received a new budget.
        lf_mutex_lock((lf_mutex_t*)(base->reactor_mutex));
        physical_time = lf_time_physical();
    }

    // At this point we have a timeout.
    watchdog_function_t watchdog_func = watchdog->watchdog_function;
    (*watchdog_func)(base);
  
    // Here the thread terminates. 
    watchdog->thread_active = false;
    lf_mutex_unlock(watchdog->mutex)
    return NULL;
}

void lf_watchdog_start(watchdog_t* watchdog, interval_t additional_timeout) {
    // Assumes reaction mutex is already held.

    self_base_t* base = watchdog->base;

    watchdog->expiration = base->environment->current_tag.time + watchdog->min_expiration + additional_timeout;

    // If we dont have a running watchdog thread.
    if (!watchdog->thread_active) {
        lf_thread_create(&(watchdog->thread_id), _lf_run_watchdog, watchdog);
        watchdog->thread_active = true;
    } 
}

void lf_watchdog_stop(watchdog_t* watchdog) {
    if (!watchdog->thread_active)
        return;
    
    lf_mutex_lock(watchdog->lock);
    lf_cond_signal(watchdog->cond);
    lf_mutex_unlock(watchdog->lock);
    void *thread_return;
    lf_thread_join(watchdog->thread_id, &thread_return);
}
