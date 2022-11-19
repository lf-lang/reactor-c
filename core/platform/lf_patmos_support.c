/* Patmos API support for the C target of Lingua Franca. */

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

/** Patmos API support for the C target of Lingua Franca.
 *
 *  @author{Martin Schoeberl <martin@jopdesign.com>}
 */

#include "lf_patmos_support.h"
#include "platform.h"

#if defined NUMBER_OF_WORKERS || defined LINGUA_FRANCA_TRACE
#if __STDC_VERSION__ < 201112L || defined (__STDC_NO_THREADS__)
#include "lf_POSIX_threads_support.h"  // (Not C++11 or later) or no threads support
#else
#include "lf_C11_threads_support.h"
#endif
#endif

#include "lf_unix_clock_support.h"

#include <time.h>

/**
 * Pause execution for a number of nanoseconds.
 *
 * A Linux-specific clock_nanosleep is used underneath that is supposedly more
 * accurate.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_nanosleep`).
 */
int lf_nanosleep(instant_t requested_time) {
    const struct timespec tp = convert_ns_to_timespec(requested_time);
    struct timespec remaining;
    return nanosleep((const struct timespec*)&tp, (struct timespec*)&remaining);
}
