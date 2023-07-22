#if defined(PLATFORM_RP2040)
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
 * @brief Raspberry Pi Pico support for the C target of Lingua Franca 
 * Uses the pico-sdk which targets the lower level peripheral layer. 
 * 
 * @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 */

#include "lf_rp2040_support.h"
#include "platform.h"
#include "utils/util.h"
#include "tag.h"

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/sync.h>

/* 
 * critical section struct
 * disables external irq and core execution
 * provides mutual exclusion using hardware spin-locks
 */
static critical_section_t _lf_crit_sec;

/*
 * binary semaphore for lf event notification 
 * used by external isr or core threads
 * used to interact with the lf runtime thread
 */
static semaphore_t _lf_sem_irq_event;

// nested critical section counter
static uint32_t _lf_num_nested_crit_sec = 0;

void _lf_initialize_clock(void) {
    // init stdio lib
    stdio_init_all();
    // init sync structs
    critical_section_init(&_lf_crit_sec);
    sem_init(&_lf_sem_irq_event, 0, 1);
}

int _lf_clock_now(instant_t* t) {
    // time struct
    absolute_time_t now;
    uint64_t ns_from_boot;

    now = get_absolute_time();
    ns_from_boot = to_us_since_boot(now) * 1000;
    *t = (instant_t) ns_from_boot;
    return 0; 
}

int lf_sleep(interval_t sleep_duration) {
    if (sleep_duration < 0) {
        return -1;
    }
    sleep_us((uint64_t) (sleep_duration / 1000));
    return 0;
}

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
    int ret_code = 0;
    // return error
    if (wakeup_time < 0) {
        ret_code = -1;
        return ret_code;
    }
    // time struct
    absolute_time_t target;
    
    // reset event semaphore 
    sem_reset(&_lf_sem_irq_event, 0);
    // create us boot wakeup time
    target = from_us_since_boot((uint64_t) (wakeup_time / 1000));
    // allow interrupts
    lf_critical_section_exit(env);
    // blocked sleep
    // return on timeout or on processor event
    if(sem_acquire_block_until(&_lf_sem_irq_event, target)) {
        ret_code = -1;
    }
    // remove interrupts
    lf_critical_section_enter(env);
    return ret_code;
}

#ifdef LF_UNTHREADED
/*
 * The single thread rp2040 platform support treats second core
 * routines similar to external interrupt routine threads.
 *
 * Second core activity is disabled at the same times as
 * when interrupts are disabled. 
 */
int lf_disable_interrupts_nested() {
    if (!critical_section_is_initialized(&_lf_crit_sec)) {
        return 1;
    }
    // check crit sec count
    // enter non-rentrant state
    // prevent second core access and disable interrupts
    if (_lf_num_nested_crit_sec == 0) {
        // block if associated spin lock in use
        critical_section_enter_blocking(&_lf_crit_sec);
    }
    // add crit sec count
    _lf_num_nested_crit_sec++;
    return 0;
}

int lf_enable_interrupts_nested() {
    if (!critical_section_is_initialized(&_lf_crit_sec) ||
        _lf_num_nested_crit_sec <= 0) {
        return 1;
    }
    // remove from crit sec count
    _lf_num_nested_crit_sec--;
    // check crit sec count
    // release spin-lock
    if (_lf_num_nested_crit_sec == 0) {
        critical_section_exit(&_lf_crit_sec);
    }
    return 0;
}

int _lf_unthreaded_notify_of_event() {
    // notify main sleep loop of event
    sem_release(&_lf_sem_irq_event);
    return 0;
}
#endif //LF_UNTHREADED

#ifdef LF_THREADED
#error "Threading for baremetal RP2040 not supported"
#endif //LF_THREADED

#endif // PLATFORM_RP2040

