/* nRF52 API support for the C target of Lingua Franca. */

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

/**
 * nRF52 API support for the C target of Lingua Franca.
 *
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 * @author{Erling Rennemo Jellum <erling.r.jellum@ntnu.no>}
 */

#ifndef LF_NRF52_SUPPORT_H
#define LF_NRF52_SUPPORT_H

// This embedded platform has no command line interface
#define NO_CLI
#define MINIMAL_STDLIB

#include <stdint.h> // For fixed-width integral types
#include <stdbool.h>

#include <inttypes.h> // Needed to define PRId64 and PRIu32
#define PRINTF_TIME "%" PRId64
#define PRINTF_MICROSTEP "%" PRIu32
#define PRINTF_TAG "(%" PRId64 ", %" PRIu32 ")"

/**
 * No mutex or condition variable needed for single threaded NRF platforms
 */
typedef void* lf_mutex_t;
typedef void _lf_cond_var_t;

#endif // LF_nRF52832_SUPPORT_H
