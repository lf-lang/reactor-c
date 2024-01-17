/* Runtime infrastructure for the C target of Lingua Franca. */

/*************
Copyright (c) 2019, The University of California at Berkeley.

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
 * Runtime infrastructure for the C target of Lingua Franca.
 * This file contains resources that are shared by the threaded and
 * non-threaded versions of the C runtime.
 *
 *  @author{Edward A. Lee <eal@berkeley.edu>}
 *  @author{Marten Lohstroh <marten@berkeley.edu>}
 *  @author{Mehrdad Niknami <mniknami@berkeley.edu>}
 *  @author{Soroush Bateni <soroush@utdallas.edu}
 *  @author{Alexander Schulz-Rosengarten <als@informatik.uni-kiel.de>}
 *  @author{Erling Rennemo Jellum <erling.r.jellum0@ntnu.no>}
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "lf_types.h"
#ifdef MODAL_REACTORS
#include "modes.h"
#endif
#ifdef FEDERATED
#include "federate.h"
#endif
#include "port.h"
#include "pqueue.h"
#include "reactor.h"
#include "reactor_common.h"
#include "tag.h"
#include "trace.h"
#include "util.h"
#include "vector.h"
#include "hashset/hashset.h"
#include "hashset/hashset_itr.h"
#include "environment.h"

#if !defined(LF_SINGLE_THREADED)
#include "watchdog.h"

// Code generated global variables.
extern int _lf_watchdog_count;
extern watchdog_t* _lf_watchdogs;
#endif

// Global variable defined in tag.c:
extern instant_t start_time;

// Global variable defined in lf_token.c:
extern int _lf_count_payload_allocations;

/**
 * Indicator of whether to wait for physical time to match logical time.
 * By default, execution will wait. The command-line argument -fast will
 * eliminate the wait and allow logical time to exceed physical time.
 */
bool fast = false;

/**
 * The number of worker threads for threaded execution.
 * By default, execution is not threaded and this variable will have value 0.
 *
 * If the execution is threaded, a value of 0 indicates that the runtime should
 * decide on the number of workers (which will be decided based on the number of
 * available cores on the host machine).
 */
unsigned int _lf_number_of_workers = 0u;

/**
 * The logical time to elapse during execution, or -1 if no timeout time has
 * been given. When the logical equal to start_time + duration has been
 * reached, execution will terminate.
 */
instant_t duration = -1LL;

/** Indicator of whether the keepalive command-line option was given. */
bool keepalive_specified = false;

/**
 * Global STP offset uniformly applied to advancement of each
 * time step in federated execution. This can be retrieved in
 * user code by calling lf_get_stp_offset() and adjusted by
 * calling lf_set_stp_offset(interval_t offset).
 */
interval_t _lf_fed_STA_offset = 0LL;

void _lf_print_event(void* event) {
    if (event == NULL) {
        printf("NULL");
    } else {
        event_t* ev = (event_t*)event;
        lf_print("Event: Time=" PRINTF_TIME ", dummy=%d, timer=%d",
                ev->time - start_time, ev->is_dummy, ev->trigger->is_timer);
    }
}

/**
 * Allocate memory using calloc (so the allocated memory is zeroed out)
 * and record the allocated memory on the specified self struct so that
 * it will be freed when calling {@link free_reactor(self_base_t)}.
 * @param count The number of items of size 'size' to accomodate.
 * @param size The size of each item.
 * @param head Pointer to the head of a list on which to record
 *  the allocation, or NULL to not record it.
 * @return A pointer to the allocated memory.
 */
void* _lf_allocate(
        size_t count, size_t size, struct allocation_record_t** head) {
    void *mem = calloc(count, size);
    if (mem == NULL) lf_print_error_and_exit("Out of memory!");
    if (head != NULL) {
        struct allocation_record_t* record
                = (allocation_record_t*)calloc(1, sizeof(allocation_record_t));
        if (record == NULL) lf_print_error_and_exit("Out of memory!");
        record->allocated = mem;
        allocation_record_t* tmp = *head; // Previous head of the list or NULL.
        *head = record;                   // New head of the list.
        record->next = tmp;
    }
    return mem;
}

/**
 * Head of a list of pointers to dynamically generated reactor
 * self structs to be freed in terminate().
 */
struct allocation_record_t* _lf_reactors_to_free = NULL;

/**
 * Allocate memory for a new runtime instance of a reactor.
 * This records the reactor on the list of reactors to be freed at
 * termination of the program. If you plan to free the reactor before
 * termination of the program, use calloc instead (which this uses).
 * @param size The size of the self struct, obtained with sizeof().
 */
void* _lf_new_reactor(size_t size) {
    return _lf_allocate(1, size, &_lf_reactors_to_free);
}

/**
 * Free memory on the specified allocation record, e.g. allocated by
 * {@link _lf_allocate(size_t, size_t, allocation_record_t**)}.
 * Mark the list empty by setting `*head` to NULL.
 * @param head Pointer to the head of a list on which to record
 *  the allocation, or NULL to not record it.
 */
void _lf_free(struct allocation_record_t** head) {
    if (head == NULL) return;
    struct allocation_record_t* record = *head;
    while (record != NULL) {
        LF_PRINT_DEBUG("Freeing memory at %p", record->allocated);
        free(record->allocated);
        struct allocation_record_t* tmp = record->next;
    	LF_PRINT_DEBUG("Freeing allocation record at %p", record);
        free(record);
        record = tmp;
    }
    *head = NULL;
}

/**
 * Free memory recorded on the allocations list of the specified reactor
 * and then free the specified self struct.
 * @param self The self struct of the reactor.
 */
void _lf_free_reactor(self_base_t *self) {
    _lf_free(&self->allocations);
    free(self);
}

/**
 * Free all the reactors that are allocated with
 * {@link #_lf_new_reactor(size_t)}.
 */
void _lf_free_all_reactors(void) {
    struct allocation_record_t* head = _lf_reactors_to_free;
    while (head != NULL) {
        _lf_free_reactor((self_base_t*)head->allocated);
        struct allocation_record_t* tmp = head->next;
        free(head);
        head = tmp;
    }
    _lf_reactors_to_free = NULL;
}

/**
 * Set the stop tag.
 *
 * This function will always choose the minimum
 * of the provided tag and stop_tag
 *
 * @note In threaded programs, the mutex must be locked before
 *  calling this function.
 */
void _lf_set_stop_tag(environment_t* env, tag_t tag) {
    assert(env != GLOBAL_ENVIRONMENT);
    if (lf_tag_compare(tag, env->stop_tag) < 0) {
        env->stop_tag = tag;
    }
}

/////////////////////////////
// The following functions are in scope for all reactors:

/**
 * Return the global STP offset on advancement of logical
 * time for federated execution.
 */
interval_t lf_get_stp_offset() {
    return _lf_fed_STA_offset;
}

/**
 * Set the global STP offset on advancement of logical
 * time for federated execution.
 *
 * @param offset A positive time value to be applied
 *  as the STP offset.
 */
void lf_set_stp_offset(interval_t offset) {
    if (offset > 0LL) {
        _lf_fed_STA_offset = offset;
    }
}


/**
 * Trigger 'reaction'.
 *
 * @param env Environment in which we are executing.
 * @param reaction The reaction.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  single-threaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 */
void _lf_trigger_reaction(environment_t* env, reaction_t* reaction, int worker_number);

/**
 * Use tables to reset is_present fields to false,
 * set intended_tag fields in federated execution
 * to the current_tag, and decrement reference
 * counts between time steps and at the end of execution.
 */
