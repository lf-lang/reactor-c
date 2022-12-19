/* nRF52832 API support for the C target of Lingua Franca. */

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

/** nrf52 API support for the C target of Lingua Franca.
 *  
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 *  @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 *  @author{Erling Rennemo Jellum <erling.r.jellum@ntnu.no>}
 */

#ifndef LF_NRF52_SUPPORT_H
#define LF_NRF52_SUPPORT_H

// This embedded platform has no TTY suport
#define NO_TTY 

#include <stdint.h> // For fixed-width integral types
#include <time.h>   // For CLOCK_MONOTONIC
#include <stdbool.h>

#include <inttypes.h>  // Needed to define PRId64 and PRIu32
#define PRINTF_TIME "%" PRId64
#define PRINTF_MICROSTEP "%" PRIu32
#define PRINTF_TAG "(%" PRId64 ", %" PRIu32 ")"

typedef int64_t _instant_t;

/**
 * Interval of time.
 */
typedef int64_t _interval_t;

/**
 * Microstep instant.
 */
typedef uint32_t _microstep_t;

/**
 * No mutex or condition variable needed for single threaded NRF platforms
 */
typedef void _lf_mutex_t;
typedef void _lf_cond_var_t;

/**
 * For user-friendly reporting of time values, the buffer length required.
 * This is calculated as follows, based on 64-bit time in nanoseconds:
 * Maximum number of weeks is 15,250
 * Maximum number of days is 6
 * Maximum number of hours is 23
 * Maximum number of minutes is 59
 * Maximum number of seconds is 59
 * Maximum number of nanoseconds is 999,999,999
 * Maximum number of microsteps is 4,294,967,295
 * Total number of characters for the above is 24.
 * Text descriptions and spaces add an additional 55,
 * for a total of 79. One more allows for a null terminator.
 */
#define LF_TIME_BUFFER_LENGTH 80


// The underlying physical clock for Linux
#define _LF_CLOCK CLOCK_MONOTONIC

#endif // LF_nRF52832_SUPPORT_H
