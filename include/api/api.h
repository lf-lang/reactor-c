/**
 * @file
 * @author Edward A. Lee
 * @author Hou Seng (Steven) Wong
 *
 * @section LICENSE
Copyright (c) 2020, The University of California at Berkeley.

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

 * @section DESCRIPTION
 *
 * This file is a translation layer that implements Lingua Franca
 * APIs which interact with the internal _lf_SET and _lf_schedule APIs.
 */

#ifndef API_H
#define API_H

#include <stdio.h> // Included for backwards compatibility so that users do not need to explicitly include this
#include "lf_types.h"
#include "tag.h"

/**
 * Define a macro to suppress warnings about unused variables.
 * Apparently, it is sufficient to just cast to void.
 */
#define SUPPRESS_UNUSED_WARNING(x) (void)(x)

//////////////////////////////////////////////////////////////
/////////////  Schedule Functions

/**
 * Schedule an action to occur with the specified value and time offset
 * with no payload (no value conveyed).
 * See lf_schedule_token(), which this uses, for details.
 *
 * @param action Pointer to an action on the self struct.
 * @param offset The time offset over and above that in the action.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule(void* action, interval_t offset);

/**
 * Schedule the specified action with an integer value at a later logical
 * time that depends on whether the action is logical or physical and
 * what its parameter values are. This wraps a copy of the integer value
 * in a token. See lf_schedule_token() for more details.
 *
 * @param action The action to be triggered.
 * @param extra_delay Extra offset of the event release above that in the action.
 * @param value The value to send.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule_int(void* action, interval_t extra_delay, int value);

/**
 * Schedule the specified action with the specified token as a payload.
 * This will trigger an event at a later logical time that depends
 * on whether the action is logical or physical and what its parameter
 * values are.
 *
 * logical action: A logical action has an offset (default is zero)
 * and a minimum interarrival time (MIT), which also defaults to zero.
 * The logical time at which this scheduled event will trigger is
 * the current time plus the offset plus the delay argument given to
 * this function. If, however, that time is not greater than a prior
 * triggering of this logical action by at least the MIT, then the
 * one of two things can happen depending on the policy specified
 * for the action. If the action's policy is DROP (default), then the
 * action is simply dropped and the memory pointed to by value argument
 * is freed. If the policy is DEFER, then the time will be increased
 * to equal the time of the most recent triggering plus the MIT.
 *
 * For the above, "current time" means the logical time of the
 * reaction that is calling this function. Logical actions should
 * always be scheduled within a reaction invocation, never asynchronously
 * from the outside. FIXME: This needs to be checked.
 *
 * physical action: A physical action has all the same parameters
 * as a logical action, but its timestamp will be the larger of the
 * current physical time and the time it would be assigned if it
 * were a logical action.
 *
 * There are three conditions under which this function will not
 * actually put an event on the event queue and decrement the reference count
 * of the token (if there is one), which could result in the payload being
 * freed. In all three cases, this function returns 0. Otherwise,
 * it returns a handle to the scheduled trigger, which is an integer
 * greater than 0.
 *
 * The first condition is that stop() has been called and the time offset
 * of this event is greater than zero.
 * The second condition is that the logical time of the event
 * is greater that the stop time (timeout) that is specified in the target
 * properties or on the command line.
 * The third condition is that the trigger argument is null.
 *
 * @param action The action to be triggered.
 * @param extra_delay Extra offset of the event release above that in the action.
 * @param token The token to carry the payload or null for no payload.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule_token(void* action, interval_t extra_delay, lf_token_t* token);

/**
 * Schedule an action to occur with the specified value and time offset with a
 * copy of the specified value. If the value is non-null, then it will be copied
 * into newly allocated memory under the assumption that its size is given in
 * the trigger's token object's element_size field multiplied by the specified
 * length.
 *
 * See lf_schedule_token(), which this uses, for details.
 *
 * @param action Pointer to an action on a self struct.
 * @param offset The time offset over and above that in the action.
 * @param value A pointer to the value to copy.
 * @param length The length, if an array, 1 if a scalar, and 0 if value is NULL.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for
 *  error.
 */
trigger_handle_t lf_schedule_copy(void* action, interval_t offset, void* value, int length);

/**
 * Variant of lf_schedule_token that creates a token to carry the specified value.
 * The value is required to be malloc'd memory with a size equal to the
 * element_size of the specified action times the length parameter.
 *
 * See lf_schedule_token(), which this uses, for details.
 *
 * @param action The action to be triggered.
 * @param extra_delay Extra offset of the event release above that in the
 *  action.
 * @param value Dynamically allocated memory containing the value to send.
 * @param length The length of the array, if it is an array, or 1 for a scalar
 *  and 0 for no payload.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for
 *  error.
 */
trigger_handle_t lf_schedule_value(void* action, interval_t extra_delay, void* value, int length);

/**
 * Check the deadline of the currently executing reaction against the
 * current physical time. If the deadline has passed, invoke the deadline
 * handler (if invoke_deadline_handler parameter is set true) and return true.
 * Otherwise, return false.
 *
 * @param self The self struct of the reactor.
 * @param invoke_deadline_handler When this is set true, also invoke deadline
 *  handler if the deadline has passed.
 * @return True if the specified deadline has passed and false otherwise.
 */
bool lf_check_deadline(void* self, bool invoke_deadline_handler);

/**
 * Compare two tags. Return -1 if the first is less than
 * the second, 0 if they are equal, and +1 if the first is
 * greater than the second. A tag is greater than another if
 * its time is greater or if its time is equal and its microstep
 * is greater.
 * @param tag1
 * @param tag2
 * @return -1, 0, or 1 depending on the relation.
 */
int lf_tag_compare(tag_t tag1, tag_t tag2);

/**
 * Return the current tag of a reactor. If NULL is passed to this function it
 * will return the "global tag" of the runtime.  
 * @param self A pointer to the environment of which you want the current tag
 * @return the current tag 
 */
tag_t lf_tag(void* env);

#endif // API_H