void _lf_start_time_step(environment_t *env) {
    assert(env != GLOBAL_ENVIRONMENT);
    if (!env->execution_started) {
        // Execution hasn't started, so this is probably being invoked in termination
        // due to an error.
        return;
    }
    LF_PRINT_LOG("--------- Start time step at tag " PRINTF_TAG ".", env->current_tag.time - start_time, env->current_tag.microstep);
    // Handle dynamically created tokens for mutable inputs.
    _lf_free_token_copies(env);

    bool** is_present_fields = env->is_present_fields_abbreviated;
    int size = env->is_present_fields_abbreviated_size;
    if (env->is_present_fields_abbreviated_size > env->is_present_fields_size) {
        size = env->is_present_fields_size;
        is_present_fields = env->is_present_fields;
    }
    for(int i = 0; i < size; i++) {
        *is_present_fields[i] = false;
    }
    // Reset sparse IO record sizes to 0, if any.
    if (env->sparse_io_record_sizes.start != NULL) {
        for (size_t i = 0; i < vector_size(&env->sparse_io_record_sizes); i++) {
            // NOTE: vector_at does not return the element at
            // the index, but rather returns a pointer to that element, which is
            // itself a pointer.
            int** sizep = (int**)vector_at(&env->sparse_io_record_sizes, i);
            if (sizep != NULL && *sizep != NULL) {
                **sizep = 0;
            }
        }
    }
    env->is_present_fields_abbreviated_size = 0;

#ifdef FEDERATED
    // If the environment is the top-level one, we have some work to do.
    environment_t *envs;
    int num_envs = _lf_get_environments(&envs);
    if (env == envs) {
        // This is the top-level environment.

#ifdef FEDERATED_DECENTRALIZED
        for (int i = 0; i < env->is_present_fields_size; i++) {
            // An intended tag of NEVER_TAG indicates that it has never been set.
            *env->_lf_intended_tag_fields[i] = NEVER_TAG;
        }
#endif // FEDERATED_DECENTRALIZED

        // Reset absent fields on network ports because
        // their status is unknown
        lf_reset_status_fields_on_input_port_triggers();
        // Signal the helper thread to reset its progress since the logical time has changed.
        lf_cond_signal(&lf_current_tag_changed);
    }
#endif // FEDERATED
}

/**
 * A helper function that returns true if the provided tag is after stop tag.
 *
 * @param env Environment in which we are executing.
 * @param tag The tag to check against stop tag
 */
bool _lf_is_tag_after_stop_tag(environment_t* env, tag_t tag) {
    assert(env != GLOBAL_ENVIRONMENT);
    return (lf_tag_compare(tag, env->stop_tag) > 0);
}

/**
 * Pop all events from event_q with timestamp equal to current_tag.time, extract all
 * the reactions triggered by these events, and stick them into the reaction
 * queue.
 * @param env Environment in which we are executing.
 */
void _lf_pop_events(environment_t *env) {
    assert(env != GLOBAL_ENVIRONMENT);
#ifdef MODAL_REACTORS
    _lf_handle_mode_triggered_reactions(env);
#endif

    event_t* event = (event_t*)pqueue_peek(env->event_q);
    while(event != NULL && event->time == env->current_tag.time) {
        event = (event_t*)pqueue_pop(env->event_q);

        if (event->is_dummy) {
            LF_PRINT_DEBUG("Popped dummy event from the event queue.");
            if (event->next != NULL) {
                LF_PRINT_DEBUG("Putting event from the event queue for the next microstep.");
                pqueue_insert(env->next_q, event->next);
            }
            _lf_recycle_event(env, event);
            // Peek at the next event in the event queue.
            event = (event_t*)pqueue_peek(env->event_q);
            continue;
        }

#ifdef MODAL_REACTORS
        // If this event is associated with an inactive mode it should haven been suspended and no longer on the event queue.
        // NOTE: This should not be possible
        if (!_lf_mode_is_active(event->trigger->mode)) {
            lf_print_warning("Assumption violated. There is an event on the event queue that is associated to an inactive mode.");
        }
#endif

        lf_token_t *token = event->token;

        // Put the corresponding reactions onto the reaction queue.
        for (int i = 0; i < event->trigger->number_of_reactions; i++) {
            reaction_t *reaction = event->trigger->reactions[i];
            // Do not enqueue this reaction twice.
            if (reaction->status == inactive) {
#ifdef FEDERATED_DECENTRALIZED
                // In federated execution, an intended tag that is not (NEVER, 0)
                // indicates that this particular event is triggered by a network message.
                // The intended tag is set in handle_tagged_message in federate.c whenever
                // a tagged message arrives from another federate.
                if (event->intended_tag.time != NEVER) {
                    // If the intended tag of the event is actually set,
                    // transfer the intended tag to the trigger so that
                    // the reaction can access the value.
                    event->trigger->intended_tag = event->intended_tag;
                    // And check if it is in the past compared to the current tag.
                    if (lf_tag_compare(event->intended_tag, env->current_tag) < 0) {
                        // Mark the triggered reaction with a STP violation
                        reaction->is_STP_violated = true;
                        LF_PRINT_LOG("Trigger %p has violated the reaction's STP offset. Intended tag: " PRINTF_TAG ". Current tag: " PRINTF_TAG,
                                    event->trigger,
                                    event->intended_tag.time - start_time, event->intended_tag.microstep,
                                    env->current_tag.time - start_time, env->current_tag.microstep);
                        // Need to update the last_known_status_tag of the port because otherwise,
                        // the MLAA could get stuck, causing the program to lock up.
                        // This should not call update_last_known_status_on_input_port because we
                        // are starting a new tag step execution, so there are no reactions blocked on this input.
                        if (lf_tag_compare(env->current_tag, event->trigger->last_known_status_tag) > 0) {
                            event->trigger->last_known_status_tag = env->current_tag;
                        }
                    }
                }
#endif

#ifdef MODAL_REACTORS
                // Check if reaction is disabled by mode inactivity
                if (!_lf_mode_is_active(reaction->mode)) {
                    LF_PRINT_DEBUG("Suppressing reaction %s due inactive mode.", reaction->name);
                    continue; // Suppress reaction by preventing entering reaction queue
                }
#endif
                LF_PRINT_DEBUG("Triggering reaction %s.", reaction->name);
                _lf_trigger_reaction(env, reaction, -1);
            } else {
                LF_PRINT_DEBUG("Reaction is already triggered: %s", reaction->name);
            }
        }

        // Mark the trigger present.
        event->trigger->status = present;

        // If the trigger is a periodic timer, create a new event for its next execution.
        if (event->trigger->is_timer && event->trigger->period > 0LL) {
            // Reschedule the trigger.
            _lf_schedule(env, event->trigger, event->trigger->period, NULL);
        }

        // Copy the token pointer into the trigger struct so that the
        // reactions can access it. This overwrites the previous template token,
        // for which we decrement the reference count.
        _lf_replace_template_token((token_template_t*)event->trigger, token);

        // Decrement the reference count because the event queue no longer needs this token.
        // This has to be done after the above call to _lf_replace_template_token because
        // that call will increment the reference count and we need to not let the token be
        // freed prematurely.
        _lf_done_using(token);

        // Mark the trigger present.
        event->trigger->status = present;

        // If this event points to a next event, insert it into the next queue.
        if (event->next != NULL) {
            // Insert the next event into the next queue.
            pqueue_insert(env->next_q, event->next);
        }

        _lf_recycle_event(env, event);

        // Peek at the next event in the event queue.
        event = (event_t*)pqueue_peek(env->event_q);
    };

    LF_PRINT_DEBUG("There are %zu events deferred to the next microstep.", pqueue_size(env->next_q));

    // After populating the reaction queue, see if there are things on the
    // next queue to put back into the event queue.
    while(pqueue_peek(env->next_q) != NULL) {
        pqueue_insert(env->event_q, pqueue_pop(env->next_q));
    }
}

