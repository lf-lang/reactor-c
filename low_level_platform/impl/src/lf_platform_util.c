#include "low_level_platform.h"
#include "platform/lf_platform_util.h"

int map_priorities(int priority, int dest_min, int dest_max) {
  // Check if priority is within the legal range
  if (priority < LF_SCHED_MIN_PRIORITY || priority > LF_SCHED_MAX_PRIORITY) {
    return -1;
  }

  // Perform the linear mapping. Since we are working with integers, it is
  // important to multiply before we divide
  return dest_min + (((priority - LF_SCHED_MIN_PRIORITY) * (dest_max - dest_min)) /
                     (LF_SCHED_MAX_PRIORITY - LF_SCHED_MIN_PRIORITY));
}

#ifndef PLATFORM_ZEPHYR // on Zephyr, this is handled separately
#ifndef LF_SINGLE_THREADED
static int _lf_worker_thread_count = 0;

static thread_local int lf_thread_id_var = -1;

int lf_thread_id() { return lf_thread_id_var; }

void initialize_lf_thread_id() { lf_thread_id_var = lf_atomic_fetch_add(&_lf_worker_thread_count, 1); }
#endif
#endif
