#if defined(PLATFORM_ZEPHYR)
#include "platform/lf_zephyr_board_support.h"
#if !defined(LF_ZEPHYR_CLOCK_COUNTER)

/*************
Copyright (c) 2023, Norwegian University of Science and Technology.

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

/**
 * @brief This implements the timing-related platform API ontop of the kernel
 * timer of Zephyr. This is less precise, but more portable than the alternative
 * Counter based implementation.
 *
 * @author{Erling Jellum <erling.r.jellum@ntnu.no>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#include <zephyr/kernel.h>
#include <errno.h>

#include "platform/lf_zephyr_support.h"
#include "low_level_platform.h"
#include "logging_macros.h"

// Convert Zephyr ticks into an interval_t. According to Zephyr docs the
// ticks are 100Hz for QEMU emulations, and normally a multiple of 10.
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC == 100
#define TICKS_TO_NSEC(ticks) MSEC(10 * ticks)
#elif CONFIG_SYS_CLOCK_TICKS_PER_SEC == 1000
#define TICKS_TO_NSEC(ticks) MSEC(ticks)
#elif CONFIG_SYS_CLOCK_TICKS_PER_SEC == 10000
#define TICKS_TO_NSEC(ticks) USEC(100 * ticks)
#elif CONFIG_SYS_CLOCK_TICKS_PER_SEC == 100000
#define TICKS_TO_NSEC(ticks) USEC(10 * ticks)
#elif CONFIG_SYS_CLOCK_TICKS_PER_SEC == 1000000
#define TICKS_TO_NSEC(ticks) USEC(1 * ticks)
#elif CONFIG_SYS_CLOCK_TICKS_PER_SEC == 10000000
#define TICKS_TO_NSEC(ticks) NSEC(100 * ticks)
#else
#define TICKS_TO_NSEC(ticks) ((SECONDS(1) / CONFIG_SYS_CLOCK_TICKS_PER_SEC) * ticks)
#endif

static uint32_t timer_freq;
static volatile bool async_event = false;

// Statically create an initialize the semaphore used for sleeping.
K_SEM_DEFINE(sleeping_sem, 0, 1)

void _lf_initialize_clock() {
  timer_freq = CONFIG_SYS_CLOCK_TICKS_PER_SEC;
  lf_print("--- Using LF Zephyr Kernel Clock with a frequency of %u Hz", timer_freq);
}

/** Uses Zephyr's monotonic increasing uptime count. */
int _lf_clock_gettime(instant_t* t) {
  interval_t uptime = k_uptime_ticks();
  *t = TICKS_TO_NSEC(uptime);
  return 0;
}

/** Interruptable sleep is implemented by a taking a semaphore with a timeout. */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup) {
  async_event = false;

  interval_t duration = wakeup - lf_time_physical();
  if (duration <= 0) {
    return 0;
  }

  // Reset the semaphore. This is safe to do before we leave the critical
  // section.
  k_sem_reset(&sleeping_sem);

  if (lf_critical_section_exit(env)) {
    lf_print_error_and_exit("Failed to exit critical section.");
  }

  int res = k_sem_take(&sleeping_sem, K_NSEC(duration));

  if (lf_critical_section_enter(env)) {
    lf_print_error_and_exit("Failed to exit critical section.");
  }

  if (res == 0) {
    // We got the semaphore, this means there should be a new event
    if (!async_event) {
      lf_print_warning("Sleep was interrupted, but no new event");
    }
    async_event = false;
    return -1;
  } else if (res == -EAGAIN) {
    // This means we timed out and have reached our wakeup instant.
    return 0;
  } else {
    lf_print_error_and_exit("k_sem_take returned %d", res);
    return -1;
  }
}

/**
 * Asynchronous events are notified by signalling a semaphore which will wakeup
 * the runtime if it is sleeping, and setting a flag to indicate what has
 * happened.
 */
int _lf_single_threaded_notify_of_event() {
  async_event = true;
  k_sem_give(&sleeping_sem);
  return 0;
}

#endif
#endif
