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
 */


#include <time.h>
#include <errno.h>

#include "lf_arduino_support.h"
#include "../platform.h"
#include "Arduino.h"

// Combine 2 32bit values into a 64bit
#define COMBINE_HI_LO(hi,lo) ((((uint64_t) hi) << 32) | ((uint64_t) lo))

/**
 * Keep track of interrupts being raised.
 */
static volatile bool _lf_async_event = false;
static volatile bool _lf_interrupts_enabled = true;
static volatile uint32_t _lf_time_us_high = 0;
static volatile uint32_t _lf_time_us_low_last = 0;

/**
 * Pause execution for a number of nanoseconds. This should be called while in a critical section.
 * There are scenarios where this function returns with -1 but due to an old physical action which
 * has not been acked yet. So calling function has to checl pQueue to verify that a physical action
 * has occured. 
 * 
 * This is currently busy-sleeping.
 *
 * @return 0 if sleep finished. -1 if woken by async event
 */
int lf_sleep(interval_t sleep_duration) {
    assert(!_lf_interrupts_enabled);
    instant_t now;
    lf_clock_gettime(&now);
    instant_t wakeup = now + sleep_duration;
    lf_critical_section_exit();

    // Do busysleep
    do {
        lf_clock_gettime(&now);        
    } while ((now < wakeup) || !_lf_async_event)

    lf_critical_section_enter();
    if (_lf_async_event) {
        lf_ack_events();
        return -1;
    } else {
        return 0;
    }
}

/**
 * Initialize the LF clock. Arduino auto-initializes its clock, so we don't do anything.
 */
void lf_initialize_clock() {}

/**
 * Return the current time in nanoseconds
 * This has to be called at least once per 35minute to work
 * FIXME: Address this
 */
int lf_clock_gettime(instant_t* t) {
    
    if (t == NULL) {
        // The t argument address references invalid memory
        errno = EFAULT;
        return -1;
    }

    uint32_t now_us_low = micros();
    
    // Detect whether overflow has occured since last read
    if (now_us_low < _lf_time_us_low_last) {
        _lf_time_us_high++;
    }

    *t = COMBINE_HI_LO(_lf_time_us_high, now_us_low) * 1000ULL;
    return 0;
}

int lf_critical_section_enter() {
    noInterrupts();
    _lf_interrupts_enabled = false;
    return 0;
}

int lf_critical_section_exit() {
    _lf_interrupts_enabled = true;
    interrupts();
    return 0;
}

int lf_notify_of_event() {
   _lf_async_event = true;
   return 0;
}

int lf_ack_events() {
    _lf_async_event = false;
    return 0;
}

int lf_nanosleep(interval_t sleep_duration) {
    return lf_sleep(sleep_duration);
}