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

/**
 * Pause execution for a number of microseconds.
 *
 * This function works very accurately in the range from 3 to 16383 microseconds.
 * We cannot assure that delayMicroseconds will perform precisely for smaller delay-times.
 * Larger delay times may actually delay for an extremely brief time.
 *
 * @return 0 always.
 */
int lf_nanosleep(instant_t requested_time) {
    unsigned int microsec = (unsigned int) requested_time;
    if(microsec < 3) {
        return 0;
    }
    else if(microsec <= 16383) {
        delayMicroseconds(microsec);
    }
    else {
        delay(microsec / 1000);
    }
    return 0;
}

/**
 * Initialize the LF clock. Arduino auto-initializes its clock, so we don't do anything.
 */
void lf_initialize_clock() {}

/**
 * Fetch the value of _LF_CLOCK (see lf_arduino_support.h) and store it in t. The
 * timestamp value in 't' will be physical Arduino time in microseconds,
 * which starts once Arduino boots up.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately.
 */
int lf_clock_gettime(instant_t* t) {
    
    if (t == NULL) {
        // The t argument address references invalid memory
        errno = EFAULT;
        return -1;
    }

    *t = micros();
    return 0;
}
