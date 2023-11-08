#if defined(PLATFORM_ZEPHYR)
#include "lf_zephyr_board_support.h"
#if defined(LF_ZEPHYR_CLOCK_COUNTER)
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
 * @brief This implements the timing-related platform API ontop of the Zephyr
 * Counter API. The Counter API is a generic interface to a timer peripheral. It
 * gives the best timing performance and allows actual sleeping rather than
 * busy-waiting which is performed with the Kernel API.
 *
 * @author{Erling Jellum <erling.r.jellum@ntnu.no>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>

#include "lf_zephyr_support.h"
#include "platform.h"
#include "util.h"

static int64_t epoch_duration_nsec;
static int64_t epoch_duration_usec;
static uint32_t counter_max_ticks;
static volatile int64_t last_epoch_nsec = 0;
static uint32_t counter_freq;
static volatile bool async_event = false;

K_SEM_DEFINE(semaphore,0,1)

static struct counter_alarm_cfg alarm_cfg;
const struct device *const counter_dev = DEVICE_DT_GET(LF_TIMER);   
static volatile bool alarm_fired;

/**
 * This callback is invoked when the underlying Timer peripheral overflows.
 * Handled by incrementing the epoch variable.
 */
static void overflow_callback(const struct device *dev, void *user_data) {
    last_epoch_nsec += epoch_duration_nsec;
}

/**
 * This callback is invoked when the alarm configured for sleeping expires.
 * The sleeping thread is released by giving it the semaphore.
 */
static void alarm_callback(const struct device *counter_dev,
				      uint8_t chan_id, uint32_t ticks,
				      void *user_data) {
    alarm_fired=true;
    k_sem_give(&semaphore);
}

/**
 * Initialize the Counter device. Check its frequency and compute epoch
 * durations.
 */
void _lf_initialize_clock() {
    struct counter_top_cfg counter_top_cfg;
    uint32_t counter_max_ticks=0;
    int res;
	
    // Verify that we have the device
    if (!device_is_ready(counter_dev)) {
		  lf_print_error_and_exit("ERROR: counter device not ready.\n");
    }

    // Verify that it is working as we think
    if(!counter_is_counting_up(counter_dev)) {
        lf_print_error_and_exit("ERROR: Counter is counting down \n");
    }
    
    // Get the frequency of the timer
    counter_freq = counter_get_frequency(counter_dev);

    // Calculate the duration of an epoch. Compute both
    //  nsec and usec now at boot to avoid these computations later
    counter_max_ticks = counter_get_max_top_value(counter_dev);
    epoch_duration_usec = counter_ticks_to_us(counter_dev, counter_max_ticks);
    epoch_duration_nsec = epoch_duration_usec * 1000LL;
    
    // Set the max_top value to be the maximum
    counter_top_cfg.ticks = counter_max_ticks;
    counter_top_cfg.callback = overflow_callback;
    res = counter_set_top_value(counter_dev, &counter_top_cfg);
    if (res != 0) {
        lf_print_error_and_exit("ERROR: Timer couldnt set top value\n");
    }

    LF_PRINT_LOG("--- Using LF Zephyr Counter Clock with a frequency of %u Hz and wraps every %u sec\n", 
      counter_freq, counter_max_ticks/counter_freq);
    
    // Prepare the alarm config
    alarm_cfg.flags = 0;
    alarm_cfg.ticks = 0;
    alarm_cfg.callback = alarm_callback;
    alarm_cfg.user_data = &alarm_cfg;

    // Start counter
    counter_start(counter_dev);
}

/**
 * The Counter device tracks current physical time. Overflows are handled in an
 * ISR.
 */
int _lf_clock_now(instant_t* t) {
    static uint64_t last_nsec = 0;
    uint32_t now_cycles;
    int res;
    uint64_t now_nsec;
    
    res = counter_get_value(counter_dev, &now_cycles);
    now_nsec = counter_ticks_to_us(counter_dev, now_cycles)*1000ULL + last_epoch_nsec;

    // Make sure that the clock is monotonic. We might have had a wrap but the
    // epoch has not been updated because interrupts are disabled.
    if (now_nsec < last_nsec) {
        now_nsec = last_nsec + 1;
    }

    *t = now_nsec;
    last_nsec = now_nsec;
    return 0;
}

/**
 * Handle interruptable sleep by configuring a future alarm callback and waiting
 * on a semaphore. Make sure we can handle sleeps that exceed an entire epoch
 * of the Counter.
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup) {
    // Reset flags
    alarm_fired = false;
    async_event = false;
    k_sem_reset(&semaphore);

    // Calculate the sleep duration
    uint32_t now_cycles, sleep_duration_ticks;
    counter_get_value(counter_dev, &now_cycles);
    instant_t now;
    _lf_clock_now(&now);
    interval_t sleep_for_us = (wakeup - now)/1000;
    
    while ( !async_event && 
            sleep_for_us > (LF_WAKEUP_OVERHEAD_US + LF_MIN_SLEEP_US)
    ) {  
        if (sleep_for_us < epoch_duration_usec) {
            sleep_duration_ticks = counter_us_to_ticks(counter_dev, ((uint64_t) sleep_for_us) - LF_WAKEUP_OVERHEAD_US);
        } else {
            sleep_duration_ticks = UINT32_MAX;
        }

        alarm_cfg.ticks = sleep_duration_ticks;
        int err = counter_set_channel_alarm(counter_dev, LF_TIMER_ALARM_CHANNEL,  &alarm_cfg);
     
        if (err != 0) {
            lf_print_error_and_exit("Could not setup alarm for sleeping. Errno %i", err);
        }
        
        lf_critical_section_exit(env);
        k_sem_take(&semaphore, K_FOREVER);
        lf_critical_section_enter(env);

        // Then calculating remaining sleep, unless we got woken up by an event
        if (!async_event) {
            _lf_clock_now(&now);
            sleep_for_us = (wakeup - now)/1000;
        }
    } 
    
    // Do remaining sleep in busy_wait
    if (!async_event &&
        sleep_for_us > LF_RUNTIME_OVERHEAD_US) {        
        k_busy_wait((uint32_t) (sleep_for_us - LF_RUNTIME_OVERHEAD_US));
    }

    if (async_event) {
        // Cancel the outstanding alarm
        counter_cancel_channel_alarm(counter_dev, LF_TIMER_ALARM_CHANNEL);
        async_event = false;
        return -1;
    } else {
        return 0;
    }
}

/**
 * We notify of async events by setting the flag and giving the semaphore.
 */
int _lf_single_threaded_notify_of_event() {
   async_event = true;
    k_sem_give(&semaphore);
   return 0;
}

#endif
#endif
