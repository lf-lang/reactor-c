/**
 * @brief API to allow the runtime to wait for a certain amount of time.
 * 
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * 
 * @copyright Copyright (c) 2021, The University of California at Berkeley.
 * 
 */

/*************
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

#ifndef LF_WAIT_UNTIL_H
#define LF_WAIT_UNTIL_H

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "../platform.h"

/**
 * Unless the "fast" option is given, an LF program will wait until
 * physical time matches logical time before handling an event with
 * a given logical time. The amount of time is less than this given
 * threshold, then no wait will occur. The purpose of this is
 * to prevent unnecessary delays caused by simply setting up and
 * performing the wait.
 */
#define MIN_WAIT_TIME USEC(10)

/**
 * Wait until physical time matches or exceeds the specified logical time,
 * unless -fast is given.
 *
 * If an event is put on the event queue during the wait, then the wait is
 * interrupted and this function returns false. It also returns false if the
 * timeout time is reached before the wait has completed.
 * 
 * The mutex lock is assumed to be held by the calling thread.
 * Note this this could return true even if the a new event
 * was placed on the queue if that event time matches or exceeds
 * the specified time.
 *
 * @param logical_time_ns Logical time to wait until physical time matches it.
 * @param return_if_interrupted If this is false, then wait_util will wait
 *  until physical time matches the logical time regardless of whether new
 *  events get put on the event queue. This is useful, for example, for
 *  synchronizing the start of the program.
 * @param fast Whether to use fast mode, which does not wait in physical time.
 * 
 * @return Return false if the wait is interrupted either because of an event
 *  queue signal or if the wait time was interrupted early by reaching
 *  the stop time, if one was specified. Return true if the full wait time
 *  was reached.
 */
bool wait_until(instant_t logical_time_ns, lf_cond_t* condition, bool fast);

#endif // LF_WAIT_UNTIL_H