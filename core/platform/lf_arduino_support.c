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
int lf_sleep_until_locked(instant_t wakeup) {
    instant_t now;
    _lf_async_event = false;
    lf_critical_section_exit();

    // Do busy sleep
    do {
        lf_clock_gettime(&now);        
    } while ((now < wakeup) && !_lf_async_event);

    lf_critical_section_enter();

    if (_lf_async_event) {
        _lf_async_event = false;
        return -1;
    } else {
        return 0;
    }

}

/**
 * @brief Sleep for a specified duration.
 * 
 * @param sleep_duration int64_t nanoseconds representing the desired sleep duration
 * @return int 0 if success. -1 if interrupted by async event.
 */
int lf_sleep(interval_t sleep_duration) {
    instant_t now;
    lf_clock_gettime(&now);
    instant_t wakeup = now + sleep_duration;

    return lf_sleep_until_locked(wakeup);

}

/**
 * Initialize the LF clock. Arduino auto-initializes its clock, so we don't do anything.
 */
void lf_initialize_clock() {}

/**
 * Write the current time in nanoseconds into the location given by the argument.
 * This returns 0 (it never fails, assuming the argument gives a valid memory location).
 * This has to be called at least once per 35 minutes to properly handle overflows of the 32-bit clock.
 * TODO: This is only addressable by setting up interrupts on a timer peripheral to occur at wrap.
 */
int lf_clock_gettime(instant_t* t) {
    
    assert(t != NULL);

    uint32_t now_us_low = micros();
    
    // Detect whether overflow has occured since last read
    // TODO: This assumes that we lf_clock_gettime is called at least once per overflow
    if (now_us_low < _lf_time_us_low_last) {
        _lf_time_us_high++;
    }

    *t = COMBINE_HI_LO(_lf_time_us_high, now_us_low) * 1000ULL;
    return 0;
}

/**
 * Enter a critical section by disabling interrupts, supports 
 * nested critical sections.
*/
int lf_critical_section_enter() { 
    _lf_num_nested_critical_sections++;
    // if (_lf_num_nested_critical_sections++ == 0) {
    //     // First nested entry into a critical section.
    //     // If interrupts are not initially enabled, then increment again to prevent
    //     // TODO: Do we need to check whether the interrupts were enabled to
    //     //  begin with? AFAIK there is no Arduino API for that 
    //     noInterrupts();
    // }
    return 0;
}

/**
 * @brief Exit a critical section.
 * 
 * TODO: Arduino currently has bugs with its interrupt process, so we disable it for now.
 * As such, physical actions are not yet supported.
 * 
 * If interrupts were enabled when the matching call to 
 * lf_critical_section_enter()
 * occurred, then they will be re-enabled here.
 */
int lf_critical_section_exit() {
    if (_lf_num_nested_critical_sections <= 0) {
        return 1;
    }
    // if (--_lf_num_nested_critical_sections == 0) {
    //     interrupts();
    // }
    --_lf_num_nested_critical_sections;
    return 0;
}

/**
 * Handle notifications from the runtime of changes to the event queue.
 * If a sleep is in progress, it should be interrupted.
*/
int lf_notify_of_event() {
   _lf_async_event = true;
   return 0;
}
