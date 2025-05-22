/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief MacOS API support for the C target of Lingua Franca.
 */

#ifdef PLATFORM_Darwin
/* MacOS API support for the C target of Lingua Franca. */

#include "platform/lf_macos_support.h"
#include "low_level_platform.h"
#include "tag.h"

#if defined LF_SINGLE_THREADED
#include "lf_os_single_threaded_support.c"
#else
#include "lf_POSIX_threads_support.c"

/**
 * Real-time scheduling API not implemented for macOS.
 */
int lf_thread_set_cpu(lf_thread_t thread, size_t cpu_number) { return -1; }

int lf_thread_set_priority(lf_thread_t thread, int priority) { return -1; }

int lf_thread_set_scheduling_policy(lf_thread_t thread, lf_scheduling_policy_t* policy) { return -1; }
#endif

#include "platform/lf_unix_clock_support.h"

// See `man 2 clock_nanosleep` for return values
int lf_sleep(interval_t sleep_duration) {
  const struct timespec tp = convert_ns_to_timespec(sleep_duration);
  struct timespec remaining;
  return nanosleep((const struct timespec*)&tp, (struct timespec*)&remaining);
}

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
  (void)env;
  interval_t sleep_duration = wakeup_time - lf_time_physical();

  if (sleep_duration <= 0) {
    return 0;
  } else {
    return lf_sleep(sleep_duration);
  }
}

int lf_nanosleep(interval_t sleep_duration) { return lf_sleep(sleep_duration); }
#endif
