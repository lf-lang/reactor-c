#if defined(PLATFORM_PATMOS)
/*************
Copyright (c) 2024, The University of California at Berkeley.

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
 * @author{Ehsan Khodadad <ehkh@dtu.dk>}
 * @author{Luca Pezzarossa <lpez@dtu.dk>}
 * @author{Martin Schoeberl <masca@dtu.dk>}
 */
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "platform/lf_patmos_support.h"
#include "low_level_platform.h"
#include <machine/rtc.h>
#include <machine/exceptions.h>
#include <stdio.h>

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not
static volatile int _lf_num_nested_critical_sections = 0;
/**
 * @brief Sleep until an absolute time.
 * Since there is no sleep mode in Patmos, and energy saving is not important for real-time systems,
 * we just used a busy sleep.
 *
 * @param wakeup int64_t time of wakeup
 * @return int 0 if successful sleep, -1 if awoken by async event
 */

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup) {
  instant_t now;
  _lf_async_event = false;
  lf_enable_interrupts_nested();

  // Do busy sleep
  do {
    _lf_clock_gettime(&now);
  } while ((now < wakeup) && !_lf_async_event);

  lf_disable_interrupts_nested();

  if (_lf_async_event) {
    _lf_async_event = false;
    return -1;
  } else {
    return 0;
  }
}

int lf_sleep(interval_t sleep_duration) {
  instant_t now;
  _lf_clock_gettime(&now);
  instant_t wakeup = now + sleep_duration;

  // Do busy sleep
  do {
    _lf_clock_gettime(&now);
  } while ((now < wakeup));
  return 0;
}

/**
 * Pause execution for a number of nanoseconds.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_nanosleep`).
 */
int lf_nanosleep(interval_t requested_time) { return lf_sleep(requested_time); }

/**
 * Patmos clock does not need initialization.
 */
void _lf_initialize_clock() {}

/**
 * Write the current time in nanoseconds into the location given by the argument.
 * This returns 0 (it never fails, assuming the argument gives a valid memory location).
 */

int _lf_clock_gettime(instant_t* t) {

  assert(t != NULL);

  *t = get_cpu_usecs() * 1000;

  return 0;
}

#if defined(LF_SINGLE_THREADED)

int lf_disable_interrupts_nested() {
  if (_lf_num_nested_critical_sections++ == 0) {
    intr_disable();
  }
  return 0;
}

int lf_enable_interrupts_nested() {
  if (_lf_num_nested_critical_sections <= 0) {
    return 1;
  }

  if (--_lf_num_nested_critical_sections == 0) {
    intr_enable();
  }
  return 0;
}

int _lf_single_threaded_notify_of_event() {
  _lf_async_event = true;
  return 0;
}
#endif // LF_SINGLE_THREADED

#endif // PLATFORM_PATMOS
