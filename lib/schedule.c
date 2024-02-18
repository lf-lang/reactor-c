/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Implementation of schedule functions for Lingua Franca programs.
 */

#include "schedule.h"
#include "reactor.h"
#include <string.h> // Defines memcpy.

/**
 * Schedule an action to occur with the specified value and time offset
 * with no payload (no value conveyed).
 * See schedule_token(), which this uses, for details.
 *
 * @param action Pointer to an action on the self struct.
 * @param offset The time offset over and above that in the action.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t lf_schedule(void* action, interval_t offset) {
    return lf_schedule_token((lf_action_base_t*)action, offset, NULL);
}

trigger_handle_t lf_schedule_int(lf_action_base_t* action, interval_t extra_delay, int value) {
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

trigger_handle_t lf_schedule_token(lf_action_base_t* action, interval_t extra_delay, lf_token_t* token) {
    environment_t* env = action->parent->environment;
    
    LF_CRITICAL_SECTION_ENTER(env);
    int return_value = _lf_schedule(env, action->trigger, extra_delay, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    LF_CRITICAL_SECTION_EXIT(env);
    return return_value;
}

trigger_handle_t lf_schedule_copy(
        lf_action_base_t* action, interval_t offset, void* value, size_t length
) {
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
    environment_t* env = action->parent->environment;
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
    trigger_handle_t result = _lf_schedule(env, action->trigger, offset, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    LF_CRITICAL_SECTION_EXIT(env);
    return result;
}

trigger_handle_t lf_schedule_value(lf_action_base_t* action, interval_t extra_delay, void* value, int length) {
    if (length < 0) {
        lf_print_error(
            "schedule_value():"
            " Ignoring request to schedule an action with a value that has a negative length (%d).",
            length
        );
        return -1;
    }
    token_template_t* template = (token_template_t*)action;
    environment_t* env = action->parent->environment;
    LF_CRITICAL_SECTION_ENTER(env);
    lf_token_t* token = _lf_initialize_token_with_value(template, value, length);
    int return_value = _lf_schedule(env, action->trigger, extra_delay, token);
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
