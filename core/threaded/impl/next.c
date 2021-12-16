/**
 * @brief Worker functionality for the threaded runtime of the C target of
 * Lingua Franca. 
 * 
 * @file worker.c
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

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "../next.h"
#include "../wait_until.h"
#include "../scheduler.h"
#include "../../reactor.h"
#include "../../tag.h"
#include "../../utils/util.h"
#include "../../platform.h"

/////////////////// External Variables /////////////////////////
/**
 * The global mutex lock.
 */
extern lf_mutex_t mutex;

/**
 * Global condition variable used for notification between threads.
 */
extern lf_cond_t event_q_changed;

/////////////////// External Functions /////////////////////////
/**
 * For the specified reaction, if it has produced outputs, insert the
 * resulting triggered reactions into the reaction queue.
 * This procedure assumes the mutex lock is NOT held and grabs
 * the lock only when it actually inserts something onto the reaction queue.
 * @param reaction The reaction that has just executed.
 * @param worker The thread number of the worker thread or 0 for unthreaded execution (for tracing).
 */
void schedule_output_reactions(reaction_t* reaction, int worker);

#ifdef FEDERATED_CENTRALIZED
// The following is defined in federate.c and used in the following function.
tag_t _lf_send_next_event_tag(tag_t tag, bool wait_for_reply);
#endif

/**
 * In a federated execution with centralized coordination, this function returns
 * a tag that is less than or equal to the specified tag when, as far
 * as the federation is concerned, it is safe to commit to advancing
 * to the returned tag. That is, all incoming network messages with
 * tags less than the returned tag have been received.
 * In unfederated execution or in federated execution with decentralized
 * control, this function returns the specified tag immediately.
 *
 * @param tag The tag to which to advance.
 * @param wait_for_reply If true, wait for the RTI to respond.
 * @return The tag to which it is safe to advance.
 */
tag_t send_next_event_tag(tag_t tag, bool wait_for_reply) {
#ifdef FEDERATED_CENTRALIZED
    return _lf_send_next_event_tag(tag, wait_for_reply);
#else
    return tag;
#endif
}

///////////////////////////////////////////////////////////////

/**
 * Return the tag of the next event on the event queue.
 * If the event queue is empty then return either FOREVER_TAG
 * or, is a stop_time (timeout time) has been set, the stop time.
 * 
 * @param event_q The event queue.
 */
tag_t get_next_event_tag(pqueue_t* event_q) {
    // Peek at the earliest event in the event queue.
    event_t* event = (event_t*)pqueue_peek(event_q);
    tag_t next_tag = FOREVER_TAG;
    tag_t current_tag = get_current_tag();
    if (event != NULL) {
        // There is an event in the event queue.
        if (event->time < current_tag.time) {
            error_print_and_exit("get_next_event_tag(): Earliest event on the event queue (%lld) is "
                                  "earlier than the current time (%lld).",
                                  event->time - get_start_time(),
                                  current_tag.time - get_start_time());
        }

        next_tag.time = event->time;
        if (next_tag.time == current_tag.time) {
        	DEBUG_PRINT("Earliest event matches current time. Incrementing microstep. Event is dummy: %d.",
        			event->is_dummy);
            next_tag.microstep =  get_microstep() + 1;
        } else {
            next_tag.microstep = 0;
        }
    }

    // If a timeout tag was given, adjust the next_tag from the
    // event tag to that timeout tag.
    if (_lf_is_tag_after_stop_tag(next_tag)) {
        next_tag = get_stop_tag();
    }
    LOG_PRINT("Earliest event on the event queue (or stop time if empty) is (%lld, %u). Event queue has size %d.",
            next_tag.time - get_start_time(), next_tag.microstep, pqueue_size(event_q));
    return next_tag;
}

/**
 * If there is at least one event in the event queue, then wait until
 * physical time matches or exceeds the time of the least tag on the event
 * queue; pop the next event(s) from the event queue that all have the same tag;
 * extract from those events the reactions that are to be invoked at this
 * logical time and insert them into the reaction queue. The event queue is
 * sorted by time tag.
 *
 * If there is no event in the queue and the keepalive command-line option was
 * not given, and this is not a federated execution with centralized coordination,
 * set the stop tag to the current tag.
 * If keepalive was given, then wait for either request_stop()
 * to be called or an event appears in the event queue and then return.
 *
 * Every time tag is advanced, it is checked against stop tag and if they are
 * equal, shutdown reactions are triggered.
 *
 * This does not acquire the mutex lock. It assumes the lock is already held.
 * 
 * @param event_q The event queue.
 * @param keepalive_specified Whether to exit if there are no more events.
 * @param fast Whether to use fast mode, which does not wait in physical time.
 */
