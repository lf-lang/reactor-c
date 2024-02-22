#include "low_level_platform.h"

#ifndef PLATFORM_ZEPHYR  // on Zephyr, this is handled separately
#ifndef LF_SINGLE_THREADED
static int _lf_worker_thread_count = 0;

static thread_local int lf_thread_id_var = -1;

int lf_thread_id() {
    return lf_thread_id_var;
}

void initialize_lf_thread_id() {
    lf_thread_id_var = lf_atomic_fetch_add32(&_lf_worker_thread_count, 1);
}
#endif
#endif
