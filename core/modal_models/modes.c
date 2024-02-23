/*************
Copyright (c) 2021, Kiel University.

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
 * Runtime infrastructure for modes in the C target of Lingua Franca.
 * This file contains functions that handle the processing of mode transitions.
 * It works together with other changes across the runtime implementation.
 *
 * Any mode related code will only work in the presence of the MODAL_REACTORS
 * definition.
 *
 * This file is intended for direct include in reactor_common.c.
 *
 * @author{Alexander Schulz-Rosengarten <als@informatik.uni-kiel.de>}
 * @author{Soroush Bateni <soroush@utdallas.edu}
 */
#ifdef MODAL_REACTORS

#include <string.h>

#include "low_level_platform.h"
#include "lf_types.h"
#include "modes.h"
#include "reactor.h"
#include "reactor_common.h"
#include "api/schedule.h"

// Bit masks for the internally used flags on modes
#define _LF_MODE_FLAG_MASK_ACTIVE        (1 << 0)
#define _LF_MODE_FLAG_MASK_NEEDS_STARTUP (1 << 1)
#define _LF_MODE_FLAG_MASK_HAD_STARTUP   (1 << 2)
#define _LF_MODE_FLAG_MASK_NEEDS_RESET   (1 << 3)

// ----------------------------------------------------------------------------

// Forward declaration of functions and variables supplied by reactor_common.c
void _lf_trigger_reaction(environment_t* env, reaction_t* reaction, int worker_number);
event_t* _lf_create_dummy_events(environment_t* env, trigger_t* trigger, instant_t time, event_t* next, microstep_t offset);

// ----------------------------------------------------------------------------

// Linked list element for suspended events in inactive modes
typedef struct _lf_suspended_event {
    struct _lf_suspended_event* next;
    event_t* event;
} _lf_suspended_event_t;
_lf_suspended_event_t* _lf_suspended_events_head = NULL; // Start of linked collection of suspended events (managed automatically!)
int _lf_suspended_events_num = 0; // Number of suspended events (managed automatically!)
_lf_suspended_event_t* _lf_unsused_suspended_events_head = NULL; // Internal collection of reusable list elements (managed automatically!)

/**
 * Save the given event as suspended.
 */
void _lf_add_suspended_event(event_t* event) {
    _lf_suspended_event_t* new_suspended_event;
    if (_lf_unsused_suspended_events_head != NULL) {
        new_suspended_event = _lf_unsused_suspended_events_head;
        _lf_unsused_suspended_events_head = _lf_unsused_suspended_events_head->next;
    } else {
        new_suspended_event = (_lf_suspended_event_t*) malloc(sizeof(_lf_suspended_event_t));
    }

    new_suspended_event->event = event;
    new_suspended_event->next = _lf_suspended_events_head; // prepend
    _lf_suspended_events_num++;

    _lf_suspended_events_head = new_suspended_event;
}

/**
 * Remove the given node from the list of suspended events and
 * Returns the next element in the list.
 */
_lf_suspended_event_t* _lf_remove_suspended_event(_lf_suspended_event_t* event) {
    _lf_suspended_event_t* next = event->next;

    // Clear content
    event->event = NULL;
    event->next = NULL;
    _lf_suspended_events_num--;

    // Store for recycling
    if (_lf_unsused_suspended_events_head == NULL) {
        _lf_unsused_suspended_events_head = event;
    } else {
        event->next = _lf_unsused_suspended_events_head;
        _lf_unsused_suspended_events_head = event;
    }

    if (_lf_suspended_events_head == event) {
        _lf_suspended_events_head = next; // Adjust head
    } else {
        _lf_suspended_event_t* predecessor = _lf_suspended_events_head;
        while(predecessor->next != event && predecessor != NULL) {
                predecessor = predecessor->next;
        }
        if (predecessor != NULL) {
                predecessor->next = next; // Remove from linked list
        }
    }

    return next;
}

// ----------------------------------------------------------------------------

/**
 * Return true if the given mode is active.
 * This includes checking all enclosing modes.
 * If any of those is inactive, then so is this one.
 *
 * @param mode The mode instance to check.
 */
bool _lf_mode_is_active(reactor_mode_t* mode) {
    if (mode != NULL) {
        // Use cached value (redundant data structure)
        return mode->flags & _LF_MODE_FLAG_MASK_ACTIVE;
    }
    return true;
}