/**
 * Get a new event. If there is a recycled event available, use that.
 * If not, allocate a new one. In either case, all fields will be zero'ed out.
 * @param env Environment in which we are executing.
 */
static event_t* _lf_get_new_event(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);
    // Recycle event_t structs, if possible.
    event_t* e = (event_t*)pqueue_pop(env->recycle_q);
    if (e == NULL) {
        e = (event_t*)calloc(1, sizeof(struct event_t));
        if (e == NULL) lf_print_error_and_exit("Out of memory!");
#ifdef FEDERATED_DECENTRALIZED
        e->intended_tag = (tag_t) { .time = NEVER, .microstep = 0u};
#endif
    }
    return e;
}

/**
 * Initialize the given timer.
 * If this timer has a zero offset, enqueue the reactions it triggers.
 * If this timer is to trigger reactions at a _future_ tag as well,
 * schedule it accordingly.
 * @param env Environment in which we are executing.
 * @param timer The timer to initialize.
 */
void _lf_initialize_timer(environment_t* env, trigger_t* timer) {
    assert(env != GLOBAL_ENVIRONMENT);
    interval_t delay = 0;

#ifdef MODAL_REACTORS
    // Suspend all timer events that start in inactive mode
    if (!_lf_mode_is_active(timer->mode)) {
        // FIXME: The following check might not be working as
        // intended
        // && (timer->offset != 0 || timer->period != 0)) {
        event_t* e = _lf_get_new_event(env);
        e->trigger = timer;
        e->time = lf_time_logical(env) + timer->offset;
        _lf_add_suspended_event(e);
        return;
    }
#endif
    if (timer->offset == 0) {
        for (int i = 0; i < timer->number_of_reactions; i++) {
            _lf_trigger_reaction(env, timer->reactions[i], -1);
            tracepoint_schedule(env->trace, timer, 0LL); // Trace even though schedule is not called.
        }
        if (timer->period == 0) {
            return;
        } else {
            // Schedule at t + period.
            delay = timer->period;
        }
    } else {
        // Schedule at t + offset.
        delay = timer->offset;
    }

    // Get an event_t struct to put on the event queue.
    // Recycle event_t structs, if possible.
    event_t* e = _lf_get_new_event(env);
    e->trigger = timer;
    e->time = lf_time_logical(env) + delay;
    // NOTE: No lock is being held. Assuming this only happens at startup.
    pqueue_insert(env->event_q, e);
    tracepoint_schedule(env->trace, timer, delay); // Trace even though schedule is not called.
}

/**
 * @brief Initialize all the timers in the environment
 * @param env Environment in which we are executing.
 */
void _lf_initialize_timers(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);
    for (int i = 0; i < env->timer_triggers_size; i++) {
        if (env->timer_triggers[i] != NULL) {
            _lf_initialize_timer(env, env->timer_triggers[i]);
        }
    }
}

/**
 * @brief Trigger all the startup reactions in our environment
 * @param env Environment in which we are executing.
 */
void _lf_trigger_startup_reactions(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);
    for (int i = 0; i < env->startup_reactions_size; i++) {
        if (env->startup_reactions[i] != NULL) {
            if (env->startup_reactions[i]->mode != NULL) {
                // Skip reactions in modes
                continue;
            }
            _lf_trigger_reaction(env, env->startup_reactions[i], -1);
        }
    }
    #ifdef MODAL_REACTORS
    if (env->modes) {
        _lf_handle_mode_startup_reset_reactions(
            env,
            env->startup_reactions, env->startup_reactions_size,
            NULL, 0,
            env->modes->modal_reactor_states, env->modes->modal_reactor_states_size
        );
    }
    #endif
}

/**
 * @brief Trigger all the shutdown reactions in our environment
 * @param env Environment in which we are executing.
 */
void _lf_trigger_shutdown_reactions(environment_t *env) {
    assert(env != GLOBAL_ENVIRONMENT);
    for (int i = 0; i < env->shutdown_reactions_size; i++) {
        if (env->shutdown_reactions[i] != NULL) {
            if (env->shutdown_reactions[i]->mode != NULL) {
                // Skip reactions in modes
                continue;
            }
            _lf_trigger_reaction(env, env->shutdown_reactions[i], -1);
        }
    }
#ifdef MODAL_REACTORS
    if (env->modes) {
        _lf_handle_mode_shutdown_reactions(env, env->shutdown_reactions, env->shutdown_reactions_size);
    }
#endif
}

/**
 * Recycle the given event.
 * Zero it out and pushed it onto the recycle queue.
 * @param env Environment in which we are executing.
 * @param e The event to recycle.
 */
void _lf_recycle_event(environment_t* env, event_t* e) {
    assert(env != GLOBAL_ENVIRONMENT);
    e->time = 0LL;
    e->trigger = NULL;
    e->pos = 0;
    e->token = NULL;
    e->is_dummy = false;
#ifdef FEDERATED_DECENTRALIZED
    e->intended_tag = (tag_t) { .time = NEVER, .microstep = 0u};
#endif
    e->next = NULL;
    pqueue_insert(env->recycle_q, e);
}

/**
 * Create dummy events to be used as spacers in the event queue.
 * @param env Environment in which we are executing.
 * @param trigger The eventual event to be triggered.
 * @param time The logical time of that event.
 * @param next The event to place after the dummy events.
 * @param offset The number of dummy events to insert.
 * @return A pointer to the first dummy event.
 */
event_t* _lf_create_dummy_events(environment_t* env, trigger_t* trigger, instant_t time, event_t* next, microstep_t offset) {
    event_t* first_dummy = _lf_get_new_event(env);
    event_t* dummy = first_dummy;
    dummy->time = time;
    dummy->is_dummy = true;
    dummy->trigger = trigger;
    while (offset > 0) {
        if (offset == 1) {
            dummy->next = next;
            break;
        }
        dummy->next = _lf_get_new_event(env);
        dummy = dummy->next;
        dummy->time = time;
        dummy->is_dummy = true;
        dummy->trigger = trigger;
        offset--;
    }
    return first_dummy;
}

/**
 * Replace the token on the specified event with the specified
 * token and free the old token.
 * @param event The event.
 * @param token The token.
 */
static void _lf_replace_token(event_t* event, lf_token_t* token) {
    if (event->token != token) {
        // Free the existing token, if any.
        _lf_done_using(event->token);
    }
    // Replace the token with ours.
    event->token = token;
}

/**
 * Schedule events at a specific tag (time, microstep), provided
 * that the tag is in the future relative to the current tag (or the
 * environment has not started executing). The input time values are absolute.
 *
 * If there is an event found at the requested tag, the payload
 * is replaced and 0 is returned.
 *
 * Note that this function is an internal API that must
 * be called with tags that are in order for a given
 * trigger. This means that the following order is illegal:
 * _lf_schedule_at_tag(trigger1, bigger_tag, ...);
 * _lf_schedule_at_tag(trigger1, smaller_tag, ...);
 * where bigger_tag > smaller_tag. This function is primarily
 * used for network communication (which is assumed to be
 * in order).
 *
 * This function assumes the caller holds the mutex lock.
 *
 * @param env Environment in which we are executing.
 * @param trigger The trigger to be invoked at a later logical time.
 * @param tag Logical tag of the event
 * @param token The token wrapping the payload or NULL for no payload.
 *
 * @return A positive trigger handle for success, 0 if no new event was scheduled
 *  (instead, the payload was updated), or -1 for error (the tag is equal to or less
 *  than the current tag).
 */
