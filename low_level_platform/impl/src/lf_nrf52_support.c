#ifdef PLATFORM_NRF52
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

/**
 * @brief NRF52 support for the C target of Lingua Franca.
 *
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 * @author{Erling Jellum <erling.r.jellum@ntnu.no>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#include <stdlib.h> // Defines malloc.
#include <string.h> // Defines memcpy.
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "platform/lf_nrf52_support.h"
#include "../platform.h"
#include "../utils/util.h"
#include "../tag.h"

#include "nrf.h"
#include "nrfx_timer.h"
#include "nrf_nvic.h"
#include "app_error.h"

/**
 * True when the last requested sleep has been completed, false otherwise.
 */
static volatile bool _lf_sleep_interrupted = false;
static volatile bool _lf_async_event = false;

/**
 * lf global timer instance
 * timerId = 3 TIMER3
 */
static const nrfx_timer_t g_lf_timer_inst = NRFX_TIMER_INSTANCE(3);

// Combine 2 32bit works to a 64 bit word
#define COMBINE_HI_LO(hi,lo) ((((uint64_t) hi) << 32) | ((uint64_t) lo))

// Maximum and minimum sleep possible
#define LF_MAX_SLEEP_NS USEC(UINT32_MAX)
#define LF_MIN_SLEEP_NS USEC(5) 

/**
 * Variable tracking the higher 32bits of the time.
 * This is incremented at each timer overflow.
 */
static volatile uint32_t _lf_time_us_high = 0;

/**
 * Flag passed to sd_nvic_critical_region_*
 */
uint8_t _lf_nested_region = 0;

/**
 * @brief Handle LF timer interrupts
 * Using lf_timer instance -> id = 3
 * channel2 -> channel for lf_sleep interrupt
 * channel3 -> channel for overflow interrupt
 *
 * [in] event_type
 *      channel that fired interrupt on timer
 * [in] p_context
 *      context passed to handler
 * 
 */
void lf_timer_event_handler(nrf_timer_event_t event_type, void *p_context) {
    
    if (event_type == NRF_TIMER_EVENT_COMPARE2) {
        _lf_sleep_interrupted = false;
    } else if (event_type == NRF_TIMER_EVENT_COMPARE3) {
        _lf_time_us_high =+ 1;
    }
}

void _lf_initialize_clock() {
    ret_code_t error_code;
    _lf_time_us_high = 0;

    // Initialize TIMER3 as a free running timer
    // 1) Set to be a 32 bit timer
    // 2) Set to count at 1MHz
    // 3) Clear the timer
    // 4) Start the timer

    nrfx_timer_config_t timer_conf = {
        .frequency = NRF_TIMER_FREQ_1MHz,
        .mode = NRF_TIMER_MODE_TIMER,
        .bit_width = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = 7, // lowest
        .p_context = NULL,
    };

    error_code = nrfx_timer_init(&g_lf_timer_inst, &timer_conf, &lf_timer_event_handler);
    APP_ERROR_CHECK(error_code);
    // Enable an interrupt to occur on channel NRF_TIMER_CC_CHANNEL3
    // when the timer reaches its maximum value and is about to overflow.
    nrfx_timer_compare(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL3, 0x0, true);
    nrfx_timer_enable(&g_lf_timer_inst);
}

/**
 * Fetch the value of _LF_CLOCK (see lf_linux_support.h) and store it in *t. The
 * timestamp value in 't' will will be the number of nanoseconds since the board was reset.
 * The timers on the board have only 32 bits and their resolution is in microseconds, so
 * the time returned will always be an integer number of microseconds. Moreover, after about 71
 * minutes of operation, the timer overflows. 
 * 
 * The function reads out the upper word before and after reading the timer.
 * If the upper word has changed (i.e. there was an overflow in between),
 * we cannot simply combine them. We read once more to be sure that 
 * we read after the overflow.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_gettime`).
 */
int _lf_clock_gettime(instant_t* t) {
    assert(t);
    
    uint32_t now_us_hi_pre = _lf_time_us_high;
    uint32_t now_us_low = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL1);
    uint32_t now_us_hi_post = _lf_time_us_high; 

    // Check if we read the time during a wrap
    if (now_us_hi_pre != now_us_hi_post) {
        // There was a wrap. read again and return
        now_us_low = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL1);
    }
    uint64_t now_us = COMBINE_HI_LO(now_us_hi_post, now_us_low);

    *t = ((instant_t)now_us) * 1000;
    return 0;
}

