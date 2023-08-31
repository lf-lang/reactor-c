/**
 * @file
 * @author Benjamin Asch
 * @author Edward A. Lee
 * @copyright (c) 2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Declarations for watchdogs.
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H 1

#include "lf_types.h"
#include "environment.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Watchdog function type. The argument passed to one of 
 * these watchdog functions is a pointer to the self struct
 * for the reactor.
 */
typedef void(*watchdog_function_t)(void*);

/** Typdef for watchdog_t struct, used to call watchdog handler. */
typedef struct watchdog_t watchdog_t;

/** Watchdog struct for handler. */
struct watchdog_t {
    struct self_base_t* base;               // The reactor that contains the watchdog.
    trigger_t* trigger;                     // The trigger associated with this watchdog.
    instant_t expiration;                   // The expiration instant for the watchdog. (Initialized to NEVER)
    interval_t min_expiration;              // The minimum expiration interval for the watchdog.
    lf_thread_t thread_id;                  // The thread that the watchdog is meant to run on.
    bool thread_active;                     // Boolean indicating whether or not thread is active.  
    watchdog_function_t watchdog_function;  // The function/handler for the watchdog.
};

/** 
 * @brief Start or restart the watchdog timer.
 * This function sets the expiration time of the watchdog to the current logical time
 * plus the minimum timeout of the watchdog plus the specified `additional_timeout`.
 * If a watchdog timer thread is not already running, then this function will start one.
 * This function assumes the reactor mutex is held when it is called; this assumption
 * is satisfied whenever this function is called from within a reaction that declares
 * the watchdog as an effect.
 * 
 * @param watchdog The watchdog to be started
 * @param additional_timeout Additional timeout to be added to the watchdog's
 * minimum expiration.
 */
void lf_watchdog_start(watchdog_t* watchdog, interval_t additional_timeout);

/**
 * @brief Stop the specified watchdog without invoking the expiration handler.
 * This function sets the expiration time of the watchdog to `NEVER`.
 * 
 * @param watchdog The watchdog.
 */
void lf_watchdog_stop(watchdog_t* watchdog);

#ifdef __cplusplus
}
#endif

#endif
