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

/** Baremetal RISC-V support for the C target of Lingua Franca.
 *  
 *  @author{Efsane Soyer <efsanesoyer@berkeley.edu>}
 *  @author{Samuel Berkun <sberkun@berkeley.edu>}
 */

#include "lf_spike_support.h"
#include "../platform.h"
#include "../tag.h" //needed for BILLION. In flexpret-script under tag.h CLOCK_FREQ is defined, but it's not defined in lf repo 

#ifdef NUMBER_OF_WORKERS
    #error "Threading not supported on spike"
#endif

#include <stdio.h>

// FIXME: This assumption about clock frequency
// varies by hardware platforms.
//hardcoded because it wasn't defined in tag.h
#ifndef CLOCK_FREQ
#define CLOCK_FREQ 100000000LL
#endif

void lf_initialize_clock() {
    // not needed
}


// ********** RISC-V Bare Metal Support
// Gets the current physical time by cycle counting
instant_t clock_gettime_helper() {
    // asm volatile keeps gcc from optimizing this away in loops
    // such as the busy waiting in lf_nanosleep
    uint32_t cycle_high;
    uint32_t cycle_low;
    asm volatile (
    "read_cycle:\n"
        "rdcycleh t0\n"
        "rdcycle %1\n"
        "rdcycleh %0\n"
        "bne t0, %0, read_cycle"
    : "=r"(cycle_high), "=r"(cycle_low)// outputs
    : // inputs
    : "t0" // clobbers
    );

    // Convert cycles to seconds and nanoseconds
    const float NSEC_PER_CYCLE = ((float) BILLION) / CLOCK_FREQ;
    instant_t sec = (instant_t) (cycle_low / CLOCK_FREQ) + (instant_t) (cycle_high * (UINT32_MAX / CLOCK_FREQ) + (cycle_high / CLOCK_FREQ));
    instant_t nsec = ((instant_t) (cycle_low * NSEC_PER_CYCLE)) % BILLION;

    return sec * BILLION + nsec;
}

// System call interface for getting the current time.
int lf_clock_gettime(instant_t* t) {
    *t = clock_gettime_helper();
    return 0;
}


/**
 * Pause execution for a number of nanoseconds.
 */
int lf_nanosleep(instant_t requested_time) {
    instant_t start_time = clock_gettime_helper();
    instant_t t = start_time;
    while (t < start_time + requested_time) {
        // this loop is fine because clock_gettime_helper() uses asm volatile
        t = clock_gettime_helper();
    }
    return 0;
}














