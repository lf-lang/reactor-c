#if defined(PLATFORM_ZEPHYR)
#include "lf_zephyr_board_support.h"
#if defined(LF_ZEPHYR_CLOCK_KERNEL)

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

#include "lf_zephyr_support.h"
#include "platform.h"
#include "util.h"

static int64_t epoch_duration_nsec;
static volatile int64_t last_epoch_nsec = 0;
static uint32_t timer_freq;
static volatile bool async_event = false;

void _lf_initialize_clock() {
    timer_freq = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
    LF_PRINT_LOG("--- Using LF Zephyr Kernel Clock with a frequency of %u Hz\n", timer_freq);
    last_epoch_nsec = 0;
    epoch_duration_nsec = ((1LL << 32) * SECONDS(1))/CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

/**
 * Detect wraps by storing the previous clock readout. When a clock readout is
 * less than the previous we have had a wrap. This only works of `_lf_clock_gettime`
 * is invoked at least once per epoch. 
 */
int _lf_clock_gettime(instant_t* t) {
    static uint32_t last_read_cycles=0;
    uint32_t now_cycles = k_cycle_get_32();
    if (now_cycles < last_read_cycles) {
        last_epoch_nsec += epoch_duration_nsec;
    }
    *t = (SECOND(1)/CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)*now_cycles + last_epoch_nsec;
    last_read_cycles = now_cycles;
    return 0;
}

/**
 * Interruptable sleep is implemented using busy-waiting.
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup) {
    async_event=false;    

    LF_CRITICAL_SECTION_EXIT(env);
    instant_t now;
    do {
    _lf_clock_gettime(&now);
    } while ( (now<wakeup) && !async_event);
    LF_CRITICAL_SECTION_ENTER(env);

    if (async_event) {
        async_event=false;
        return -1;
    } else {
        return 0;
    }
}

/**
 * Asynchronous events are notified by setting a flag which breaks the sleeping
 * thread out of the busy-wait.
 */
int _lf_single_threaded_notify_of_event() {
   async_event = true;
   return 0;
}

#endif
#endif