trigger_handle_t _lf_schedule_at_tag(environment_t* env, trigger_t* trigger, tag_t tag, lf_token_t* token) {
    assert(env != GLOBAL_ENVIRONMENT);
    tag_t current_logical_tag = env->current_tag;

    LF_PRINT_DEBUG("_lf_schedule_at_tag() called with tag " PRINTF_TAG " at tag " PRINTF_TAG ".",
                  tag.time - start_time, tag.microstep,
                  current_logical_tag.time - start_time, current_logical_tag.microstep);
    if (lf_tag_compare(tag, current_logical_tag) <= 0 && env->execution_started) {
        lf_print_warning("_lf_schedule_at_tag(): requested to schedule an event at the current or past tag.");
        return -1;
    }

    // Increment the reference count of the token.
    if (token != NULL) {
        token->ref_count++;
        LF_PRINT_DEBUG("_lf_schedule_at_tag: Incremented ref_count of %p to %zu.",
                token, token->ref_count);
    }

    // Do not schedule events if the tag is after the stop tag
    if (_lf_is_tag_after_stop_tag(env, tag)) {
         lf_print_warning("_lf_schedule_at_tag: event time is past the timeout. Discarding event.");
        _lf_done_using(token);
        return -1;
    }

    event_t* e = _lf_get_new_event(env);
    // Set the event time
    e->time = tag.time;

    tracepoint_schedule(env->trace, trigger, tag.time - current_logical_tag.time);

    // Make sure the event points to this trigger so when it is
    // dequeued, it will trigger this trigger.
    e->trigger = trigger;

    // Set the payload.
    e->token = token;

#ifdef FEDERATED_DECENTRALIZED
    // Set the intended tag
    e->intended_tag = trigger->intended_tag;
#endif

    event_t* found = (event_t *)pqueue_find_equal_same_priority(env->event_q, e);
    if (found != NULL) {
        if (tag.microstep == 0u) {
                // The microstep is 0, which means that the event is being scheduled
                // at a future time and at the beginning of the skip list of events
                // at that time.
                // In case the event is a dummy event
                // convert it to a real event.
                found->is_dummy = false;
                switch (trigger->policy) {
                    case drop:
                        if (found->token != token) {
                            _lf_done_using(token);
                        }
                        _lf_recycle_event(env, e);
                        return(0);
                        break;
                    case replace:
                        // Replace the payload of the event at the head with our
                        // current payload.
                        _lf_replace_token(found, token);
                        _lf_recycle_event(env, e);
                        return 0;
                        break;
                    default:
                        // Adding a microstep to the original
                        // intended tag.
                        if (_lf_is_tag_after_stop_tag(env, (tag_t) {.time=found->time,.microstep=1})) {
                            // Scheduling e will incur a microstep after the stop tag,
                            // which is illegal.
                            _lf_recycle_event(env, e);
                            return 0;
                        }
                        if (found->next != NULL) {
                            lf_print_error("_lf_schedule_at_tag: in-order contract violated.");
                            return -1;
                        }
                        found->next = e;
                }
        } else {
            // We are requesting a microstep greater than 0
            // where there is already an event for this trigger on the event queue.
            // That event may itself be a dummy event for a real event that is
            // also at a microstep greater than 0.
            // We have to insert our event into the chain or append it
            // to the end of the chain, depending on which microstep is lesser.
            microstep_t microstep_of_found = 0;
            if (tag.time == current_logical_tag.time) {
                // This is a situation where the head of the queue
                // is an event with microstep == current_microstep + 1
                // which should be reflected in our steps calculation.
                microstep_of_found += current_logical_tag.microstep + 1; // Indicating that
                                                            // the found event
                                                            // is at this microstep.
            }
            // Follow the chain of events until the right point
            // to insert the new event.
            while (microstep_of_found < tag.microstep - 1) {
                if (found->next == NULL) {
                    // The chain stops short of where we want to be.
                    // If it exactly one microstep short of where we want to be,
                    // then we don't need a dummy. Otherwise, we do.
                    microstep_t undershot_by = (tag.microstep - 1) - microstep_of_found;
                    if (undershot_by > 0) {
                        found->next = _lf_create_dummy_events(env, trigger, tag.time, e, undershot_by);
                    } else {
                        found->next = e;
                    }
                    return 1;
                }
                found = found->next;
                microstep_of_found++;
            }
            // At this point, microstep_of_found == tag.microstep - 1.
            if (found->next == NULL) {
                found->next = e;
            } else {
                switch (trigger->policy) {
                    case drop:
                        if (found->next->token != token) {
                            _lf_done_using(token);
                        }
                        _lf_recycle_event(env, e);
                        return 0;
                        break;
                    case replace:
                        // Replace the payload of the event at the head with our
                        // current payload.
                        _lf_replace_token(found->next, token);
                        _lf_recycle_event(env, e);
                        return 0;
                        break;
                    default:
                        // Adding a microstep to the original
                        // intended tag.
                        if (_lf_is_tag_after_stop_tag(env, (tag_t){.time=found->time,.microstep=microstep_of_found+1})) {
                            // Scheduling e will incur a microstep at timeout,
                            // which is illegal.
                            _lf_recycle_event(env, e);
                            return 0;
                        }
                        if (found->next->next != NULL) {
                            lf_print_error("_lf_schedule_at_tag: in-order contract violated.");
                            return -1;
                        }
                        found->next->next = e;
                }
            }
        }
    } else {
        // No existing event queued.
        microstep_t relative_microstep = tag.microstep;
        if (tag.time == current_logical_tag.time) {
            relative_microstep -= current_logical_tag.microstep;
        }
        if ((tag.time == current_logical_tag.time && relative_microstep == 1 && env->execution_started) ||
                tag.microstep == 0) {
            // Do not need a dummy event if we are scheduling at 1 microstep
            // in the future at current time or at microstep 0 in a future time.
            // Note that if execution hasn't started, then we have to insert dummy events.
            pqueue_insert(env->event_q, e);
        } else {
            // Create a dummy event. Insert it into the queue, and let its next
            // pointer point to the actual event.
            pqueue_insert(env->event_q, _lf_create_dummy_events(env, trigger, tag.time, e, relative_microstep));
        }
    }
    trigger_handle_t return_value = env->_lf_handle++;
    if (env->_lf_handle < 0) {
        env->_lf_handle = 1;
    }
    return return_value;
}

/**
 * Schedule the specified trigger at env->current_tag.time plus the offset of the
 * specified trigger plus the delay. See schedule_token() in reactor.h for details.
 * This is the internal implementation shared by both the threaded
 * and non-threaded versions.
 *
 * The value is required to be either
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
 *
 * @param env Environment in which we are executing.
 * @param trigger The trigger to be invoked at a later logical time.
 * @param extra_delay The logical time delay, which gets added to the
 *  trigger's minimum delay, if it has one. If this number is negative,
 *  then zero is used instead.
 * @param token The token wrapping the payload or NULL for no payload.
 * @return A handle to the event, or 0 if no new event was scheduled, or -1 for error.
 */