/**
 * Fallback implementation of _lf_mode_is_active.
 * Does not rely on cached activity flag.
 * (More reliable; for debugging).
 *
 * @param mode The mode instance to check.
 */
bool _lf_mode_is_active_fallback(reactor_mode_t* mode) {
    if (mode != NULL) {
        LF_PRINT_DEBUG("Checking mode state of %s", mode->name);
        reactor_mode_state_t* state = mode->state;
        while (state != NULL) {
            // If this or any parent mode is inactive, return inactive
            if (state->current_mode != mode) {
                LF_PRINT_DEBUG(" => Mode is inactive");
                return false;
            }
            mode = state->parent_mode;
            if (mode != NULL) {
                state = mode->state;
            } else {
                state = NULL;
            }
        }
        LF_PRINT_DEBUG(" => Mode is active");
    }
    return true;
}

/**
 * Initialize internal data structures and caches for modes.
 * Must be invoked only once before any execution.
 * Must be invoked before startup reactions are triggered.
 *
 * @param state An array of all mode states of modal reactor instance
 * @param states_size
 */
void _lf_initialize_mode_states(environment_t* env, reactor_mode_state_t* states[], int states_size) {
    LF_PRINT_DEBUG("Modes: Initialization");
    // Initialize all modes (top down for correct active flags)
    for (int i = 0; i < states_size; i++) {
        reactor_mode_state_t* state = states[i];
        if (state != NULL && _lf_mode_is_active(state->parent_mode)) {
            // If there is no enclosing mode or the parent is marked active,
            // then activate the active mode (same as initial at this point)
            // and request startup.
            state->current_mode->flags |= _LF_MODE_FLAG_MASK_ACTIVE | _LF_MODE_FLAG_MASK_NEEDS_STARTUP;
        }
    }
    // Register execution of special triggers
    env->modes->triggered_reactions_request |= _LF_MODE_FLAG_MASK_NEEDS_STARTUP;
}

/**
 * Handles the triggering of startup and reset reactions INSIDE modes.
 * A startup reaction will be triggered once upon first entry of mode.
 * Reset reactions are triggered when a mode is entered via a reset transition.
 * Shutdown reactions are triggered at program's end for those mode that had a
 * startup and will bypass mode inactivity.
 *
 * This function is supposed to be call from the generated functions:
 * - _lf_trigger_startup_reactions()
 * - _lf_handle_mode_triggered_reactions()
 *
 * The generated functions must handle all startup/shutdown reactions outside
 * of modes, as these are excluded here.
 *
 * Bookkeeping and triggering of reaction is internally connected via flags set by
 * _lf_initialize_mode_states and _lf_process_mode_changes.
 *
 * @param startup_reactions An array of all startup reactions
 * @param startup_reactions_size
 * @param reset_reactions An array of all reset reactions
 * @param reset_reactions_size
 * @param state An array of all mode states of modal reactor instance
 * @param states_size
 *
 */
void _lf_handle_mode_startup_reset_reactions(
        environment_t* env,
        reaction_t** startup_reactions,
        int startup_reactions_size,
        reaction_t** reset_reactions,
        int reset_reactions_size,
        reactor_mode_state_t* states[],
        int states_size
) {
    // Handle startup reactions
    if (env->modes->triggered_reactions_request & _LF_MODE_FLAG_MASK_NEEDS_STARTUP) {
        if (startup_reactions != NULL) {
            for (int i = 0; i < startup_reactions_size; i++) {
                reaction_t* reaction = startup_reactions[i];
                if (reaction->mode != NULL) {
                    if(reaction->status == inactive
                            && _lf_mode_is_active(reaction->mode)
                            && reaction->mode->flags & _LF_MODE_FLAG_MASK_NEEDS_STARTUP
                    ) {
                        // Trigger reaction if not already triggered, is active,
                        // and requires startup
                        _lf_trigger_reaction(env, reaction, -1);
                    }
                }
            }
        }
    }

    // Handle reset reactions
    if (env->modes->triggered_reactions_request & _LF_MODE_FLAG_MASK_NEEDS_RESET) {
        if (reset_reactions != NULL) {
            for (int i = 0; i < reset_reactions_size; i++) {
                reaction_t* reaction = reset_reactions[i];
                if (reaction->mode != NULL) {
                    if(reaction->status == inactive
                            && _lf_mode_is_active(reaction->mode)
                            && reaction->mode->flags & _LF_MODE_FLAG_MASK_NEEDS_RESET
                    ) {
                        // Trigger reaction if not already triggered, is active,
                        // and requires reset
                        _lf_trigger_reaction(env, reaction, -1);
                    }
                }
            }
        }
    }

    // Reset the flags in all active modes.
    // Hence, register that a mode had a startup even if there are no startup
    // reactions to make sure that shutdown is executed properly.
    for (int i = 0; i < states_size; i++) {
        reactor_mode_state_t* state = states[i];
        if (state != NULL && _lf_mode_is_active(state->current_mode)) {
            // Clear and save execution of startup for shutdown
            if (state->current_mode->flags & _LF_MODE_FLAG_MASK_NEEDS_STARTUP) {
                state->current_mode->flags |= _LF_MODE_FLAG_MASK_HAD_STARTUP;
                state->current_mode->flags &= ~_LF_MODE_FLAG_MASK_NEEDS_STARTUP;
            }

            // Clear reset flag
            state->current_mode->flags &= ~_LF_MODE_FLAG_MASK_NEEDS_RESET;
        }
    }

    // Clear request
    env->modes->triggered_reactions_request = 0;
}