void lf_next_locked(pqueue_t* event_q, bool keepalive_specified, bool fast) {
    // Previous logical time is complete.
    tag_t next_tag = get_next_event_tag(event_q);
    tag_t current_tag = get_current_tag();

#ifdef FEDERATED_CENTRALIZED
    // In case this is in a federation with centralized coordination, notify 
    // the RTI of the next earliest tag at which this federate might produce 
    // an event. This function may block until it is safe to advance the current 
    // tag to the next tag. Specifically, it blocks if there are upstream 
    // federates. If an action triggers during that wait, it will unblock
    // and return with a time (typically) less than the next_time.
    tag_t grant_tag = send_next_event_tag(next_tag, true); // true means this blocks.
    if (compare_tags(grant_tag, next_tag) < 0) {
        // RTI has granted tag advance to an earlier tag or the wait
        // for the RTI response was interrupted by a local physical action with
        // a tag earlier than requested.
        // Continue executing. The event queue may have changed.
        return;
    }
    // Granted tag is greater than or equal to next event tag that we sent to the RTI.
    // Since send_next_event_tag releases the mutex lock internally, we need to check
    // again for what the next tag is (e.g., the stop time could have changed).
    next_tag = get_next_event_tag(event_q);
    
    // FIXME: Do starvation analysis for centralized coordination.
    // Specifically, if the event queue is empty on *all* federates, this
    // can become known to the RTI which can then stop execution.
    // Hence, it will no longer be necessary to force keepalive to be true
    // for all federated execution. With centralized coordination, we could
    // allow keepalive to be either true or false and could get the same
    // behavior with centralized coordination as with unfederated execution.

#else  // not FEDERATED_CENTRALIZED
    if (pqueue_peek(event_q) == NULL && !keepalive_specified) {
        // There is no event on the event queue and keepalive is false.
        // No event in the queue
        // keepalive is not set so we should stop.
        // Note that federated programs with decentralized coordination always have
        // keepalive = true
        _lf_set_stop_tag((tag_t){.time=current_tag.time,.microstep=current_tag.microstep+1});

        // Stop tag has changed. Need to check next_tag again.
        next_tag = get_next_event_tag(event_q);
    }
#endif

    // Wait for physical time to advance to the next event time (or stop time).
    // This can be interrupted if a physical action triggers (e.g., a message
    // arrives from an upstream federate or a local physical action triggers).
    LOG_PRINT("Waiting until elapsed time %lld.", (next_tag.time - get_start_time()));
    while (!wait_until(next_tag.time, &event_q_changed, fast)) {
        DEBUG_PRINT("_lf_next_locked(): Wait until time interrupted.");
        // Sleep was interrupted.  Check for a new next_event.
        // The interruption could also have been due to a call to request_stop().
        next_tag = get_next_event_tag(event_q);

        // If this (possibly new) next tag is past the stop time, return.
        if (_lf_is_tag_after_stop_tag(next_tag)) {
            return;
        }
    }
    // A wait occurs even if wait_until() returns true, which means that the
    // tag on the head of the event queue may have changed.
    next_tag = get_next_event_tag(event_q);

    // If this (possibly new) next tag is past the stop time, return.
    if (_lf_is_tag_after_stop_tag(next_tag)) { // compare_tags(tag, stop_tag) > 0
        return;
    }

    DEBUG_PRINT("Physical time is ahead of next tag time by %lld. This should be small unless -fast is used.",
                get_physical_time() - next_tag.time);
    
#ifdef FEDERATED
    // In federated execution (at least under decentralized coordination),
    // it is possible that an incoming message has been partially read,
    // enough to see its tag. To prevent it from becoming tardy, the thread
    // that is reading the message has set a barrier to prevent logical time
    // from exceeding the timestamp of the message. It will remove that barrier
    // once the complete message has been read. Here, we wait for that barrier
    // to be removed, if appropriate.
    if(_lf_wait_on_global_tag_barrier(next_tag)) {
        // A wait actually occurred, so the next_tag may have changed again.
        next_tag = get_next_event_tag();
    }
#endif // FEDERATED

    // If the first event in the event queue has a tag greater than or equal to the
    // stop time, and the current_tag matches the stop tag (meaning that we have already
    // executed microstep 0 at the timeout time), then we are done. The above code prevents the next_tag
    // from exceeding the stop_tag, so we have to do further checks if
    // they are equal.
    if (compare_tags(next_tag, get_stop_tag()) >= 0 && compare_tags(current_tag, get_stop_tag()) >= 0) {
        // If we pop anything further off the event queue with this same time or larger,
        // then it will be assigned a tag larger than the stop tag.
        return;
    }

    // Invoke code that must execute before starting a new logical time round,
    // such as initializing outputs to be absent.
    _lf_start_time_step();
        
    // At this point, finally, we have an event to process.
    // Advance current time to match that of the first event on the queue.
    _lf_advance_logical_time(next_tag.time);

    if (compare_tags(get_current_tag(), get_stop_tag()) >= 0) {
        // Pop shutdown events
        DEBUG_PRINT("Scheduling shutdown reactions.");
        _lf_trigger_shutdown_reactions();
    }

    // Pop all events from event_q with timestamp equal to current_tag.time,
    // extract all the reactions triggered by these events, and
    // stick them into the reaction queue.
    _lf_pop_events();
}
