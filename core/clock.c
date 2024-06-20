/**
 * @file
 * @author Erling Jellum
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Implementations of functions in clock.h.
 */
#include "clock.h"
#include "low_level_platform.h"

// If we are federated, include clock-sync API (and implementation)
#if defined(FEDERATED)
#include "clock-sync.h"
#else
// In the unfederated case, just provide empty implementations.
void clock_sync_add_offset(instant_t* t) { (void)t; }
void clock_sync_subtract_offset(instant_t* t) { (void)t; }
#endif // defined(FEDERATED)

static instant_t last_read_physical_time = NEVER;

int lf_clock_gettime(instant_t* now) {
  instant_t last_read_local;
  int res = _lf_clock_gettime(now);
  if (res != 0) {
    return -1;
  }
  clock_sync_add_offset(now);
  do {
    // Atomically fetch the last read value. This is done with
    // atomics to guarantee that it works on 32bit platforms as well.
    last_read_local = lf_atomic_fetch_add64(&last_read_physical_time, 0);

    // Ensure monotonicity.
    if (*now < last_read_local) {
      *now = last_read_local + 1;
    }

    // Update the last read value, atomically and also make sure that another
    // thread has not been here in between and changed it. If so. We must redo
    // the monotonicity calculation.
  } while (!lf_atomic_bool_compare_and_swap64(&last_read_physical_time, last_read_local, *now));

  return 0;
}

int lf_clock_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
  // Remove any clock sync offset and call the Platform API.
  clock_sync_subtract_offset(&wakeup_time);
  return _lf_interruptable_sleep_until_locked(env, wakeup_time);
}

#if !defined(LF_SINGLE_THREADED)
int lf_clock_cond_timedwait(lf_cond_t* cond, instant_t wakeup_time) {
  // Remove any clock sync offset and call the Platform API.
  clock_sync_subtract_offset(&wakeup_time);
  return _lf_cond_timedwait(cond, wakeup_time);
}
#endif // !defined(LF_SINGLE_THREADED)