/**
 * Handles the triggering of shutdown reactions INSIDE modes.
 * Shutdown reactions are triggered at program's end for those modes that had a
 * startup and will bypass mode inactivity.
 *
 * This function is supposed to be call from the generated function:
 * - _lf_trigger_shutdown_reactions()
 *
 * The generated functions must handle all shutdown reactions outside
 * of modes, as these are excluded here.
 *
 * @param shutdown_reactions An array of all shutdown reactions
 * @param shutdown_reactions_size
 *
 */
void _lf_handle_mode_shutdown_reactions(
        environment_t* env,
        reaction_t** shutdown_reactions,
        int shutdown_reactions_size
) {
    if (shutdown_reactions != NULL) {
        for (int i = 0; i < shutdown_reactions_size; i++) {
            reaction_t* reaction = shutdown_reactions[i];
            if (reaction->mode != NULL) {
                if (reaction->mode->flags & _LF_MODE_FLAG_MASK_HAD_STARTUP) { // if mode had startup
                    // Release the reaction from its association with the mode.
                    // This will effectively bypass the mode activity check.
                    // This assumes that the reaction will never be trigger/used after shutdown.
                    // If that is not the case, a temporary bypassing mechanism should be implemented.
                    reaction->mode = NULL;

                    if(reaction->status == inactive) {
                        // Trigger reaction if not already triggered
                        _lf_trigger_reaction(env, reaction, -1);
                    }
                }
            }
        }
    }
}

/**
 * Perform transitions in all modal reactors.
 *
 * @param state An array of all mode states in modal reactor instances,
 *      which must be ordered hierarchically, where an enclosing mode must come before the inner mode.
 * @param states_size
 * @param reset_data A list of initial values for reactor state variables that should be automatically reset.
 * @param reset_data_size
 * @param timer_triggers Array of pointers to timer triggers.
 * @param timer_triggers_size
 */
