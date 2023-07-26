#if defined(LF_THREADED)
/*************
Copyright (c) 2022, The University of Texas at Dallas.
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

/**
 * @file scheduler_sync_tag_advance.c
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @author Edward A. Lee <eal@berkeley.edu>
 * @author Marten Lohstroh <marten@berkeley.edu>
 * @brief API used to advance tag globally.
 *
 * @copyright Copyright (c) 2022, The University of Texas at Dallas.
 * @copyright Copyright (c) 2022, The University of California at Berkeley.
 */

#include "scheduler_sync_tag_advance.h"
#include "environment.h"
#include "trace.h"
#include "util.h"

/////////////////// External Functions /////////////////////////
/**
 * Placeholder for function that will advance tag and initially fill the
 * reaction queue.
 *
 * This does not acquire the mutex lock. It assumes the lock is already held.
 */

/**
 * @brief Indicator that execution of at least one tag has completed.
 */
static bool _lf_logical_tag_completed = false;

/**
 * Return true if the worker should stop now; false otherwise.
 * This function assumes the caller holds the mutex lock.
 */
bool should_stop_locked(lf_scheduler_t * sched) {
    // If this is not the very first step, check against the stop tag to see whether this is the last step.
    if (_lf_logical_tag_completed) {
        // If we are at the stop tag, do not call _lf_next_locked()
        // to prevent advancing the logical time.
        if (lf_tag_compare(sched->env->current_tag, sched->env->stop_tag) >= 0) {
            return true;
        }
    }
    return false;
}

/**
 * Advance tag. This will also pop events for the newly acquired tag and put
 * the triggered reactions on the '_lf_sched_vector_of_reaction_qs'.
 *
 * This function assumes the caller holds the 'mutex' lock.
 *
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_advance_tag_locked(lf_scheduler_t * sched) {
    environment_t* env = sched->env;
    logical_tag_complete(env->current_tag);

    if (should_stop_locked(sched)) {
        return true;
    }

    _lf_logical_tag_completed = true;

    // Advance time.
    // _lf_next_locked() may block waiting for real time to pass or events to appear.
    // to appear on the event queue. Note that we already
    tracepoint_scheduler_advancing_time_starts(env->trace);
    _lf_next_locked(env);
    tracepoint_scheduler_advancing_time_ends(env->trace);

    LF_PRINT_DEBUG("Scheduler: Done waiting for _lf_next_locked().");
    return false;
}
#endif
