/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Marten Lohstroh
 * @author Erling Jellum
 *
 * @brief Platform support for the Linux operating system.
 */

#ifdef PLATFORM_Linux
/* MacOS API support for the C target of Lingua Franca. */

#define _GNU_SOURCE // Needed to get access to Linux thread-scheduling API
#include "platform/lf_linux_support.h"
#include "platform/lf_platform_util.h"
#include "low_level_platform.h"

#include "platform/lf_unix_clock_support.h"

#if defined LF_SINGLE_THREADED
#include "lf_os_single_threaded_support.c"
#else
#include "lf_POSIX_threads_support.c"

int lf_thread_set_cpu(lf_thread_t thread, size_t cpu_number) {
  // Create a CPU-set consisting of only the desired CPU
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu_number, &cpu_set);

  return pthread_setaffinity_np(thread, sizeof(cpu_set), &cpu_set);
}

int lf_thread_set_priority(lf_thread_t thread, int priority) {
  int posix_policy, min_pri, max_pri, final_priority, res;
  struct sched_param schedparam;

  if (priority > LF_SCHED_MAX_PRIORITY || priority < LF_SCHED_MIN_PRIORITY) {
    return -1;
  }

  // Get the current scheduling policy
  res = pthread_getschedparam(thread, &posix_policy, &schedparam);
  if (res != 0) {
    return res;
  }

  min_pri = sched_get_priority_min(posix_policy);
  max_pri = sched_get_priority_max(posix_policy);
  if (min_pri == -1 || max_pri == -1) {
    return -1;
  }

  final_priority = map_priorities(priority, min_pri, max_pri);
  if (final_priority < 0) {
    return -1;
  }

  return pthread_setschedprio(thread, final_priority);
}

int lf_thread_set_scheduling_policy(lf_thread_t thread, lf_scheduling_policy_t* policy) {
  int posix_policy, res;
  bool set_priority;
  struct sched_param schedparam;

  // Get the current scheduling policy
  res = pthread_getschedparam(thread, &posix_policy, &schedparam);
  if (res != 0) {
    return res;
  }

  // Update the policy, and initially set the priority to max.
  // The priority value is later updated. Initializing it
  // is just to avoid code duplication.
  switch (policy->policy) {
  case LF_SCHED_FAIR:
    posix_policy = SCHED_OTHER;
    schedparam.sched_priority = 0;
    set_priority = false;
    break;
  case LF_SCHED_TIMESLICE:
    posix_policy = SCHED_RR;
    schedparam.sched_priority = sched_get_priority_max(SCHED_RR);
    set_priority = true;
    break;
  case LF_SCHED_PRIORITY:
    posix_policy = SCHED_FIFO;
    schedparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
    set_priority = true;
    break;
  default:
    return -1;
    break;
  }

  // Write it back
  res = pthread_setschedparam(thread, posix_policy, &schedparam);
  if (res != 0) {
    return res;
  }

  // Set the priority of we chose a RT scheduler
  if (set_priority) {
    res = lf_thread_set_priority(thread, policy->priority);
    if (res != 0) {
      return res;
    }
  }

  return 0;
}
#endif

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
