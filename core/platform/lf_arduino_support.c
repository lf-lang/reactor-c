#if defined(PLATFORM_ARDUINO)
/* Arduino Platform API support for the C target of Lingua Franca. */

/*************
Copyright (c) 2022, The University of California at Berkeley.
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

/** Arduino API support for the C target of Lingua Franca.
 *
 *  @author{Anirudh Rengarajan <arengarajan@berkeley.edu>}
 *  @author{Erling Rennemo Jellum <erling.r.jellum@ntnu.no>}
 */


#include <time.h>
#include <errno.h>
#include <assert.h>

#include "lf_arduino_support.h"
#include "../platform.h"
#include "Arduino.h"

// Combine 2 32bit values into a 64bit
#define COMBINE_HI_LO(hi,lo) ((((uint64_t) hi) << 32) | ((uint64_t) lo))

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not
static volatile int _lf_num_nested_critical_sections = 0;

/**
 * Global timing variables:
 * Since Arduino is 32bit, we need to also maintain the 32 higher bits.

 * _lf_time_us_high is incremented at each overflow of 32bit Arduino timer.
 * _lf_time_us_low_last is the last value we read from the 32 bit Arduino timer.
 *  We can detect overflow by reading a value that is lower than this.
 *  This does require us to read the timer and update this variable at least once per 35 minutes.
 *  This is not an issue when we do a busy-sleep. If we go to HW timer sleep we would want to register an interrupt
 *  capturing the overflow.

 */
static volatile uint32_t _lf_time_us_high = 0;
static volatile uint32_t _lf_time_us_low_last = 0;

/**
 * @brief Sleep until an absolute time.
 * TODO: For improved power consumption this should be implemented with a HW timer and interrupts.
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
 * Initialize the LF clock. Arduino auto-initializes its clock, so we don't do anything.
 */
void _lf_initialize_clock() {}

/**
 * Write the current time in nanoseconds into the location given by the argument.
 * This returns 0 (it never fails, assuming the argument gives a valid memory location).
 * This has to be called at least once per 35 minutes to properly handle overflows of the 32-bit clock.
 * TODO: This is only addressable by setting up interrupts on a timer peripheral to occur at wrap.
 */
int _lf_clock_gettime(instant_t* t) {

    assert(t != NULL);

    uint32_t now_us_low = micros();

    // Detect whether overflow has occured since last read
    // TODO: This assumes that we _lf_clock_gettime is called at least once per overflow
    if (now_us_low < _lf_time_us_low_last) {
        _lf_time_us_high++;
    }

    *t = COMBINE_HI_LO(_lf_time_us_high, now_us_low) * 1000ULL;
    return 0;
}

int lf_enable_interrupts_nested() {
    if (_lf_num_nested_critical_sections++ == 0) {
        // First nested entry into a critical section.
        // If interrupts are not initially enabled, then increment again to prevent
        // TODO: Do we need to check whether the interrupts were enabled to
        //  begin with? AFAIK there is no Arduino API for that
        noInterrupts();
    }
    return 0;
}

int lf_disable_interrupts_nested() {
    if (_lf_num_nested_critical_sections <= 0) {
        return 1;
    }
    if (--_lf_num_nested_critical_sections == 0) {
        interrupts();
    }
    return 0;
}

#if defined(LF_SINGLE_THREADED)
/**
 * Handle notifications from the runtime of changes to the event queue.
 * If a sleep is in progress, it should be interrupted.
*/
int _lf_single_threaded_notify_of_event() {
   _lf_async_event = true;
   return 0;
}

#else
#warning "Threaded support on Arduino is still experimental"
#include "ConditionWrapper.h"
#include "MutexWrapper.h"
#include "ThreadWrapper.h"

// Typedef that represents the function pointers passed by LF runtime into lf_thread_create
typedef void *(*lf_function_t) (void *);

/**
 * @brief Get the number of cores on the host machine.
 */
int lf_available_cores() {
    return 1;
}

int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    lf_thread_t t = thread_new();
    long int start = thread_start(t, *lf_thread, arguments);
    *thread = t;
    return start;
}

int lf_thread_join(lf_thread_t thread, void** thread_return) {
   return thread_join(thread, thread_return);
}

int lf_mutex_init(lf_mutex_t* mutex) {
    *mutex = (lf_mutex_t) mutex_new();
    return 0;
}

int lf_mutex_lock(lf_mutex_t* mutex) {
    mutex_lock(*mutex);
    return 0;
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
    mutex_unlock(*mutex);
    return 0;
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    *cond = (lf_cond_t) condition_new (*mutex);
    return 0;
}

int lf_cond_broadcast(lf_cond_t* cond) {
    condition_notify_all(*cond);
    return 0;
}

int lf_cond_signal(lf_cond_t* cond) {
    condition_notify_one(*cond);
    return 0;
}

int lf_cond_wait(lf_cond_t* cond) {
    condition_wait(*cond);
    return 0;
}

int _lf_cond_timedwait(lf_cond_t* cond, instant_t wakeup_time) {
    instant_t now;
    _lf_clock_gettime(&now);
    interval_t sleep_duration_ns = wakeup_time - now;
    bool res = condition_wait_for(*cond, sleep_duration_ns);
    if (!res) {
        return 0;
    } else {
        return LF_TIMEOUT;
    }
}

#endif
#endif
