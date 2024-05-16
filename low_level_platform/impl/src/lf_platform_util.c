#include "low_level_platform.h"
#include "platform/lf_platform_util.h"

int map_value(int value, int src_min, int src_max, int dest_min, int dest_max) {
  // Perform the linear mapping. Since we are working with integers, it is
  // important to multiply before we divide
  return dest_min + (((value - src_min) * (dest_max - dest_min)) / (src_max - src_min));
}

#ifndef PLATFORM_ZEPHYR // on Zephyr, this is handled separately
#ifndef LF_SINGLE_THREADED
static int _lf_worker_thread_count = 0;

static thread_local int lf_thread_id_var = -1;

int lf_thread_id() { return lf_thread_id_var; }

void initialize_lf_thread_id() { lf_thread_id_var = lf_atomic_fetch_add32(&_lf_worker_thread_count, 1); }
#endif
#endif
