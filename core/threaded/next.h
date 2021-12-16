/**
 * @brief Worker functionality for the threaded runtime of the C target of
 * Lingua Franca. 
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

#ifndef LF_NEXT_H
#define LF_NEXT_H

#include "../utils/pqueue.h"
#include "../tag.h"
#include <stdbool.h>

/**
 * Return the tag of the next event on the event queue.
 * If the event queue is empty then return either FOREVER_TAG
 * or, is a stop_time (timeout time) has been set, the stop time.
 * 
 * @param event_q The event queue.
 */
tag_t get_next_event_tag(pqueue_t* event_q);

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
void lf_next_locked(pqueue_t* event_q, bool keepalive_specified, bool fast);

#endif // LF_NEXT_H