/**
 * @file
 * @author Edward A. Lee
 * @author Hou Seng (Steven) Wong
 * @author Soroush Bateni
 *
 * @brief API functions for scheduling actions.
 * @ingroup API
 *
 * Most of these functions take a `void*` pointer to an action, which will be internally cast to
 * a `lf_action_base_t*` pointer. The cast could be done by macros in reaction_macros.h, but unlike
 * the macros defined there, it is common for `lf_schedule` functions to be invoked outside of reaction
 * bodies.  This means that users writing code in separate library files are responsible for ensuring that
 * the `void*` pointer is indeed a valid `lf_action_base_t*` pointer before passing it to `lf_schedule`.
 * The compiler will not check this.
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "lf_types.h"
#include "tag.h"

/**
 * @brief Schedule an action to occur with the specified time offset with no payload (no value conveyed).
 * @ingroup API
 *
 * The later tag will depend on whether the action is logical or physical. If it is logical,
 * the time of the event will be the current logical time of the environment associated with
 * the action plus the minimum delay of the action plus the extra delay. If that time is equal
 * to the current time, then the tag will be one microstep beyond the current tag.
 * If the action is physical, the time will be the current physical time plus the extra delay,
 * and the microstep will be zero.
 *
 * See lf_schedule_token(), which this uses, for details.
 *
 * @param action The action to be triggered (a pointer to an `lf_action_base_t`).
 * @param offset The time offset over and above the minimum delay of the action.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule(void* action, interval_t offset);

/**
 * @brief Schedule the specified action with an integer value at a later logical time.
 * @ingroup API
 *
 * The later tag will depend on whether the action is logical or physical. If it is logical,
 * the time of the event will be the current logical time of the environment associated with
 * the action plus the minimum delay of the action plus the extra delay. If that time is equal
 * to the current time, then the tag will be one microstep beyond the current tag.
 * If the action is physical, the time will be the current physical time plus the extra delay,
 * and the microstep will be zero.
 *
 * This wraps a copy of the integer value in a token. See lf_schedule_token() for more details.
 *
 * @param action The action to be triggered (a pointer to an `lf_action_base_t`).
 * @param extra_delay Extra offset of the event release above that in the action.
 * @param value The value to send.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule_int(void* action, interval_t extra_delay, int value);

/**
 * @brief Schedule the specified action at a later tag with the specified token as a payload.
 * @ingroup API
 *
 * The later tag will depend on whether the action is logical or physical. If it is logical,
 * the time of the event will be the current logical time of the environment associated with
 * the action plus the minimum delay of the action plus the extra delay. If that time is equal
 * to the current time, then the tag will be one microstep beyond the current tag.
 * If the action is physical, the time will be the current physical time plus the extra delay,
 * and the microstep will be zero.
 *
 * In both cases, if the resulting tag of the event conincides with a previously scheduled event
 * for the same action, then, by default, a microstep will be added to the tag until there is no
 * colliding event. This behavior can be changed by specifying a minimum spacing and a policy,
 * as explained below.
 *
 * An action will trigger at a logical time that depends on the `extra_delay` argument given to
 * this schedule function, the `<min_delay>`, `<min_spacing>`, and `<policy>` arguments in the
 * action declaration, and whether the action is physical or logical.
 *
 * For a `logical` action `a`, the tag assigned to the event is computed as follows.
 * First, let _t_ be the _current logical time_. For a logical action, _t_ is just the logical
 * time at which the reaction calling `schedule()` is called. The **preliminary time** of the
 * action is then just _t_ + `<min_delay>` + `<offset>`. This preliminary time may be further
 * modified, as explained below.
 *
 * For a **physical** action, the preliminary time is similar, except that _t_ is replaced by
 * the current _physical_ time _T_ when `schedule()` is called.
 *
 * If no `<min_spacing>` has been declared, then the tag of the event is simply the preliminary time
 * unless there is already an event scheduled for the same action with the same tag.
 * In that case, a microstep is added to the tag. If there is again a previously scheduled
 * event with the same tag, then a microstep is added to the tag again.
 * This process is repeated until there is no previously scheduled event with the same tag.
 *
 * If a `<min_spacing>` has been declared, then it gives a minimum logical time
 * interval between the tags of two subsequently scheduled events. The first effect this
 * has is that events will have monotically increasing tags. The difference between the
 * times of two successive tags is at least `<min_spacing>`. If the
 * preliminary time is closer than `<min_spacing>` to the time of the previously
 * scheduled event (if there is one), or if the preliminary time is earlier than
 * the previously scheduled event, then the time will be modified to enforce
 * the minimum spacing. The `<policy>` argument  determines how the minimum spacing
 * constraint is enforced.
 *
 * Note that "previously scheduled" here means specifically the tag resulting from
 * the most recent call to the schedule function for the same action.
 *
 * A `<min_spacing>` of 0 is not quite the same as no `<min_spacing>` declared.
 * With a `<min_spacing>` of 0, events will still have monotically increasing tags,
 * but the difference between the times of two successive tags can be 0.
 *
 * The `<policy>` is one of the following:
 *
 * - `"defer"`: (**the default**) The event is added to the event queue with a tag that is
 *    equal to earliest time that satisfies the minimal spacing requirement. Assuming the
 *    time of the preceding event is _t_prev_, then the tag of the new event simply becomes
 *    _t_prev_ + `<min_spacing>`.
 * - `"drop"`: The new event is dropped and `schedule()` returns without having modified the event queue.
 * - `"replace"`: The payload (if any) of the new event is assigned to the preceding event
 *     if it is still pending in the event queue; no new event is added to the event queue
 *     in this case. If the preceding event has already been pulled from the event queue,
 *     the default `"defer"` policy is applied.
 *
 * Note that while the `"defer"` policy is conservative in the sense that it does not discard events,
 * it could potentially cause an unbounded growth of the event queue.
 *
 * For example, suppose the minimum spacing of a logical action is 10 ms and the policy is `"defer"`.
 * Suppose that in a reaction to `startup`, the logical action is scheduled with a delay of
 * 100 ms, then again with a delay of 99 ms, and a third time with a delay of 101 ms.
 * The logical action will trigger at elapsed times 100 ms, 110 ms, and 120 ms.
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
 * @param action The action to be triggered (a pointer to an `lf_action_base_t`).
 * @param extra_delay Extra offset of the event release above that in the action.
 * @param token The token to carry the payload or null for no payload.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule_token(void* action, interval_t extra_delay, lf_token_t* token);

/**
 * @brief Schedule an action to occur with the specified value and time offset with a
 * copy of the specified value.
 * @ingroup API
 *
 * If the value is non-null, then it will be copied
 * into newly allocated memory under the assumption that its size is given in
 * the trigger's token object's element_size field multiplied by the specified
 * length.
 *
 * The later tag will depend on whether the action is logical or physical. If it is logical,
 * the time of the event will be the current logical time of the environment associated with
 * the action plus the minimum delay of the action plus the extra delay. If that time is equal
 * to the current time, then the tag will be one microstep beyond the current tag.
 * If the action is physical, the time will be the current physical time plus the extra delay,
 * and the microstep will be zero.
 *
 * See @ref lf_schedule_token(), which this uses, for details.
 *
 * @param action The action to be triggered (a pointer to an `lf_action_base_t`).
 * @param offset The time offset over and above that in the action.
 * @param value A pointer to the value to copy.
 * @param length The length, if an array, 1 if a scalar, and 0 if value is NULL.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for
 *  error.
 */