/**
 * @brief Pause execution for a given duration.
 * 
 * This implementation performs a busy-wait because it is unclear what will
 * happen if this function is called from within an ISR.
 * 
 * @param sleep_duration 
 * @return 0 for success, or -1 for failure.
 */
int lf_sleep(interval_t sleep_duration) {
    instant_t target_time;
    instant_t current_time;
    _lf_clock_gettime(&current_time);
    target_time = current_time + sleep_duration;
    
    while (current_time <= target_time) {
        _lf_clock_gettime(&current_time);
    }
    return 0;
}

/**
 * @brief Do a busy-wait until a time instant
 * 
 * @param wakeup_time 
 */

static void lf_busy_wait_until(instant_t wakeup_time) {
    instant_t now;
    do {
        _lf_clock_gettime(&now);
    } while (now < wakeup_time);
}

/**
 * @brief Sleep until the given wakeup time. There are a couple of edge cases to consider
 *  1. Wakeup time is already past
 *  2. Implied sleep duration is below `LF_MAX_SLEEP_NS` threshold
 *  3. Implied sleep duration is above `LF_MAX_SLEEP_NS` limit
 * 
 * @param wakeup_time The time instant at which to wake up.
 * @return int 0 if sleep completed, or -1 if it was interrupted.
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
    instant_t now;
    _lf_clock_gettime(&now);
    interval_t duration = wakeup_time - now;
    if (duration <= 0) {
        return 0;
    } else if (duration < LF_MIN_SLEEP_NS) {
        lf_busy_wait_until(wakeup_time);
        return 0;
    } 

    // The sleeping while loop continues until either:
    // 1) A physical action is scheduled, resulting in a new event on the event queue
    // 2) Sleep has completed successfully
    bool sleep_next = true;
    _lf_sleep_interrupted = false;
    _lf_async_event = false;

    do {
        // Schedule a new timer interrupt unless we already have one pending
        if (!_lf_sleep_interrupted) {
            uint32_t curr_timer_val = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2);
            uint32_t target_timer_val = 0;
            // If the remaining sleep is longer than the limit, sleep for the maximum possible time.
            if (duration > LF_MAX_SLEEP_NS) {
                target_timer_val = curr_timer_val-1;
                duration -= LF_MAX_SLEEP_NS;
            } else {
                target_timer_val = (uint32_t)(wakeup_time / 1000);
                sleep_next = false;        
            }
            // init timer interrupt for sleep time
            _lf_sleep_interrupted = true;
            nrfx_timer_compare(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2, target_timer_val, true);
        }

        // Leave critical section
        lf_enable_interrupts_nested();
        
        // wait for exception
        __WFE();

        // Enter critical section again
        lf_disable_interrupts_nested();

        // Redo while loop and go back to sleep if:
        //  1) We didnt have async event AND
        //  2) We have more sleeps left OR the sleep didnt complete
        // 
        // This means we leave the sleep while if:
        //  1) There was an async event OR
        //  2) no more sleeps AND sleep not interrupted 
    } while(!_lf_async_event && (sleep_next || _lf_sleep_interrupted));
    
    if (!_lf_async_event) {
        return 0;
    } else {
        LF_PRINT_DEBUG("Sleep got interrupted...\n");
        return -1;
    }
}

/**
 * @brief Enter critical section. Let NRF Softdevice handle nesting
 * @return int 
 */
int lf_enable_interrupts_nested() {
    return sd_nvic_critical_region_enter(&_lf_nested_region);
}

/**
 * @brief Exit citical section. Let NRF SoftDevice handle nesting
 * 
 * @return int 
 */
int lf_disable_interrupts_nested() {
    return sd_nvic_critical_region_exit(_lf_nested_region);
}

/**
 * @brief Set global flag to true so that sleep will return when woken 
 * 
 * @return int 
 */
int _lf_single_threaded_notify_of_event() {
    _lf_async_event = true;
    return 0;
}
#endif
