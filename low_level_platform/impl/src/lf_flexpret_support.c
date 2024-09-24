#if defined(PLATFORM_FLEXPRET)
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

/** Support file for Bare-metal FlexPRET platform.
 *
 *  @author{Shaokai Lin <shaokai@berkeley.edu>}
 *  @author{Magnus Mæhlum <magnmaeh@stud.ntnu.no>}
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "low_level_platform.h"

/**
 * Used to keep track of the number of nested critical sections.
 *
 * We should only disable interrupts when this is zero and we enter a critical section
 * We should only  enable interrupts when we exit a critical section and this is zero
 */
static int critical_section_num_nested[FP_THREADS] = THREAD_ARRAY_INITIALIZER(0);

static volatile bool _lf_async_event_occurred = false;

#define EPOCH_DURATION_NS (1ULL << 32)

int _lf_clock_gettime(instant_t* t) {
  *t = (instant_t)rdtime64();
  return 0;
}

int _lf_sleep_common(instant_t wakeup_time, bool interruptable) {
  // Store the number of epochs; i.e., how many times the 32-bit timer
  // will overflow
  uint32_t wakeup_time_epochs = 0;
  uint32_t wakeup_time_after_epochs = 0;
  uint32_t sleep_start = rdtime();

  if (wakeup_time > (instant_t)EPOCH_DURATION_NS) {
    wakeup_time_epochs = wakeup_time / EPOCH_DURATION_NS;
    wakeup_time_after_epochs = wakeup_time % EPOCH_DURATION_NS;

    if (wakeup_time < sleep_start) {
      // This means we need to do another epoch
      wakeup_time_epochs++;
    }
  } else {
    wakeup_time_epochs = 0;
    wakeup_time_after_epochs = wakeup_time;
    if (wakeup_time < sleep_start) {
      // Nothing to do; should not happen
      // LF_PRINT_DEBUG("FlexPRET: _lf_sleep_common called with wakeup_time < current time\n");
      return 0;
    }
  }

  const uint32_t max_uint32_value = 0xFFFFFFFF;
  _lf_async_event_occurred = false;

  for (uint32_t i = 0; i < wakeup_time_epochs; i++) {
    // The first sleep until will only be partial
    if (interruptable) {
      // Can be interrupted
      // NOTE: Does not work until this issue is resolved:
      // https://github.com/pretis/flexpret/issues/93
      fp_wait_until(max_uint32_value);
      if (_lf_async_event_occurred)
        break;
    } else {
      // Cannot be interrupted
      // NOTE: Does not work until this issue is resolved:
      // https://github.com/pretis/flexpret/issues/93
      fp_delay_until(max_uint32_value);
    }
  }

  if (interruptable) {
    if (!_lf_async_event_occurred) {
      fp_wait_until(wakeup_time_after_epochs);
    }
  } else {
    // Cannot be interrupted
    fp_delay_until(wakeup_time_after_epochs);
  }

  return _lf_async_event_occurred;
}

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
  // Enable interrupts and execute wait until instruction
  lf_critical_section_exit(env);

  // Wait until will stop sleep if interrupt occurs
  int ret = _lf_sleep_common(wakeup_time, true);

  lf_critical_section_enter(env);
  return ret;
}

int lf_sleep(interval_t sleep_duration) {
  interval_t sleep_until = rdtime64() + sleep_duration;
  return _lf_sleep_common(sleep_until, false);
}

/**
 * Initialize the LF clock.
 */
void _lf_initialize_clock() {
  // FlexPRET clock does not require any initialization
}

int lf_disable_interrupts_nested() {
  // In the special case where this function is called during an interrupt
  // subroutine (isr) it should have no effect
  if ((read_csr(CSR_STATUS) & 0x04) == 0x04)
    return 0;

  uint32_t hartid = read_hartid();

  fp_assert(critical_section_num_nested[hartid] >= 0, "Number of nested critical sections less than zero.");
  if (critical_section_num_nested[hartid]++ == 0) {
    fp_interrupt_disable();
  }
  return 0;
}

int lf_enable_interrupts_nested() {
  // In the special case where this function is called during an interrupt
  // subroutine (isr) it should have no effect
  if ((read_csr(CSR_STATUS) & 0x04) == 0x04)
    return 0;

  uint32_t hartid = read_hartid();

  if (--critical_section_num_nested[hartid] == 0) {
    fp_interrupt_enable();
  }
  fp_assert(critical_section_num_nested[hartid] >= 0, "Number of nested critical sections less than zero.");
  return 0;
}

/**
 * Pause execution for a number of nanoseconds.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_nanosleep`).
 */
int lf_nanosleep(interval_t requested_time) { return lf_sleep(requested_time); }

#if defined(LF_SINGLE_THREADED)

int _lf_single_threaded_notify_of_event() {
  _lf_async_event_occurred = true;
  return 0;
}

#else // Multi threaded

int lf_available_cores() {
  return FP_THREADS - 1; // Return the number of Flexpret HW threads
}

lf_thread_t lf_thread_self() { return read_hartid(); }

int lf_thread_create(lf_thread_t* thread, void* (*lf_thread)(void*), void* arguments) {
  /**
   * Need to select between HRTT or SRTT; see
   * https://github.com/lf-lang/reactor-c/issues/421
   */
  return fp_thread_create(HRTT, thread, lf_thread, arguments);
}

int lf_thread_join(lf_thread_t thread, void** thread_return) { return fp_thread_join(thread, thread_return); }

int lf_mutex_init(lf_mutex_t* mutex) {
  *mutex = (lf_mutex_t)FP_LOCK_INITIALIZER;
  return 0;
}

int lf_mutex_lock(lf_mutex_t* mutex) {
  fp_lock_acquire(mutex);
  return 0;
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
  fp_lock_release(mutex);
  return 0;
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
  *cond = (lf_cond_t)FP_COND_INITIALIZER(mutex);
  return 0;
}

int lf_cond_broadcast(lf_cond_t* cond) { return fp_cond_broadcast(cond); }

int lf_cond_signal(lf_cond_t* cond) { return fp_cond_signal(cond); }

int lf_cond_wait(lf_cond_t* cond) { return fp_cond_wait(cond); }

int _lf_cond_timedwait(lf_cond_t* cond, instant_t absolute_time_ns) {
  return (fp_cond_timed_wait(cond, absolute_time_ns) == FP_TIMEOUT) ? LF_TIMEOUT : 0;
}

int lf_thread_id() { return read_hartid(); }

void initialize_lf_thread_id() {
  // Nothing needed here; thread ID's are already available in harware registers
  // which can be fetched with `read_hartid`.
}
#endif

#endif // PLATFORM_FLEXPRET
