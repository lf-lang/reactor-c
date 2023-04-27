/**
 * @file
 * @author Benjamin Asch
 * @author Edward A. Lee
 * @copyright (c) 2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Definitions for watchdogs.
 */

#include "watchdog.h"

extern int _lf_watchdog_number;

/** 
 * Watchdog function type. The argument passed to one of 
 * these watchdog functions is a pointer to the self struct
 * for the reactor.
 */
typedef void(*watchdog_function_t)(void*);

void _lf_initialize_watchdog_mutexes() {
    for (int i = 0; i < _lf_watchdog_number; i++) {
        self_base_t* current_base = _lf_watchdogs[i]->base;
        if (&(current_base->watchdog_mutex) == NULL) {
            lf_mutex_init(&(current_base->watchdog_mutex));
            current_base->has_watchdog = true;
        }
    }
}

void* _lf_run_watchdog(void* arg) {
    watchdog_t* watchdog = (watchdog_t*)arg;

    self_base_t* base = watchdog->base;
    assert(base->reaction_mutex != NULL);
    lf_mutex_lock((lf_mutex*)base->reaction_mutex);

    while (lf_time_physical() < watchdog->expiration) {
        interval_t T = watchdog->expiration - lf_time_physical();
        lf_mutex_unlock((lf_mutex*)base->reaction_mutex);
        lf_sleep(T);
        lf_mutex_lock((lf_mutex*)base->reaction_mutex);
    }

    if (watchdog->expiration != NEVER) {
        watchdog_function_t watchdog_func = watchdog->watchdog_function;
        (*watchdog_func)(base);
    }
    watchdog->thread_active = false;

    lf_mutex_unlock((lf_mutex*)base->reaction_mutex);
    watchdog->thread_active = false;
    return NULL;
}

void lf_watchdog_start(watchdog_t* watchdog, interval_t additional_timeout) {
    // Assumes reaction mutex is already held.

    self_base_t* base = watchdog->base;

    watchdog->expiration = lf_time_logical() + watchdog->min_expiration + additional_timeout;

    if (!watchdog->thread_active) {
        lf_thread_create(&(watchdog->thread_id), _lf_run_watchdog, watchdog);
        watchdog->thread_active = true;
    } 
}

void lf_watchdog_stop(watchdog_t* watchdog) {
    watchdog->expiration = NEVER;
}