trigger_handle_t _lf_schedule(environment_t *env, trigger_t* trigger, interval_t extra_delay, lf_token_t* token) {
    assert(env != GLOBAL_ENVIRONMENT);
    if (_lf_is_tag_after_stop_tag(env, env->current_tag)) {
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

    LF_PRINT_DEBUG("_lf_schedule: scheduling trigger %p with delay " PRINTF_TIME " and token %p.",
            trigger, extra_delay, token);

    // Increment the reference count of the token.
    if (token != NULL) {
        token->ref_count++;
        LF_PRINT_DEBUG("_lf_schedule: Incremented ref_count of %p to %zu.",
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
    
    LF_PRINT_DEBUG("_lf_schedule: env->current_tag.time = " PRINTF_TIME ". Total logical delay = " PRINTF_TIME "",
            env->current_tag.time, delay);
    interval_t min_spacing = trigger->period;

    event_t* e = _lf_get_new_event(env);

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
            if (_lf_is_tag_after_stop_tag(env, intended_tag)) {
                LF_PRINT_DEBUG("Attempt to schedule an event after stop_tag was rejected.");
                // Scheduling an event will incur a microstep
                // after the stop tag.
                _lf_recycle_event(env, e);
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
                    _lf_recycle_event(env, e);
                    return(0);
                case replace:
                    LF_PRINT_DEBUG("Policy is replace. Replacing the previous event.");
                    // If the event with the previous time is still on the event
                    // queue, then replace the token.  To find this event, we have
                    // to construct a dummy event_t struct.
                    event_t* dummy = _lf_get_new_event(env);
                    dummy->next = NULL;
                    dummy->trigger = trigger;
                    dummy->time = trigger->last_tag.time;
                    event_t* found = (event_t *)pqueue_find_equal_same_priority(env->event_q, dummy);

                    if (found != NULL) {
                        // Recycle the existing token and the new event
                        // and update the token of the existing event.
                        _lf_replace_token(found, token);
                        _lf_recycle_event(env, e);
                        _lf_recycle_event(env, dummy);
                        // Leave the last_tag the same.
                        return(0);
                    }
                    _lf_recycle_event(env, dummy);

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
    LF_PRINT_DEBUG("Comparing event with elapsed time " PRINTF_TIME " against stop time " PRINTF_TIME ".", e->time - start_time, env->stop_tag.time - start_time);
    if (e->time > env->stop_tag.time) {
        LF_PRINT_DEBUG("_lf_schedule: event time is past the timeout. Discarding event.");
        _lf_done_using(token);
        _lf_recycle_event(env, e);
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
            e->time - start_time);
    pqueue_insert(env->event_q, e);

    tracepoint_schedule(env->trace, trigger, e->time - env->current_tag.time);

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

/**
 * Insert reactions triggered by trigger to the reaction queue...
 *
 * @param env Environment in which we are executing.
 * @param trigger The trigger
 * @param token The token wrapping the payload or NULL for no payload.
 * @return 1 if successful, or 0 if no new reaction was scheduled because the function
 *  was called incorrectly.
 */
trigger_handle_t _lf_insert_reactions_for_trigger(environment_t* env, trigger_t* trigger, lf_token_t* token) {
    assert(env != GLOBAL_ENVIRONMENT);
    // The trigger argument could be null, meaning that nothing is triggered.
    // Doing this after incrementing the reference count ensures that the
    // payload will be freed, if there is one.
    if (trigger == NULL) {
        lf_print_warning("_lf_schedule_init_reactions() called with a NULL trigger");
        _lf_done_using(token);
        return 0;
    }

    // Check to see if the trigger is not a timer
    // and not a physical action
    if (trigger->is_timer || trigger->is_physical) {
        lf_print_warning("_lf_schedule_init_reactions() called on a timer or physical action.");
        return 0;
    }

#ifdef MODAL_REACTORS
    // If this trigger is associated with an inactive mode, it should not trigger any reaction.
    if (!_lf_mode_is_active(trigger->mode)) {
        LF_PRINT_DEBUG("Suppressing reactions of trigger due inactivity of mode %s.", trigger->mode->name);
        return 1;
    }
#endif

    // Check if the trigger has violated the STP offset
    bool is_STP_violated = false;
#ifdef FEDERATED
    if (lf_tag_compare(trigger->intended_tag, env->current_tag) < 0) {
        is_STP_violated = true;
    }
#ifdef FEDERATED_CENTRALIZED
    // Check for STP violation in the centralized coordination, which is a
    // critical error.
    if (is_STP_violated) {
        lf_print_error_and_exit("Attempted to insert reactions for a trigger that had an intended tag that was in the past. "
                             "This should not happen under centralized coordination. Intended tag: " PRINTF_TAG ". Current tag: " PRINTF_TAG ").",
                             trigger->intended_tag.time - lf_time_start(),
                             trigger->intended_tag.microstep,
                             lf_time_logical_elapsed(env), 
                             env->current_tag.microstep);
    }
#endif
#endif

    // Copy the token pointer into the trigger struct so that the
    // reactions can access it. This overwrites the previous template token,
    // for which we decrement the reference count.
    _lf_replace_template_token((token_template_t*)trigger, token);

    // Mark the trigger present.
    trigger->status = present;

    // Push the corresponding reactions for this trigger
    // onto the reaction queue.
    for (int i = 0; i < trigger->number_of_reactions; i++) {
        reaction_t* reaction = trigger->reactions[i];
#ifdef MODAL_REACTORS
        // Check if reaction is disabled by mode inactivity
        if (!_lf_mode_is_active(reaction->mode)) {
            LF_PRINT_DEBUG("Suppressing reaction %s due inactivity of mode %s.", reaction->name, reaction->mode->name);
            continue; // Suppress reaction by preventing entering reaction queue
        }
#endif
        // Do not enqueue this reaction twice.
        if (reaction->status == inactive) {
            reaction->is_STP_violated = is_STP_violated;
            _lf_trigger_reaction(env, reaction, -1);
            LF_PRINT_LOG("Enqueued reaction %s at time " PRINTF_TIME ".", reaction->name, lf_time_logical(env));
        }
    }

    return 1;
}

/**
 * Schedule the specified trigger at env->current_tag.time plus the offset of the
 * specified trigger plus the delay.
 * See reactor.h for documentation.
 */
trigger_handle_t _lf_schedule_token(lf_action_base_t* action, interval_t extra_delay, lf_token_t* token) {
    environment_t* env = action->parent->environment;
    
    if (lf_critical_section_enter(env) != 0) {
        lf_print_error_and_exit("Could not enter critical section");
    }
    int return_value = _lf_schedule(env, action->trigger, extra_delay, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    if(lf_critical_section_exit(env) != 0) {
        lf_print_error_and_exit("Could not leave critical section");
    }
    return return_value;
}

/**
 * Schedule an action to occur with the specified value and time offset
 * with a copy of the specified value.
 * See reactor.h for documentation.
 */
trigger_handle_t _lf_schedule_copy(lf_action_base_t* action, interval_t offset, void* value, size_t length) {
    if (value == NULL) {
        return _lf_schedule_token(action, offset, NULL);
    }
    environment_t* env = action->parent->environment;
    token_template_t* template = (token_template_t*)action;
    if (action == NULL || template->type.element_size <= 0) {
        lf_print_error("schedule: Invalid element size.");
        return -1;
    }
    if (lf_critical_section_enter(env) != 0) {
        lf_print_error_and_exit("Could not enter critical section");
    }
    // Initialize token with an array size of length and a reference count of 0.
    lf_token_t* token = _lf_initialize_token(template, length);
    // Copy the value into the newly allocated memory.
    memcpy(token->value, value, template->type.element_size * length);
    // The schedule function will increment the reference count.
    trigger_handle_t result = _lf_schedule(env, action->trigger, offset, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    if(lf_critical_section_exit(env) != 0) {
        lf_print_error_and_exit("Could not leave critical section");
    }
    return result;
}


/**
 * Variant of schedule_token that creates a token to carry the specified value.
 * See reactor.h for documentation.
 */
trigger_handle_t _lf_schedule_value(lf_action_base_t* action, interval_t extra_delay, void* value, size_t length) {
    token_template_t* template = (token_template_t*)action;
    environment_t* env = action->parent->environment;
    if (lf_critical_section_enter(env) != 0) {
        lf_print_error_and_exit("Could not enter critical section");
    }
    lf_token_t* token = _lf_initialize_token_with_value(template, value, length);
    int return_value = _lf_schedule(env, action->trigger, extra_delay, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_notify_of_event(env);
    if(lf_critical_section_exit(env) != 0) {
        lf_print_error_and_exit("Could not leave critical section");
    }
    return return_value;
}

void _lf_advance_logical_time(environment_t *env, instant_t next_time) {
    assert(env != GLOBAL_ENVIRONMENT);

    // FIXME: The following checks that _lf_advance_logical_time()
    // is being called correctly. Namely, check if logical time
    // is being pushed past the head of the event queue. This should
    // never happen if _lf_advance_logical_time() is called correctly.
    // This is commented out because it will add considerable overhead
    // to the ordinary execution of LF programs. Instead, there might
    // be a need for a target property that enables these kinds of logic
    // assertions for development purposes only.
    #ifndef NDEBUG
    event_t* next_event = (event_t*)pqueue_peek(env->event_q);
    if (next_event != NULL) {
        if (next_time > next_event->time) {
            lf_print_error_and_exit("_lf_advance_logical_time(): Attempted to move time to " PRINTF_TIME ", which is "
                    "past the head of the event queue, " PRINTF_TIME ".",
                    next_time - start_time, next_event->time - start_time);
        }
    }
    #endif
    if (env->current_tag.time < next_time) {
        env->current_tag.time = next_time;
        env->current_tag.microstep = 0;
    } else if (env->current_tag.time == next_time) {
        env->current_tag.microstep++;
    } else {
        lf_print_error_and_exit("_lf_advance_logical_time(): Attempted to move tag back in time.");
    }
    LF_PRINT_LOG("Advanced (elapsed) tag to " PRINTF_TAG " at physical time " PRINTF_TIME,
        next_time - start_time, env->current_tag.microstep, lf_time_physical_elapsed());
}

/**
 * Variant of schedule_value when the value is an integer.
 * See reactor.h for documentation.
 * @param action Pointer to an action on the self struct.
 */
trigger_handle_t _lf_schedule_int(lf_action_base_t* action, interval_t extra_delay, int value) {
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
    return _lf_schedule_value(action, extra_delay, container, 1);
}

/**

 * Invoke the given reaction
 *
 * @param env Environment in which we are executing.
 * @param reaction The reaction that has just executed.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution (for tracing).
 */
void _lf_invoke_reaction(environment_t* env, reaction_t* reaction, int worker) {
    assert(env != GLOBAL_ENVIRONMENT);

#if !defined(LF_SINGLE_THREADED)
    if (((self_base_t*) reaction->self)->reactor_mutex != NULL) {
        lf_mutex_lock((lf_mutex_t*)((self_base_t*)reaction->self)->reactor_mutex);
    }
#endif

    tracepoint_reaction_starts(env->trace, reaction, worker);
    ((self_base_t*) reaction->self)->executing_reaction = reaction;
    reaction->function(reaction->self);
    ((self_base_t*) reaction->self)->executing_reaction = NULL;
    tracepoint_reaction_ends(env->trace, reaction, worker);


#if !defined(LF_SINGLE_THREADED)
    if (((self_base_t*) reaction->self)->reactor_mutex != NULL) {
        lf_mutex_unlock((lf_mutex_t*)((self_base_t*)reaction->self)->reactor_mutex);
    }
#endif
}

/**
 * For the specified reaction, if it has produced outputs, insert the
 * resulting triggered reactions into the reaction queue.
 * This procedure assumes the mutex lock is NOT held and grabs
 * the lock only when it actually inserts something onto the reaction queue.
 * @param env Environment in which we are executing.
 * @param reaction The reaction that has just executed.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution (for tracing).
 */
void schedule_output_reactions(environment_t *env, reaction_t* reaction, int worker) {
    assert(env != GLOBAL_ENVIRONMENT);

    // If the reaction produced outputs, put the resulting triggered
    // reactions into the reaction queue. As an optimization, if exactly one
    // downstream reaction is enabled by this reaction, then it may be
    // executed immediately in this same thread
    // without going through the reaction queue.
    reaction_t* downstream_to_execute_now = NULL;
    int num_downstream_reactions = 0;
#ifdef FEDERATED_DECENTRALIZED // Only pass down STP violation for federated programs that use decentralized coordination.
    // Extract the inherited STP violation
    bool inherited_STP_violation = reaction->is_STP_violated;
    LF_PRINT_DEBUG("Reaction %s has STP violation status: %d.", reaction->name, reaction->is_STP_violated);
#endif
    LF_PRINT_DEBUG("There are %zu outputs from reaction %s.", reaction->num_outputs, reaction->name);
    for (size_t i=0; i < reaction->num_outputs; i++) {
        if (reaction->output_produced[i] != NULL && *(reaction->output_produced[i])) {
            LF_PRINT_DEBUG("Output %zu has been produced.", i);
            trigger_t** triggerArray = (reaction->triggers)[i];
            LF_PRINT_DEBUG("There are %d trigger arrays associated with output %zu.",
                    reaction->triggered_sizes[i], i);
            for (int j=0; j < reaction->triggered_sizes[i]; j++) {
                trigger_t* trigger = triggerArray[j];
                if (trigger != NULL) {
                    LF_PRINT_DEBUG("Trigger %p lists %d reactions.", trigger, trigger->number_of_reactions);
                    for (int k=0; k < trigger->number_of_reactions; k++) {
                        reaction_t* downstream_reaction = trigger->reactions[k];
#ifdef FEDERATED_DECENTRALIZED // Only pass down tardiness for federated LF programs
                        // Set the is_STP_violated for the downstream reaction
                        if (downstream_reaction != NULL) {
                            downstream_reaction->is_STP_violated = inherited_STP_violation;
                            LF_PRINT_DEBUG("Passing is_STP_violated of %d to the downstream reaction: %s",
                                    downstream_reaction->is_STP_violated, downstream_reaction->name);
                        }
#endif
                        if (downstream_reaction != NULL && downstream_reaction != downstream_to_execute_now) {
                            num_downstream_reactions++;
                            // If there is exactly one downstream reaction that is enabled by this
                            // reaction, then we can execute that reaction immediately without
                            // going through the reaction queue. In multithreaded execution, this
                            // avoids acquiring a mutex lock.
                            // FIXME: Check the earliest deadline on the reaction queue.
                            // This optimization could violate EDF scheduling otherwise.
                            if (num_downstream_reactions == 1 && downstream_reaction->last_enabling_reaction == reaction) {
                                // So far, this downstream reaction is a candidate to execute now.
                                downstream_to_execute_now = downstream_reaction;
                            } else {
                                // If there is a previous candidate reaction to execute now,
                                // it is no longer a candidate.
                                if (downstream_to_execute_now != NULL) {
                                    // More than one downstream reaction is enabled.
                                    // In this case, if we were to execute the downstream reaction
                                    // immediately without changing any queues, then the second
                                    // downstream reaction would be blocked because this reaction
                                    // remains on the executing queue. Hence, the optimization
                                    // is not valid. Put the candidate reaction on the queue.
                                    _lf_trigger_reaction(env, downstream_to_execute_now, worker);
                                    downstream_to_execute_now = NULL;
                                }
                                // Queue the reaction.
                                _lf_trigger_reaction(env, downstream_reaction, worker);
                            }
                        }
                    }
                }
            }
        }
    }
    if (downstream_to_execute_now != NULL) {
        LF_PRINT_LOG("Worker %d: Optimizing and executing downstream reaction now: %s", worker, downstream_to_execute_now->name);
        bool violation = false;
#ifdef FEDERATED_DECENTRALIZED // Only use the STP handler for federated programs that use decentralized coordination
        // If the is_STP_violated for the reaction is true,
        // an input trigger to this reaction has been triggered at a later
        // logical time than originally anticipated. In this case, a special
        // STP handler will be invoked.
        // FIXME: Note that the STP handler will be invoked
        // at most once per logical time value. If the STP handler triggers the
        // same reaction at the current time value, even if at a future superdense time,
        // then the reaction will be invoked and the STP handler will not be invoked again.
        // However, input ports to a federate reactor are network port types so this possibly should
        // be disallowed.
        // @note The STP handler and the deadline handler are not mutually exclusive.
        //  In other words, both can be invoked for a reaction if it is triggered late
        //  in logical time (STP offset is violated) and also misses the constraint on
        //  physical time (deadline).
        // @note In absence of a STP handler, the is_STP_violated will be passed down the reaction
        //  chain until it is dealt with in a downstream STP handler.
        if (downstream_to_execute_now->is_STP_violated == true) {
            // Tardiness has occurred
            LF_PRINT_LOG("Event has STP violation.");
            reaction_function_t handler = downstream_to_execute_now->STP_handler;
            // Invoke the STP handler if there is one.
            if (handler != NULL) {
                // There is a violation and it is being handled here
                // If there is no STP handler, pass the is_STP_violated
                // to downstream reactions.
                violation = true;
                LF_PRINT_LOG("Invoke tardiness handler.");
                (*handler)(downstream_to_execute_now->self);

                // If the reaction produced outputs, put the resulting
                // triggered reactions into the queue or execute them directly if possible.
                schedule_output_reactions(env, downstream_to_execute_now, worker);

                // Reset the tardiness because it has been dealt with in the
                // STP handler
                downstream_to_execute_now->is_STP_violated = false;
                LF_PRINT_DEBUG("Reset reaction's is_STP_violated field to false: %s",
                        downstream_to_execute_now->name);
            }
        }
#endif
        if (downstream_to_execute_now->deadline >= 0LL) {
            // Get the current physical time.
            instant_t physical_time = lf_time_physical();
            // Check for deadline violation.
            if (downstream_to_execute_now->deadline == 0 || physical_time > env->current_tag.time + downstream_to_execute_now->deadline) {
                // Deadline violation has occurred.
                tracepoint_reaction_deadline_missed(env->trace, downstream_to_execute_now, worker);
                violation = true;
                // Invoke the local handler, if there is one.
                reaction_function_t handler = downstream_to_execute_now->deadline_violation_handler;
                if (handler != NULL) {
                    // Assume the mutex is still not held.
                    (*handler)(downstream_to_execute_now->self);

                    // If the reaction produced outputs, put the resulting
                    // triggered reactions into the queue or execute them directly if possible.
                    schedule_output_reactions(env, downstream_to_execute_now, worker);
                }
            }
        }
        if (!violation) {
            // Invoke the downstream_reaction function.
            _lf_invoke_reaction(env, downstream_to_execute_now, worker);

            // If the downstream_reaction produced outputs, put the resulting triggered
            // reactions into the queue (or execute them directly, if possible).
            schedule_output_reactions(env, downstream_to_execute_now, worker);
        }

        // Reset the is_STP_violated because it has been passed
        // down the chain
        downstream_to_execute_now->is_STP_violated = false;
        LF_PRINT_DEBUG("Finally, reset reaction's is_STP_violated field to false: %s",
                downstream_to_execute_now->name);
    }
}

/**
 * Print a usage message.
 * TODO: This is not necessary for NO_TTY
 */
void usage(int argc, const char* argv[]) {
    printf("\nCommand-line arguments: \n\n");
    printf("  -f, --fast [true | false]\n");
    printf("   Whether to wait for physical time to match logical time.\n\n");
    printf("  -o, --timeout <duration> <units>\n");
    printf("   Stop after the specified amount of logical time, where units are one of\n");
    printf("   nsec, usec, msec, sec, minute, hour, day, week, or the plurals of those.\n\n");
    printf("  -k, --keepalive\n");
    printf("   Whether continue execution even when there are no events to process.\n\n");
    printf("  -w, --workers <n>\n");
    printf("   Executed in <n> threads if possible (optional feature).\n\n");
    printf("  -i, --id <n>\n");
    printf("   The ID of the federation that this reactor will join.\n\n");
    #ifdef FEDERATED
    printf("  -r, --rti <n>\n");
    printf("   The address of the RTI, which can be in the form of user@host:port or ip:port.\n\n");
    printf("  -l\n");
    printf("   Send stdout to individual log files for each federate.\n\n");
    #endif

    printf("Command given:\n");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n\n");
}

// Some options given in the target directive are provided here as
// default command-line options.
int default_argc = 0;
const char** default_argv = NULL;


/**
 * Process the command-line arguments. If the command line arguments are not
 * understood, then print a usage message and return 0. Otherwise, return 1.
 * @return 1 if the arguments processed successfully, 0 otherwise.
 * TODO: Not necessary for NO_TTY
 */
int process_args(int argc, const char* argv[]) {
    int i = 1;
    while (i < argc) {
        const char* arg = argv[i++];
        if (strcmp(arg, "-f") == 0 || strcmp(arg, "--fast") == 0) {
            if (argc < i + 1) {
                lf_print_error("--fast needs a boolean.");
                usage(argc, argv);
                return 0;
            }
            const char* fast_spec = argv[i++];
            if (strcmp(fast_spec, "true") == 0) {
                fast = true;
            } else if (strcmp(fast_spec, "false") == 0) {
                fast = false;
            } else {
                lf_print_error("Invalid value for --fast: %s", fast_spec);
            }
        } else if (strcmp(arg, "-o") == 0
                || strcmp(arg, "--timeout") == 0
                || strcmp(arg, "-timeout") == 0) {
            // Tolerate -timeout for legacy uses.
            if (argc < i + 2) {
                lf_print_error("--timeout needs time and units.");
                usage(argc, argv);
                return 0;
            }
            const char* time_spec = argv[i++];
            const char* units = argv[i++];


            #if defined(PLATFORM_ARDUINO)
            duration = atol(time_spec);
            #else
            duration = atoll(time_spec);
            #endif
            
            // A parse error returns 0LL, so check to see whether that is what is meant.
            if (duration == 0LL && strncmp(time_spec, "0", 1) != 0) {
                // Parse error.
                lf_print_error("Invalid time value: %s", time_spec);
                usage(argc, argv);
                return 0;
            }
            if (strncmp(units, "sec", 3) == 0) {
                duration = SEC(duration);
            } else if (strncmp(units, "msec", 4) == 0) {
                duration = MSEC(duration);
            } else if (strncmp(units, "usec", 4) == 0) {
                duration = USEC(duration);
            } else if (strncmp(units, "nsec", 4) == 0) {
                duration = NSEC(duration);
            } else if (strncmp(units, "min", 3) == 0) {
                duration = MINUTE(duration);
            } else if (strncmp(units, "hour", 4) == 0) {
                duration = HOUR(duration);
            } else if (strncmp(units, "day", 3) == 0) {
                duration = DAY(duration);
            } else if (strncmp(units, "week", 4) == 0) {
                duration = WEEK(duration);
            } else {
                // Invalid units.
                lf_print_error("Invalid time units: %s", units);
                usage(argc, argv);
                return 0;
            }
        } else if (strcmp(arg, "-k") == 0 || strcmp(arg, "--keepalive") == 0) {
            if (argc < i + 1) {
                lf_print_error("--keepalive needs a boolean.");
                usage(argc, argv);
                return 0;
            }
            const char* keep_spec = argv[i++];
            if (strcmp(keep_spec, "true") == 0) {
                keepalive_specified = true;
            } else if (strcmp(keep_spec, "false") == 0) {
                keepalive_specified = false;
            } else {
                lf_print_error("Invalid value for --keepalive: %s", keep_spec);
            }
        } else if (strcmp(arg, "-w") == 0 || strcmp(arg, "--workers") == 0) {
            if (argc < i + 1) {
                lf_print_error("--workers needs an integer argument.s");
                usage(argc, argv);
                return 0;
            }
            const char* threads_spec = argv[i++];
            int num_workers = atoi(threads_spec);
            if (num_workers <= 0) {
                lf_print_error("Invalid value for --workers: %s. Using 1.", threads_spec);
                num_workers = 1;
            }
            _lf_number_of_workers = (unsigned int)num_workers;
        }
        #ifdef FEDERATED
          else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--id") == 0) {
            if (argc < i + 1) {
                lf_print_error("--id needs a string argument.");
                usage(argc, argv);
                return 0;
            }
            const char* fid = argv[i++];
            lf_set_federation_id(fid);
            lf_print("Federation ID for executable %s: %s", argv[0], fid);
        } else if (strcmp(arg, "-r") == 0 || strcmp(arg, "--rti") == 0) {
            if (argc < i + 1) {
                lf_print_error("--rti needs a string argument in the form of [user]@[host]:[port].");
                usage(argc, argv);
                return 0;
            }
            parse_rti_code_t code = lf_parse_rti_addr(argv[i++]);
            if (code != SUCCESS) {
                switch (code) {
                    case INVALID_HOST:
                        lf_print_error("--rti needs a valid host");
                        break;
                    case INVALID_PORT:
                        lf_print_error("--rti needs a valid port");
                        break;
                    case INVALID_USER:
                        lf_print_error("--rti needs a valid user");
                        break;
                    case FAILED_TO_PARSE:
                        lf_print_error("Failed to parse address of RTI");
                        break;
                    default:
                        break;
                }
                usage(argc, argv);
                return 0;
            }
        }
        #endif
          else if (strcmp(arg, "--ros-args") == 0) {
              // FIXME: Ignore ROS arguments for now
        } else {
            lf_print_error("Unrecognized command-line argument: %s", arg);
            usage(argc, argv);
            return 0;
        }
    }
    return 1;
}

/**
 * Initialize global variables and start tracing before calling the
 * `_lf_initialize_trigger_objects` function
 */
void initialize_global(void) {
    _lf_count_payload_allocations = 0;
    _lf_count_token_allocations = 0;
    
    environment_t *envs;
    int num_envs = _lf_get_environments(&envs);
    for (int i = 0; i<num_envs; i++) {
        start_trace(envs[i].trace);
    }

    // Federation trace object must be set before `initialize_trigger_objects` is called because it
    //  uses tracing functionality depending on that pointer being set.
    #ifdef FEDERATED
    lf_set_federation_trace_object(envs->trace);
    #endif
    // Call the code-generated function to initialize all actions, timers, and ports
    // This is done for all environments/enclaves at the same time.
    _lf_initialize_trigger_objects() ;
}

/** 
 * Flag to prevent termination function from executing twice and to signal to background
 * threads to terminate.
 */
bool _lf_termination_executed = false;

/** Flag used to disable cleanup operations on abnormal termination. */
bool _lf_normal_termination = false;

/**
 * Report elapsed logical and physical times and report if any
 * memory allocated by set_new, set_new_array, or lf_writable_copy
 * has not been freed.
 */
void termination(void) {
    if (_lf_termination_executed) return;
    _lf_termination_executed = true;

    environment_t *env;
    int num_envs = _lf_get_environments(&env);
    // Invoke the code generated termination function. It terminates the federated related services. 
    // It should only be called for the top-level environment, which, by convention, is the first environment.
    terminate_execution(env);

    // In order to free tokens, we perform the same actions we would have for a new time step.
    for (int i = 0; i < num_envs; i++) {
        if (!env[i].initialized) {
            lf_print_warning("---- Environment %u was never initialized", env[i].id);
            continue;
        }
        LF_PRINT_LOG("---- Terminating environment %u, normal termination: %d", env[i].id, _lf_normal_termination);
        // Stop any tracing, if it is running.
        // No need to acquire a mutex because if this is normal termination, all
        // other threads have stopped, and if it's not, then acquiring a mutex could
        // lead to a deadlock.
        stop_trace_locked(env[i].trace);

        // Skip most cleanup on abnormal termination.
        if (_lf_normal_termination) {
            _lf_start_time_step(&env[i]);

    #ifdef MODAL_REACTORS
            // Free events and tokens suspended by modal reactors.
            _lf_terminate_modal_reactors(&env[i]);
    #endif
            // If the event queue still has events on it, report that.
            if (env[i].event_q != NULL && pqueue_size(env[i].event_q) > 0) {
                lf_print_warning("---- There are %zu unprocessed future events on the event queue.", pqueue_size(env[i].event_q));
                event_t* event = (event_t*)pqueue_peek(env[i].event_q);
                interval_t event_time = event->time - start_time;
                lf_print_warning("---- The first future event has timestamp " PRINTF_TIME " after start time.", event_time);
            }
            // Print elapsed times.
            // If these are negative, then the program failed to start up.
            interval_t elapsed_time = lf_time_logical_elapsed(&env[i]);
            if (elapsed_time >= 0LL) {
                char time_buffer[29]; // 28 bytes is enough for the largest 64 bit number: 9,223,372,036,854,775,807
                lf_comma_separated_time(time_buffer, elapsed_time);
                printf("---- Elapsed logical time (in nsec): %s\n", time_buffer);

                // If start_time is 0, then execution didn't get far enough along
                // to initialize this.
                if (start_time > 0LL) {
                    lf_comma_separated_time(time_buffer, lf_time_physical_elapsed());
                    printf("---- Elapsed physical time (in nsec): %s\n", time_buffer);
                }
            }
        }
    }
    // Skip most cleanup on abnormal termination.
    if (_lf_normal_termination) {
        _lf_free_all_tokens(); // Must be done before freeing reactors.
        // Issue a warning if a memory leak has been detected.
        if (_lf_count_payload_allocations > 0) {
            lf_print_warning("Memory allocated for messages has not been freed.");
            lf_print_warning("Number of unfreed messages: %d.", _lf_count_payload_allocations);
        }
        if (_lf_count_token_allocations > 0) {
            lf_print_warning("Memory allocated for tokens has not been freed!");
            lf_print_warning("Number of unfreed tokens: %d.", _lf_count_token_allocations);
        }
#if !defined(LF_SINGLE_THREADED)
        for (int i = 0; i < _lf_watchdog_count; i++) {
            if (_lf_watchdogs[i].base->reactor_mutex != NULL) {
                free(_lf_watchdogs[i].base->reactor_mutex);
            }
        }
#endif
        _lf_free_all_reactors();

        // Free up memory associated with environment.
        // Do this last so that printed warnings don't access freed memory.
        for (int i = 0; i < num_envs; i++) {
            environment_free(&env[i]);
        }
#if defined LF_ENCLAVES
        free_local_rti();
#endif
    }
}
