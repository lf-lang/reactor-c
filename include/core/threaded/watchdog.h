/**
 * @file watchdog.h
 * @author Benjamin Asch
 * @author Edward A. Lee
 *
 * @brief Declarations for watchdogs.
 * @ingroup Internal
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H 1

#include "lf_types.h"
#include "environment.h"
#include "platform.h" // For lf_thread_t.

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Watchdog function type.
 * @ingroup Internal
 *
 * The argument passed to one of these watchdog functions is a pointer to the self
 * struct for the reactor.
 */
typedef void (*watchdog_function_t)(void*);

/**
 * @brief Typdef for watchdog_t struct, used to call watchdog handler.
 * @ingroup Internal
 */
typedef struct watchdog_t {
  /**
   * @brief Pointer to the reactor that contains this watchdog.
   *
   * Points to the self_base_t struct of the reactor instance that owns this watchdog.
   */
  struct self_base_t* base;

  /**
   * @brief The trigger associated with this watchdog.
   *
   * Used to schedule watchdog expiration events and manage the watchdog's lifecycle.
   */
  trigger_t* trigger;

  /**
   * @brief The expiration instant for the watchdog.
   *
   * The logical time at which the watchdog will expire. Initialized to NEVER
   * and updated when the watchdog is started or restarted.
   */
  instant_t expiration;

  /**
   * @brief The minimum expiration interval for the watchdog.
   *
   * The minimum time that must elapse before the watchdog can expire.
   * This is added to the current logical time when starting the watchdog.
   */
  interval_t min_expiration;

  /**
   * @brief The thread ID where the watchdog handler should run.
   *
   * Identifies the thread that should execute the watchdog's handler function
   * when the watchdog expires.
   */
  lf_thread_t thread_id;

  /**
   * @brief Condition variable for thread synchronization.
   *
   * Used to coordinate the watchdog thread's sleep and wake operations,
   * and to handle termination signals.
   */
  lf_cond_t cond;

  /**
   * @brief Indicates whether the watchdog thread is currently active.
   *
   * When true, the watchdog thread is running or waiting for expiration.
   * When false, the watchdog thread is inactive (stopped or terminated).
   */
  bool active;

  /**
   * @brief Indicates whether watchdog thread termination has been requested.
   *
   * When true, the watchdog thread should terminate. Used to coordinate
   * graceful shutdown of the watchdog thread.
   */
  bool terminate;

  /**
   * @brief The watchdog handler function.
   *
   * Function pointer to the handler that will be called when the watchdog
   * expires. The handler receives a pointer to the reactor's self struct.
   */
  watchdog_function_t watchdog_function;
} watchdog_t;

/**
 * @brief Start or restart the watchdog timer.
 * @ingroup API
 *
 * This function sets the expiration time of the watchdog to the current logical time
 * plus the minimum timeout of the watchdog plus the specified `additional_timeout`.
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
 * @ingroup API
 *
 * This function sets the expiration time of the watchdog to `NEVER`.
 *
 * @param watchdog The watchdog.
 */
void lf_watchdog_stop(watchdog_t* watchdog);

///////////////////// Internal functions /////////////////////
// The following functions are internal to the runtime and should not be documented by Doxygen.

/**
 * @brief Function to initialize mutexes for watchdogs
 * @ingroup Internal
 *
 * This function is used to initialize the mutexes for the watchdogs.
 *
 * @param env The environment to initialize the watchdogs for.
 */
void _lf_initialize_watchdogs(environment_t* env);

/**
 * @brief Terminates all watchdogs inside the environment.
 * @ingroup Internal
 *
 * This function is used to terminate all the watchdogs inside the environment.
 *
 * @param env The environment to terminate the watchdogs for.
 */
void _lf_watchdog_terminate_all(environment_t* env);

#ifdef __cplusplus
}
#endif

#endif
