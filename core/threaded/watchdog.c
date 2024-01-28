/**
 * @file
 * @author Benjamin Asch
 * @author Edward A. Lee
 * @author Erling Jellum
 * @copyright (c) 2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Definitions for watchdogs.
 */

#include <assert.h>
#include "watchdog.h"
#include "environment.h"
#include "util.h"

/**
 * @brief Initialize watchdog mutexes.
 * For any reactor with one or more watchdogs, the self struct should have a non-NULL
 * `reactor_mutex` field which points to an instance of `lf_mutex_t`.
 * This function initializes those mutexes. It also initializes the condition
 * variable which enables the safe termination of a running watchdog.
 */
void _lf_initialize_watchdog_mutexes(environment_t *env) {
    int ret;            
    for (int i = 0; i < env->watchdogs_size; i++) {
        watchdog_t *watchdog = env->watchdogs[i];
        if (watchdog->base->reactor_mutex != NULL) {
            ret = lf_mutex_init((lf_mutex_t*)(watchdog->base->reactor_mutex));
            LF_ASSERTN(ret, "lf_mutex_init failed");
        }
        ret = lf_cond_init(&watchdog->cond, watchdog->base->reactor_mutex);
        LF_ASSERTN(ret, "lf_cond_init failed");
    }
}

/**
 * @brief Terminate all watchdog threads. 
 */
void _lf_watchdog_terminate(environment_t *env) {
    void *thread_return;
    int ret;
    for (int i = 0; i < env->watchdogs_size; i++) {
        watchdog_t *watchdog = env->watchdogs[i];
        if (watchdog->thread_active) {
            ret = lf_mutex_lock(watchdog->base->reactor_mutex);
            LF_ASSERTN(ret, "lf_mutex_lock failed");
            lf_watchdog_stop(watchdog);
            ret = lf_mutex_unlock(watchdog->base->reactor_mutex);
            LF_ASSERTN(ret, "lf_mutex_unlock failed");
            ret = lf_thread_join(watchdog->thread_id, &thread_return);
            LF_ASSERTN(ret, "lf_thread_join failed");
        }
    }
}

/**
 * @brief Thread function for watchdog.
 * Each watchdog has a thread which sleeps until one out of two scenarios:
 * 1) The watchdog timeout expires and there has not been a renewal of the watchdog budget.
 * 2) The watchdog is signaled to wake up and terminate.
 * In normal usage, the expiration time is incremented while the thread is
 * sleeping, so when the thread wakes up, it can go back to sleep again.
 * If the watchdog does expire. It will execute the watchdog handler and the 
 * thread will terminate. To stop the watchdog, another thread will signal the
 * condition variable, in that case the watchdog thread will terminate directly.
 * The expiration field of the watchdog is used to protect against race conditions.
 * It is set to NEVER when the watchdog is terminated.
 * 
 * @param arg A pointer to the watchdog struct
 * @return NULL
 */
void* _lf_run_watchdog(void* arg) {
    int ret;
    watchdog_t* watchdog = (watchdog_t*)arg;
    LF_PRINT_DEBUG("Starting Watchdog %p", watchdog);

    self_base_t* base = watchdog->base;
    LF_ASSERT(base->reactor_mutex, "reactor-mutex not alloc'ed but has watchdogs.");
    ret = lf_mutex_lock((lf_mutex_t*)(base->reactor_mutex));
    LF_ASSERTN(ret, "lf_mutex_lock failed");
    instant_t physical_time = lf_time_physical();
    while (watchdog->expiration != NEVER && physical_time < watchdog->expiration) {
        interval_t T = watchdog->expiration - physical_time;
        LF_PRINT_DEBUG("Watchdog %p going to sleep", watchdog);
        // Wait for expiration, or a signal to terminate
        if(lf_cond_timedwait(&watchdog->cond, watchdog->expiration) != LF_TIMEOUT) {
            LF_PRINT_DEBUG("Watchdog %p was cancelled. Terminating", watchdog);
            goto terminate;
        }
        LF_PRINT_DEBUG("Watchdog %p woke up from sleep. Checking for timeout", watchdog);
        physical_time = lf_time_physical();
    }
    // If we get here then we either have a time-out or expiration is NEVER.

    if (watchdog->expiration != NEVER) {
        // At this point we have a timeout.
        LF_PRINT_DEBUG("Watchdog %p timed out", watchdog);
        watchdog_function_t watchdog_func = watchdog->watchdog_function;
        (*watchdog_func)(base);
    }

terminate:
    // Here the thread terminates. 
    watchdog->thread_active = false;
    ret = lf_mutex_unlock(base->reactor_mutex);
    LF_ASSERTN(ret, "lf_mutex_unlock failed");
    return NULL;
}

void lf_watchdog_start(watchdog_t* watchdog, interval_t additional_timeout) {
    int ret;
    // Assumes reactor mutex is already held.

    self_base_t* base = watchdog->base;
    watchdog->expiration = base->environment->current_tag.time + watchdog->min_expiration + additional_timeout;

    // If we dont have a running watchdog thread.
    if (!watchdog->thread_active) {
        ret = lf_thread_create(&(watchdog->thread_id), _lf_run_watchdog, watchdog);
        LF_ASSERTN(ret, "lf_thread_create failed");
        watchdog->thread_active = true;
    } 
}

void lf_watchdog_stop(watchdog_t* watchdog) {
    int ret;
    // Assumes reactor mutex is already held.
    if (!watchdog->thread_active)
        return;
    watchdog->expiration = NEVER;
    ret = lf_cond_signal(&watchdog->cond);
    LF_ASSERTN(ret, "lf_conf_signal failed");
}