void _lf_process_mode_changes(
    environment_t* env,
    reactor_mode_state_t* states[],
    int states_size,
    mode_state_variable_reset_data_t reset_data[],
    int reset_data_size,
    trigger_t* timer_triggers[],
    int timer_triggers_size
) {
    bool transition = false; // any mode change in this step

    // Detect mode changes (top down for hierarchical reset)
    for (int i = 0; i < states_size; i++) {
        reactor_mode_state_t* state = states[i];
        if (state != NULL) {
            // Hierarchical reset: if this mode has parent that is entered in
            // this step with a reset this reactor has to enter its initial mode
            if (state->parent_mode != NULL
                    && state->parent_mode->state != NULL
                    && state->parent_mode->state->next_mode == state->parent_mode
                    && state->parent_mode->state->mode_change == reset_transition
            ){
                // Reset to initial state.
                state->next_mode = state->initial_mode;
                // Enter with reset, to cascade it further down.
                state->mode_change = reset_transition;
                LF_PRINT_DEBUG("Modes: Hierarchical mode reset to %s when entering %s.",
                        state->initial_mode->name, state->parent_mode->name);
            }

            // Handle effect of entering next mode
            if (state->next_mode != NULL) {
                LF_PRINT_DEBUG("Modes: Transition to %s.", state->next_mode->name);
                transition = true;

                if (state->mode_change == reset_transition) {
                    // Reset state variables (if explicitly requested for automatic reset).
                    // The generated code will not register all state variables by default.
                    // Usually the reset trigger is used.
                    for (int j = 0; j < reset_data_size; j++) {
                        mode_state_variable_reset_data_t data = reset_data[j];
                        if (data.mode == state->next_mode) {
                            LF_PRINT_DEBUG("Modes: Reseting state variable.");
                            memcpy(data.target, data.source, data.size);
                        }
                    }

                    // Handle timers that have a period of 0. These timers will only trigger
                    // once and will not be on the event_q after their initial triggering.
                    // Therefore, the logic above cannot handle these timers. We need
                    // to trigger these timers manually if there is a reset transition.
                    for (int j = 0; j < timer_triggers_size; j++) {
                        trigger_t* timer = timer_triggers[j];
                        if (timer->period == 0 && timer->mode == state->next_mode) {
                            lf_schedule_trigger(env, timer, timer->offset, NULL);
                        }
                    }
                }

                // Reset/Reactivate previously suspended events of next state
                _lf_suspended_event_t* suspended_event = _lf_suspended_events_head;
                while(suspended_event != NULL) {
                    event_t* event = suspended_event->event;
                    if (event != NULL && event->trigger != NULL && event->trigger->mode == state->next_mode) {
                        if (state->mode_change == reset_transition) { // Reset transition
                            if (event->trigger->is_timer) { // Only reset timers
                                trigger_t* timer = event->trigger;

                                LF_PRINT_DEBUG("Modes: Re-enqueuing reset timer.");
                                // Reschedule the timer with no additional delay.
                                // This will take care of super dense time when offset is 0.
                                lf_schedule_trigger(env, timer, event->trigger->offset, NULL);
                            }
                            // No further processing; drops all events upon reset (timer event was recreated by schedule and original can be removed here)
                        } else if (state->next_mode != state->current_mode && event->trigger != NULL) { // History transition to a different mode
                            // Remaining time that the event would have been waiting before mode was left
                            instant_t local_remaining_delay = event->time - (state->next_mode->deactivation_time != 0 ? state->next_mode->deactivation_time : lf_time_start());
                            tag_t current_logical_tag = env->current_tag;

                            // Reschedule event with original local delay
                            LF_PRINT_DEBUG("Modes: Re-enqueuing event with a suspended delay of " PRINTF_TIME
                            		" (previous TTH: " PRINTF_TIME ", Mode suspended at: " PRINTF_TIME ").",
                            		local_remaining_delay, event->time, state->next_mode->deactivation_time);
                            tag_t schedule_tag = {.time = current_logical_tag.time + local_remaining_delay, .microstep = (local_remaining_delay == 0 ? current_logical_tag.microstep + 1 : 0)};
                            _lf_schedule_at_tag(env, event->trigger, schedule_tag, event->token);

                            // Also schedule events stacked up in super dense time.
                            event_t* e = event;
                            while (e->next != NULL) {
                                schedule_tag.microstep++;
                                _lf_schedule_at_tag(env, e->next->trigger, schedule_tag, e->next->token);
                                event_t* tmp = e->next;
                                e = tmp->next;
                                // A fresh event was created by schedule, hence, recycle old one
                                lf_recycle_event(env, tmp);
                            }
                        }
                        // A fresh event was created by schedule, hence, recycle old one
                        lf_recycle_event(env, event);

                        // Remove suspended event and continue
                        suspended_event = _lf_remove_suspended_event(suspended_event);
                    } else {
                        suspended_event = suspended_event->next;
                    }
                }
            }
        }
    }

    // Handle leaving active mode in all states
    if (transition) {
        // Set new active mode and clear mode change flags
        // (top down for correct active flags)
        for (int i = 0; i < states_size; i++) {
            reactor_mode_state_t* state = states[i];
            if (state != NULL) {
                // Clear cached active flag on active state, because
                // parent activity might have changed or active state may change.
                state->current_mode->flags &= ~_LF_MODE_FLAG_MASK_ACTIVE;

                // Apply transition effect
                if (state->next_mode != NULL) {
                    // Save time when mode was left to handle suspended events in the future
                    state->current_mode->deactivation_time = lf_time_logical(env);

                    // Apply transition
                    state->current_mode = state->next_mode;

                    // Trigger startup reactions if entered first time
                    if (!(state->current_mode->flags & _LF_MODE_FLAG_MASK_HAD_STARTUP)) {
                        state->current_mode->flags |= _LF_MODE_FLAG_MASK_NEEDS_STARTUP;
                    }

                    // Trigger reset reactions
                    if (state->mode_change == reset_transition) {
                        state->current_mode->flags |= _LF_MODE_FLAG_MASK_NEEDS_RESET;
                    } else {
                        // Needs to be cleared because flag could be there from previous
                        // entry (with subsequent inactivity) which is now obsolete
                        state->current_mode->flags &= ~_LF_MODE_FLAG_MASK_NEEDS_RESET;
                    }

                    state->next_mode = NULL;
                    state->mode_change = no_transition;
                }

                // Compute new cached activity flag
                if (_lf_mode_is_active(state->parent_mode)) {
                    // If there is no enclosing or the parent is marked active,
                    // then set active flag on active mode
                    state->current_mode->flags |= _LF_MODE_FLAG_MASK_ACTIVE;

                    // Register execution of special triggers
                    // This is not done when setting the flag because actual triggering
                    // might be delayed by parent mode inactivity.
                    env->modes->triggered_reactions_request |= state->current_mode->flags &
                            (_LF_MODE_FLAG_MASK_NEEDS_STARTUP | _LF_MODE_FLAG_MASK_NEEDS_RESET);
                }
            }
        }

        // Retract all events from the event queue that are associated with now inactive modes
        if (env->event_q != NULL) {
            size_t q_size = pqueue_size(env->event_q);
            if (q_size > 0) {
                event_t** delayed_removal = (event_t**) calloc(q_size, sizeof(event_t*));
                size_t delayed_removal_count = 0;

                // Find events
                for (size_t i = 0; i < q_size; i++) {
                    event_t* event = (event_t*)env->event_q->d[i + 1]; // internal queue data structure omits index 0
                    if (event != NULL && event->trigger != NULL && !_lf_mode_is_active(event->trigger->mode)) {
                        delayed_removal[delayed_removal_count++] = event;
                        // This will store the event including possibly those chained up in super dense time
                        _lf_add_suspended_event(event);
                    }
                }

                // Events are removed delayed in order to allow linear iteration over the queue
                LF_PRINT_DEBUG("Modes: Pulling %zu events from the event queue to suspend them. %d events are now suspended.",
                		delayed_removal_count, _lf_suspended_events_num);
                for (size_t i = 0; i < delayed_removal_count; i++) {
                    pqueue_remove(env->event_q, delayed_removal[i]);
                }

                free(delayed_removal);
            }
        }

        if (env->modes->triggered_reactions_request) {
            // Insert a dummy event in the event queue for the next microstep to make
            // sure startup/reset reactions (if any) are triggered as soon as possible.
            pqueue_insert(env->event_q, _lf_create_dummy_events(env, NULL, env->current_tag.time, NULL, 1));
        }
    }
}