trigger_handle_t lf_schedule_copy(void* action, interval_t offset, void* value, size_t length);

/**
 * @brief Variant of lf_schedule_token that creates a token to carry the specified value.
 * @ingroup API
 *
 * The value is required to be malloc'd memory with a size equal to the
 * element_size of the specified action times the length parameter.
 *
 * See @ref lf_schedule_token(), which this uses, for details.
 *
 * @param action The action to be triggered (a pointer to an `lf_action_base_t`).
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
 * @brief Schedule the specified trigger to execute in the specified environment with given delay and token.
 * @ingroup Internal
 *
 * This is the most flexible version of the schedule functions and is used in the implementation
 * of many of the others. End users would rarely use it.
 *
 * This will schedule the specified trigger at env->current_tag.time plus the offset of the
 * specified trigger plus the delay. The value is required to be either
 * NULL or a pointer to a token wrapping the payload. The token carries
 * a reference count, and when the reference count decrements to 0,
 * the will be freed. Hence, it is essential that the payload be in
 * memory allocated using malloc.
 *
 * There are several conditions under which this function will not
 * actually put an event on the event queue and decrement the reference count
 * of the token (if there is one), which could result in the payload being
 * freed. In all cases, this function returns 0. Otherwise,
 * it returns a handle to the scheduled trigger, which is an integer
 * greater than 0.
 *
 * The first condition is that a stop has been requested and the trigger
 * offset plus the extra delay is greater than zero.
 * The second condition is that the trigger offset plus the extra delay
 * is greater that the requested stop time (timeout).
 * A third condition is that the trigger argument is null.
 * Also, an event might not be scheduled if the trigger is an action
 * with a `min_spacing` parameter.  See the documentation.

 * @param env The environment in which to schedule the event.
 * @param trigger The action or timer to be triggered.
 * @param delay Offset of the event release.
 * @param token The token payload.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule_trigger(environment_t* env, trigger_t* trigger, interval_t delay, lf_token_t* token);

/**
 * @brief Check the deadline of the currently executing reaction against the
 * current physical time.
 * @ingroup API
 *
 * If the deadline has passed, invoke the deadline
 * handler (if invoke_deadline_handler parameter is set true) and return true.
 * Otherwise, return false.
 *
 * This function is intended to be used within a reaction that has been invoked without a deadline
 * violation, but that wishes to check whether the deadline gets violated _during_ the execution of
 * the reaction. This can be used, for example, to implement a timeout mechanism that bounds the
 * execution time of a reaction, for example to realize an "anytime" computation.
 *
 * @param self The self struct of the reactor.
 * @param invoke_deadline_handler When this is set true, also invoke deadline
 *  handler if the deadline has passed.
 * @return True if the specified deadline has passed and false otherwise.
 */
bool lf_check_deadline(void* self, bool invoke_deadline_handler);

/**
 * @brief Update the deadline of the currently executing reaction.
 * @ingroup API
 *
 * This function allows the deadline of the current reaction to be adjusted dynamically at runtime.
 * It can be useful in scenarios where timing requirements change depending on system conditions.
 *
 * Updating the deadline with this function does not affect the deadline check that was performed
 * when the reaction started. Lingua Franca checks deadlines only at the beginning of each reaction.
 * Therefore, if you need to confirm whether the newly updated deadline has been violated during the current execution,
 * this can be done by invoking lf_check_deadline() immediately after this function.
 *
 * @param self The self struct of the reactor.
 * @param updated_deadline The updated deadline.
 */
void lf_update_deadline(void* self, interval_t updated_deadline);

#endif // SCHEDULE_H
