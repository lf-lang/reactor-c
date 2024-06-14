#ifdef PLATFORM_Darwin
/* MacOS API support for the C target of Lingua Franca. */

/*************
Copyright (c) 2021, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/** MacOS API support for the C target of Lingua Franca.
 *
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 */

#define _GNU_SOURCE
#include "lf_macos_support.h"
#include "platform.h"
#include "tag.h"

#if defined LF_SINGLE_THREADED
#include "lf_os_single_threaded_support.c"
#else
#include "lf_POSIX_threads_support.c"

int lf_thread_set_cpu(lf_thread_t thread, int cpu_number) {
    return -1;
}

int lf_thread_set_priority(lf_thread_t thread, int priority) {
    return -1;
}

int lf_thread_set_scheduling_policy(lf_thread_t thread, lf_scheduling_policy_t *policy) {
    return -1;
}
#endif

#include "lf_unix_clock_support.h"

// See `man 2 clock_nanosleep` for return values
int lf_sleep(interval_t sleep_duration) {
    const struct timespec tp = convert_ns_to_timespec(sleep_duration);
    struct timespec remaining;
    return nanosleep((const struct timespec*)&tp, (struct timespec*)&remaining);
}

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
    interval_t sleep_duration = wakeup_time - lf_time_physical();

    if (sleep_duration <= 0) {
        return 0;
    } else {
        return lf_sleep(sleep_duration);
    }
}

int lf_nanosleep(interval_t sleep_duration) {
    return lf_sleep(sleep_duration);
}
#endif
