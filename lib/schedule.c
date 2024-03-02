/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Implementation of schedule functions for Lingua Franca programs.
 */

#include "schedule.h"
#include "reactor.h"
#include "reactor_common.h"
#include "environment.h"

#include <assert.h>
#include <string.h> // Defines memcpy.

trigger_handle_t lf_schedule(void* action, interval_t offset) {
    return lf_schedule_token((lf_action_base_t*)action, offset, NULL);
}

trigger_handle_t lf_schedule_int(void* action, interval_t extra_delay, int value) {
    token_template_t* template = (token_template_t*)action;

    // NOTE: This doesn't acquire the mutex lock in the multithreaded version
    // until schedule_value is called. This should be OK because the element_size
    // does not change dynamically.
    if (template->type.element_size != sizeof(int)) {
        lf_print_error("Action type is not an integer. element_size is %zu", template->type.element_size);
        return -1;
    }
    int* container = (int*)malloc(sizeof(int));
    *container = value;
    return lf_schedule_value(action, extra_delay, container, 1);
}

trigger_handle_t lf_schedule_token(void* action, interval_t extra_delay, lf_token_t* token) {
    environment_t* env = ((lf_action_base_t*)action)->parent->environment;
    
    LF_CRITICAL_SECTION_ENTER(env);
    int return_value = lf_schedule_trigger(env, ((lf_action_base_t*)action)->trigger, extra_delay, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    LF_CRITICAL_SECTION_EXIT(env);
    return return_value;
}

trigger_handle_t lf_schedule_copy(void* action, interval_t offset, void* value, size_t length) {
    if (length < 0) {
        lf_print_error(
            "schedule_copy():"
            " Ignoring request to copy a value with a negative length (%zu).",
            length
        );
        return -1;
    }
    if (value == NULL) {
        return lf_schedule_token(action, offset, NULL);
    }
    environment_t* env = ((lf_action_base_t*)action)->parent->environment;
    token_template_t* template = (token_template_t*)action;
    if (action == NULL || template->type.element_size <= 0) {
        lf_print_error("schedule: Invalid element size.");
        return -1;
    }
    LF_CRITICAL_SECTION_ENTER(env);
    // Initialize token with an array size of length and a reference count of 0.
    lf_token_t* token = _lf_initialize_token(template, length);
    // Copy the value into the newly allocated memory.
    memcpy(token->value, value, template->type.element_size * length);
    // The schedule function will increment the reference count.
    trigger_handle_t result = lf_schedule_trigger(env, ((lf_action_base_t*)action)->trigger, offset, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    LF_CRITICAL_SECTION_EXIT(env);
    return result;
}

trigger_handle_t lf_schedule_value(void* action, interval_t extra_delay, void* value, int length) {
    if (length < 0) {
        lf_print_error(
            "schedule_value():"
            " Ignoring request to schedule an action with a value that has a negative length (%d).",
            length
        );
        return -1;
    }
    token_template_t* template = (token_template_t*)action;
    environment_t* env = ((lf_action_base_t*)action)->parent->environment;
    LF_CRITICAL_SECTION_ENTER(env);
    lf_token_t* token = _lf_initialize_token_with_value(template, value, length);
    int return_value = lf_schedule_trigger(env, ((lf_action_base_t*)action)->trigger, extra_delay, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    LF_CRITICAL_SECTION_EXIT(env);
    return return_value;
}

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
bool lf_check_deadline(void* self, bool invoke_deadline_handler) {
    reaction_t* reaction = ((self_base_t*)self)->executing_reaction;
    if (lf_time_physical() > (lf_time_logical(((self_base_t *)self)->environment) + reaction->deadline)) {
        if (invoke_deadline_handler) {
            reaction->deadline_violation_handler(self);
        }
        return true;
    }
    return false;
}

trigger_handle_t lf_schedule_trigger(environment_t *env, trigger_t* trigger, interval_t extra_delay, lf_token_t* token) {
    assert(env != GLOBAL_ENVIRONMENT);
    if (lf_is_tag_after_stop_tag(env, env->current_tag)) {
        // If schedule is called after stop_tag
        // This is a critical condition.
        _lf_done_using(token);
        lf_print_warning("lf_schedule() called after stop tag.");
        return 0;
    }

    if (extra_delay < 0LL) {
        lf_print_warning("schedule called with a negative extra_delay " PRINTF_TIME ". Replacing with zero.", extra_delay);
        extra_delay = 0LL;
    }

    LF_PRINT_DEBUG("lf_schedule_trigger: scheduling trigger %p with delay " PRINTF_TIME " and token %p.",
            trigger, extra_delay, token);

    // Increment the reference count of the token.
    if (token != NULL) {
        token->ref_count++;
        LF_PRINT_DEBUG("lf_schedule_trigger: Incremented ref_count of %p to %zu.",
                token, token->ref_count);
    }

    // The trigger argument could be null, meaning that nothing is triggered.
    // Doing this after incrementing the reference count ensures that the
    // payload will be freed, if there is one.
    if (trigger == NULL) {
        _lf_done_using(token);
        return 0;
    }

    // Compute the tag (the logical timestamp for the future event).
    // We first do this assuming it is logical action and then, if it is a
    // physical action, modify it if physical time exceeds the result.
    interval_t delay = extra_delay;
    // Add the offset if this is not a timer because, in that case,
    // it is the minimum delay.
    if (!trigger->is_timer) {
        delay += trigger->offset;
    }
    tag_t intended_tag = (tag_t){.time = env->current_tag.time + delay, .microstep = 0};
    
    LF_PRINT_DEBUG("lf_schedule_trigger: env->current_tag.time = " PRINTF_TIME ". Total logical delay = " PRINTF_TIME "",
            env->current_tag.time, delay);
    interval_t min_spacing = trigger->period;

    event_t* e = lf_get_new_event(env);

    // Initialize the next pointer.
    e->next = NULL;

    // Set the payload.
    e->token = token;

    // Make sure the event points to this trigger so when it is
    // dequeued, it will trigger this trigger.
    e->trigger = trigger;

    // If the trigger is physical, then we need to check whether
    // physical time is larger than the intended time and, if so,
    // modify the intended time.
    if (trigger->is_physical) {
        // Get the current physical time and assign it as the intended time.
        intended_tag.time = lf_time_physical() + delay;
    } else {
        // FIXME: We need to verify that we are executing within a reaction?
        // See reactor_threaded.
        // If a logical action is scheduled asynchronously (which should never be
        // done) the computed tag can be smaller than the current tag, in which case
        // it needs to be adjusted.
        // FIXME: This can go away once:
        // - we have eliminated the possibility to have a negative additional delay; and
        // - we detect the asynchronous use of logical actions
        #ifndef NDEBUG
        if (intended_tag.time < env->current_tag.time) {
            lf_print_warning("Attempting to schedule an event earlier than current time by " PRINTF_TIME " nsec! "
                    "Revising to the current time " PRINTF_TIME ".",
                    env->current_tag.time - intended_tag.time, env->current_tag.time);
            intended_tag.time = env->current_tag.time;
        }
        #endif
    }

#ifdef FEDERATED_DECENTRALIZED
    // Event inherits the original intended_tag of the trigger
    // set by the network stack (or the default, which is (NEVER,0))
    e->intended_tag = trigger->intended_tag;
#endif

    // Check for conflicts (a queued event with the same trigger and time).
    if (min_spacing <= 0) {
        // No minimum spacing defined.
        e->time = intended_tag.time;
        event_t* found = (event_t *)pqueue_find_equal_same_priority(env->event_q, e);
        // Check for conflicts. Let events pile up in super dense time.
        if (found != NULL) {
            intended_tag.microstep++;
            // Skip to the last node in the linked list.
            while(found->next != NULL) {
                found = found->next;
                intended_tag.microstep++;
            }
            if (lf_is_tag_after_stop_tag(env, intended_tag)) {
                LF_PRINT_DEBUG("Attempt to schedule an event after stop_tag was rejected.");
                // Scheduling an event will incur a microstep
                // after the stop tag.
                lf_recycle_event(env, e);
                return 0;
            }
            // Hook the event into the list.
            found->next = e;
            trigger->last_tag = intended_tag;
            return(0); // FIXME: return value
        }
        // If there are not conflicts, schedule as usual. If intended time is
        // equal to the current logical time, the event will effectively be
        // scheduled at the next microstep.
    } else if (!trigger->is_timer && trigger->last_tag.time != NEVER) {
        // There is a min_spacing and there exists a previously
        // scheduled event. It determines the
        // earliest time at which the new event can be scheduled.
        // Check to see whether the event is too early.
        instant_t earliest_time = trigger->last_tag.time + min_spacing;
        LF_PRINT_DEBUG("There is a previously scheduled event; earliest possible time "
                "with min spacing: " PRINTF_TIME,
                earliest_time);
        // If the event is early, see which policy applies.
        if (earliest_time > intended_tag.time) {
            LF_PRINT_DEBUG("Event is early.");
            switch(trigger->policy) {
                case drop:
                    LF_PRINT_DEBUG("Policy is drop. Dropping the event.");
                    // Recycle the new event and decrement the
                    // reference count of the token.
                    _lf_done_using(token);
                    lf_recycle_event(env, e);
                    return(0);
                case replace:
                    LF_PRINT_DEBUG("Policy is replace. Replacing the previous event.");
                    // If the event with the previous time is still on the event
                    // queue, then replace the token.  To find this event, we have
                    // to construct a dummy event_t struct.
                    event_t* dummy = lf_get_new_event(env);
                    dummy->next = NULL;
                    dummy->trigger = trigger;
                    dummy->time = trigger->last_tag.time;
                    event_t* found = (event_t *)pqueue_find_equal_same_priority(env->event_q, dummy);

                    if (found != NULL) {
                        // Recycle the existing token and the new event
                        // and update the token of the existing event.
                        lf_replace_token(found, token);
                        lf_recycle_event(env, e);
                        lf_recycle_event(env, dummy);
                        // Leave the last_tag the same.
                        return(0);
                    }
                    lf_recycle_event(env, dummy);

                    // If the preceding event _has_ been handled, then adjust
                    // the tag to defer the event.
                    intended_tag = (tag_t){.time = earliest_time, .microstep = 0};
                    break;
                default:
                    // Default policy is defer
                    intended_tag = (tag_t){.time = earliest_time, .microstep = 0};
                    break;
            }
        }
    }

    // Check if the intended time is in the future
    // This is a sanity check for the logic above
    // FIXME: This is a development assertion and might
    // not be necessary for end-user LF programs
    #ifndef NDEBUG
    if (intended_tag.time < env->current_tag.time) {
        lf_print_error("Attempting to schedule an event earlier than current time by " PRINTF_TIME " nsec! "
                "Revising to the current time " PRINTF_TIME ".",
                env->current_tag.time - intended_tag.time, env->current_tag.time);
        intended_tag.time = env->current_tag.time;
    }
    #endif

    // Set the tag of the event.
    e->time = intended_tag.time;

    // Do not schedule events if if the event time is past the stop time
    // (current microsteps are checked earlier).
    LF_PRINT_DEBUG("Comparing event with elapsed time " PRINTF_TIME " against stop time " PRINTF_TIME ".", e->time - lf_time_start(), env->stop_tag.time - lf_time_start());
    if (e->time > env->stop_tag.time) {
        LF_PRINT_DEBUG("lf_schedule_trigger: event time is past the timeout. Discarding event.");
        _lf_done_using(token);
        lf_recycle_event(env, e);
        return(0);
    }

    // Store the time in order to check the min spacing
    // between this and any following event.
    trigger->last_tag = intended_tag;

    // Queue the event.
    // NOTE: There is no need for an explicit microstep because
    // when this is called, all events at the current tag
    // (time and microstep) have been pulled from the queue,
    // and any new events added at this tag will go into the reaction_q
    // rather than the event_q, so anything put in the event_q with this
    // same time will automatically be executed at the next microstep.
    LF_PRINT_LOG("Inserting event in the event queue with elapsed time " PRINTF_TIME ".",
            e->time - lf_time_start());
    pqueue_insert(env->event_q, e);

    tracepoint_schedule(env, trigger, e->time - env->current_tag.time);

    // FIXME: make a record of handle and implement unschedule.
    // NOTE: Rather than wrapping around to get a negative number,
    // we reset the handle on the assumption that much earlier
    // handles are irrelevant.
    trigger_handle_t return_value = env->_lf_handle++;
    if (env->_lf_handle < 0) {
        env->_lf_handle = 1;
    }
    return return_value;
}
