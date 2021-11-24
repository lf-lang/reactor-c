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
 *  @author{Marten Lohstroh <marten@berkeley.edu>}
 *  @author{Shaokai Lin <shaokai@berkeley.edu>}
 */

#include "lf_spike_support.h"
#include "../platform.h"
#include <time.h> //needed for timespec _lf_time_spec_t -- is this a linux or OS based library?
#include "../tag.h" //needed for BILLION. In flexpret-script under tag.h CLOCK_FREQ is defined, but it's not defined in lf repo
#include <stdint.h> //needed for the uint32_t  -- can we use this under baremetal platform

// FIXME: This assumption about clock frequency
// varies by hardware platforms.
//hardcoded because it wasn't defined in tag.h
#define CLOCK_FREQ 100000000LL

/**
 * Convert a _lf_time_spec_t ('tp') to an instant_t representation in
 * nanoseconds.
 *
 * @return nanoseconds (long long).
 */
instant_t convert_timespec_to_ns(struct timespec tp) {
    return tp.tv_sec * 1000000000 + tp.tv_nsec;
}

/**
 * Convert an instant_t ('t') representation in nanoseconds to a
 * _lf_time_spec_t.
 *
 * @return _lf_time_spec_t representation of 't'.
 */
struct timespec convert_ns_to_timespec(instant_t t) {
    struct timespec tp;
    tp.tv_sec = t / 1000000000;
    tp.tv_nsec = (t % 1000000000);
    return tp;
}

// ********** RISC-V Bare Metal Support
// Gets the current physical time by cycle counting
struct timespec __clock_gettime() {
    
    uint32_t cycle_high;
    uint32_t cycle_low;
    struct timespec ts;

    asm(
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
    const uint32_t CYCLES_PER_NANOSEC = CLOCK_FREQ / BILLION;
    const float NSEC_PER_CYCLE = BILLION / CLOCK_FREQ;

    ts.tv_sec = (time_t) (cycle_low / CLOCK_FREQ) \
        + (time_t) (cycle_high * (UINT32_MAX / CLOCK_FREQ) + (cycle_high / CLOCK_FREQ));
    ts.tv_nsec = (long) ((uint32_t) (cycle_low * NSEC_PER_CYCLE) % BILLION);

    return ts;
}

/**
 * Initialize the LF clock.
 */
void lf_initialize_clock() {
}

// System call interface for getting the current time.
int lf_clock_gettime(instant_t* t) {
    struct timespec tp = __clock_gettime();
    *t = convert_timespec_to_ns(tp);

    return 0;
}

/**
 * Pause execution for a number of nanoseconds.
 */
// Spin waits until the request time has elapsed.
// FIXME: the multithreaded version needs to take care of rem.
int lf_nanosleep(instant_t requested_time) {
    
    struct timespec start_time = __clock_gettime();
    struct timespec ts = start_time;
    struct timespec req = convert_ns_to_timespec(requested_time);
    while (ts.tv_sec < start_time.tv_sec + req.tv_sec || (ts.tv_sec == start_time.tv_sec + req.tv_sec && ts.tv_nsec <= start_time.tv_nsec + req.tv_nsec)) {
        ts = __clock_gettime();
    }

    return 0;
}


// Default nosys implementation of _sbrk
void *
_sbrk (incr)
     int incr;
{
   extern char   end; /* Set by linker.  */
   static char * heap_end;
   char *        prev_heap_end;

   if (heap_end == 0)
     heap_end = & end;

   prev_heap_end = heap_end;
   heap_end += incr;

   return (void *) prev_heap_end;
}