/**
 * Release internal data structures for modes.
 * - Frees all suspended events.
 */
void _lf_terminate_modal_reactors(environment_t* env) {
    _lf_suspended_event_t* suspended_event = _lf_suspended_events_head;
    while(suspended_event != NULL) {
        lf_recycle_event(env, suspended_event->event);
        _lf_suspended_event_t* next = suspended_event->next;
        free(suspended_event);
        suspended_event = next;
    }
    _lf_suspended_events_head = NULL;
    _lf_suspended_events_num = 0;

    // Also free suspended_event elements stored for recycling
    suspended_event = _lf_unsused_suspended_events_head;
    while(suspended_event != NULL) {
        _lf_suspended_event_t* next = suspended_event->next;
        free(suspended_event);
        suspended_event = next;
    }
    _lf_unsused_suspended_events_head = NULL;
}
void _lf_initialize_modes(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);
    if (env->modes) {
        _lf_initialize_mode_states(
            env, 
            env->modes->modal_reactor_states, 
            env->modes->modal_reactor_states_size);
    }
}
void _lf_handle_mode_changes(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);
    if (env->modes) {
        _lf_process_mode_changes(
            env, 
            env->modes->modal_reactor_states, 
            env->modes->modal_reactor_states_size, 
            env->modes->state_resets, 
            env->modes->state_resets_size, 
            env->timer_triggers, 
            env->timer_triggers_size
        );
    }
}

void _lf_handle_mode_triggered_reactions(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);
    if (env->modes) {
        _lf_handle_mode_startup_reset_reactions(
            env, env->startup_reactions, env->startup_reactions_size,
            env->reset_reactions, env->reset_reactions_size,
            env->modes->modal_reactor_states, env->modes->modal_reactor_states_size);
    }
}

#endif
