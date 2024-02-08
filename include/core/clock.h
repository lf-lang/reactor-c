/**
 * @file
 * @author Erling Rennemo Jellum
 * @copyright (c) 2024
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief A higher level API to the clock utilities provided by the platform API.
 * It builds ontop of the clocking API of the different platforms and ensures:
 * 1. Monotonicity
 * 2. That clock synchronization offsets are applied and removed.
 */

#ifndef CLOCK_H
#define CLOCK_H

#include "platform.h"

/**
 * This will block the calling thread until wakeup_time is reached or it is
 * interrupted by an asynchronous scheduling. Used by the single-threaded
 * runtime. Before calling the appropriate function in the platform API, the
 * wakeup_time will be translated into the correct timescale by removing any
 * clock synchronization offset.

 * @return 0 on success or -1 if interrupted.
 */
int lf_clock_interruptable_sleep_until_locked(environment_t *env, instant_t wakeup_time);


/**
 * Retrives the current physical time from the platform API. It adjusts for
 * clock synchronization and guaranteed monotonicity.
 * @param now 
 * @return 0 on success. 
 */
int lf_clock_gettime(instant_t *now);

#if !defined(LF_SINGLE_THREADED)
/**
 * This will block the calling thread on the condition variable until it is
 * signaled or until wakeup_time is reached. Before calling the appropriate
 * function in the platform API, the wakeup_time will be translated into the
 * correct timescale by removing any clock synchronization offset.

 * @return 0 on success, LF_TIMEOUT on timeout, platform-specific error
 * otherwise.
 */
int lf_clock_cond_timedwait(lf_cond_t *cond, instant_t wakeup_time);
#endif

#endif
