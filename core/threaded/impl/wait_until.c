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

#include "../wait_until.h"
#include "../../tag.h"
#include "../../utils/util.h"

/////////////////// External Variables /////////////////////////
/**
 * The global mutex lock.
 */
extern lf_mutex_t mutex;

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
bool wait_until(instant_t logical_time_ns, lf_cond_t* condition, bool fast) {
    DEBUG_PRINT("-------- Waiting until physical time matches logical time %lld", logical_time_ns);
    bool return_value = true;
    interval_t wait_until_time_ns = logical_time_ns;
#ifdef FEDERATED_DECENTRALIZED // Only apply the STP offset if coordination is decentralized
    // Apply the STP offset to the logical time
    // Prevent an overflow
    if (wait_until_time_ns < FOREVER - _lf_global_time_STP_offset) {
        // If wait_time is not forever
        DEBUG_PRINT("Adding STP offset %lld to wait until time %lld.",
                _lf_global_time_STP_offset,
                wait_until_time_ns - start_time);
        wait_until_time_ns += _lf_global_time_STP_offset;
    }
#endif
    if (!fast) {
        // Get physical time as adjusted by clock synchronization offset.
        instant_t current_physical_time = get_physical_time();
        // We want to wait until that adjusted time matches the logical time.
        interval_t ns_to_wait = wait_until_time_ns - current_physical_time;
        // We should not wait if that adjusted time is already ahead
        // of logical time.
        if (ns_to_wait < MIN_WAIT_TIME) {
            DEBUG_PRINT("Wait time %lld is less than MIN_WAIT_TIME %lld. Skipping wait.",
                ns_to_wait, MIN_WAIT_TIME);
            return return_value;
        }

        // We will use lf_cond_timedwait, which takes as an argument the absolute
        // time to wait until. However, that will not include the offset that we
        // have calculated with clock synchronization. So we need to instead ensure
        // that the time it waits is ns_to_wait.
        // We need the current clock value as obtained using CLOCK_REALTIME because
        // that is what lf_cond_timedwait will use.
        // The above call to setPhysicalTime() set the
        // _lf_last_reported_unadjusted_physical_time_ns to the CLOCK_REALTIME value
        // unadjusted by clock synchronization.
        // Note that if ns_to_wait is large enough, then the following addition could
        // overflow. This could happen, for example, if wait_until_time_ns == FOREVER.
        instant_t unadjusted_wait_until_time_ns = FOREVER;
        if (FOREVER - _lf_last_reported_unadjusted_physical_time_ns > ns_to_wait) {
            unadjusted_wait_until_time_ns = _lf_last_reported_unadjusted_physical_time_ns + ns_to_wait;
        }
        DEBUG_PRINT("-------- Clock offset is %lld ns.", current_physical_time - _lf_last_reported_unadjusted_physical_time_ns);
        DEBUG_PRINT("-------- Waiting %lld ns for physical time to match logical time %llu.", ns_to_wait, 
                logical_time_ns - get_start_time());

        // lf_cond_timedwait returns 0 if it is awakened before the timeout.
        // Hence, we want to run it repeatedly until either it returns non-zero or the
        // current physical time matches or exceeds the logical time.
        if (lf_cond_timedwait(condition, &mutex, unadjusted_wait_until_time_ns) != LF_TIMEOUT) {
            DEBUG_PRINT("-------- wait_until interrupted before timeout.");

            // Wait did not time out, which means that there
            // may have been an asynchronous call to schedule().
            // Continue waiting.
            // Do not adjust current_tag.time here. If there was an asynchronous
            // call to schedule(), it will have put an event on the event queue,
            // and current_tag.time will be set to that time when that event is pulled.
            return_value = false;
        } else {
            // Reached timeout.
            // FIXME: move this to Mac-specific platform implementation
            // Unfortunately, at least on Macs, pthread_cond_timedwait appears
            // to be implemented incorrectly and it returns well short of the target
            // time.  Check for this condition and wait again if necessary.
            interval_t ns_to_wait = wait_until_time_ns - get_physical_time();
            // We should not wait if that adjusted time is already ahead
            // of logical time.
            if (ns_to_wait < MIN_WAIT_TIME) {
                return true;
            }
            DEBUG_PRINT("-------- lf_cond_timedwait claims to have timed out, "
                    "but it did not reach the target time. Waiting again.");
            return wait_until(wait_until_time_ns, condition, fast);
        }

        DEBUG_PRINT("-------- Returned from wait, having waited %lld ns.", get_physical_time() - current_physical_time);
    }
    return return_value;
}