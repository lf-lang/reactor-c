/* Arduino Platform API support for the C target of Lingua Franca. */

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

/** Arduino API support for the C target of Lingua Franca.
 *  
 *  @author{Anirudh Rengarajan <arengarajan@berkeley.edu>}
 */


#include <time.h>
#include <errno.h>

#include "lf_arduino_support.h"
#include "../platform.h"

instant_t ns_to_microsec(instant_t time){
    return (time / 1000) - ((time / 1000) % 4);
}

/**
 * Pause execution for a number of nanoseconds.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_nanosleep`).
 */
int lf_nanosleep(instant_t requested_time) {
    unsigned int microsec = (unsigned int) requested_time;
    if(microsec < 3){
        delayMicroseconds(3); //Warning: Needs to be >= 3 for precision reasons
    }
    else if(microsec <= 16383){
        delayMicroseconds(microsec);
    }else{
        delay(microsec / 1000);
    }
    return 0;
}

/**
 * Initialize the LF clock.
 */
void lf_initialize_clock() {}

/**
 * Fetch the value of _LF_CLOCK (see lf_linux_support.h) and store it in tp. The
 * timestamp value in 't' will always be epoch time, which is the number of
 * nanoseconds since January 1st, 1970.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_gettime`).
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
