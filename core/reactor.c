#if defined(LF_SINGLE_THREADED)
/* Runtime infrastructure for the non-threaded version of the C target of Lingua Franca. */

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
 * @brief Runtime implementation for the non-threaded version of the 
 * C target of Lingua Franca.
 * 
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Erling Jellum <erlingrj@berkeley.edu>}
 */
#include <assert.h>
#include <string.h>

#include "reactor.h"
#include "lf_types.h"
#include "platform.h"
#include "reactor_common.h"
#include "environment.h"

// Embedded platforms with no TTY shouldnt have signals
#if !defined(NO_TTY)
#include <signal.h> // To trap ctrl-c and invoke termination().
#endif

// Global variable defined in tag.c:
extern instant_t start_time;

void lf_set_present(lf_port_base_t* port) {
    if (!port->source_reactor) return;
    environment_t *env = port->source_reactor->environment;
    bool* is_present_field = &port->is_present;
    if (env->is_present_fields_abbreviated_size < env->is_present_fields_size) {
        env->is_present_fields_abbreviated[env->is_present_fields_abbreviated_size]
            = is_present_field;
    }
    env->is_present_fields_abbreviated_size++;
    *is_present_field = true;

    // Support for sparse destination multiports.
    if(port->sparse_record
    		&& port->destination_channel >= 0
			&& port->sparse_record->size >= 0) {
    	size_t next = port->sparse_record->size++;
    	if (next >= port->sparse_record->capacity) {
    		// Buffer is full. Have to revert to the classic iteration.
    		port->sparse_record->size = -1;
    	} else {
    		port->sparse_record->present_channels[next]
				  = port->destination_channel;
    	}
    }
}

/**
 * Wait until physical time matches the given logical time or the time of a 
 * concurrently scheduled physical action, which might be earlier than the 
 * requested logical time.
 * @param env Environment in which we are executing
 * @return 0 if the wait was completed, -1 if it was skipped or interrupted.
 */ 
int wait_until(environment_t* env, instant_t wakeup_time) {
    if (!fast) {
        LF_PRINT_LOG("Waiting for elapsed logical time " PRINTF_TIME ".", wakeup_time - start_time);
        return lf_clock_interruptable_sleep_until_locked(env, wakeup_time);
    }
    return 0;
}

void lf_print_snapshot(environment_t* env) {
    if(LOG_LEVEL > LOG_LEVEL_LOG) {
        LF_PRINT_DEBUG(">>> START Snapshot");
        pqueue_dump(env->reaction_q, env->reaction_q->prt);
        LF_PRINT_DEBUG(">>> END Snapshot");
    }
}

/**
 * Trigger 'reaction'.
 *
 * @param env Environment in which we are executing
 * @param reaction The reaction.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  single-threaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 */
void _lf_trigger_reaction(environment_t* env, reaction_t* reaction, int worker_number) {
    assert(env != GLOBAL_ENVIRONMENT);

#ifdef MODAL_REACTORS
    // Check if reaction is disabled by mode inactivity
    if (!_lf_mode_is_active(reaction->mode)) {
        LF_PRINT_DEBUG("Suppressing downstream reaction %s due inactivity of mode %s.", reaction->name, reaction->mode->name);
        return; // Suppress reaction by preventing entering reaction queue
    }
#endif
    // Do not enqueue this reaction twice.
    if (reaction->status == inactive) {
        LF_PRINT_DEBUG("Enqueueing downstream reaction %s, which has level %lld.",
        		reaction->name, reaction->index & 0xffffLL);
        reaction->status = queued;
        if (pqueue_insert(env->reaction_q, reaction) != 0) {
            lf_print_error_and_exit("Could not insert reaction into reaction_q");
        }
    }
}

/**
 * Execute all the reactions in the reaction queue at the current tag.
 * 
 * @param env Environment in which we are executing
 * @return Returns 1 if the execution should continue and 0 if the execution
 *  should stop.
 */
int _lf_do_step(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);

    // Invoke reactions.
    while(pqueue_size(env->reaction_q) > 0) {
        // lf_print_snapshot();
        reaction_t* reaction = (reaction_t*)pqueue_pop(env->reaction_q);
        reaction->status = running;

        LF_PRINT_LOG("Invoking reaction %s at elapsed logical tag " PRINTF_TAG ".",
        		reaction->name,
                env->current_tag.time - start_time, env->current_tag.microstep);

        bool violation = false;

        // FIXME: These comments look outdated. We may need to update them.
        // If the reaction has a deadline, compare to current physical time
        // and invoke the deadline violation reaction instead of the reaction function
        // if a violation has occurred. Note that the violation reaction will be invoked
        // at most once per logical time value. If the violation reaction triggers the
        // same reaction at the current time value, even if at a future superdense time,
        // then the reaction will be invoked and the violation reaction will not be invoked again.
        if (reaction->deadline >= 0LL) {
            // Get the current physical time.
            instant_t physical_time = lf_time_physical();
            // FIXME: These comments look outdated. We may need to update them.
            // Check for deadline violation.
            // There are currently two distinct deadline mechanisms:
            // local deadlines are defined with the reaction;
            // container deadlines are defined in the container.
            // They can have different deadlines, so we have to check both.
            // Handle the local deadline first.
            if (reaction->deadline == 0 || physical_time > env->current_tag.time + reaction->deadline) {
                LF_PRINT_LOG("Deadline violation. Invoking deadline handler.");
                tracepoint_reaction_deadline_missed(env->trace, reaction, 0);
                // Deadline violation has occurred.
                violation = true;
                // Invoke the local handler, if there is one.
                reaction_function_t handler = reaction->deadline_violation_handler;
                if (handler != NULL) {
                    (*handler)(reaction->self);
                    // If the reaction produced outputs, put the resulting
                    // triggered reactions into the queue.
                    schedule_output_reactions(env, reaction, 0);
                }
            }
        }

        if (!violation) {
            // Invoke the reaction function.
            _lf_invoke_reaction(env, reaction, 0);   // 0 indicates single-threaded.

            // If the reaction produced outputs, put the resulting triggered
            // reactions into the queue.
            schedule_output_reactions(env, reaction, 0);
        }
        // There cannot be any subsequent events that trigger this reaction at the
        //  current tag, so it is safe to conclude that it is now inactive.
        reaction->status = inactive;
    }

#ifdef MODAL_REACTORS
    // At the end of the step, perform mode transitions
    _lf_handle_mode_changes(env);
#endif

    if (lf_tag_compare(env->current_tag, env->stop_tag) >= 0) {
        return 0;
    }

    return 1;
}

// Wait until physical time matches or exceeds the time of the least tag
// on the event queue. If there is no event in the queue, return 0.
// After this wait, advance current_tag of the environment to match
// this tag. Then pop the next event(s) from the
// event queue that all have the same tag, and extract from those events
// the reactions that are to be invoked at this logical time.
// Sort those reactions by index (determined by a topological sort)
// and then execute the reactions in order. Each reaction may produce
// outputs, which places additional reactions into the index-ordered
// priority queue. All of those will also be executed in order of indices.
// If the -timeout option has been given on the command line, then return
// 0 when the logical time duration matches the specified duration.
// Also return 0 if there are no more events in the queue and
// the keepalive command-line option has not been given.
// Otherwise, return 1.
int next(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);

    // Enter the critical section and do not leave until we have
    // determined which tag to commit to and start invoking reactions for.
    LF_CRITICAL_SECTION_ENTER(env);
    event_t* event = (event_t*)pqueue_peek(env->event_q);
    //pqueue_dump(event_q, event_q->prt);
    // If there is no next event and -keepalive has been specified
    // on the command line, then we will wait the maximum time possible.
    tag_t next_tag = FOREVER_TAG_INITIALIZER;
    if (event == NULL) {
        // No event in the queue.
        if (!keepalive_specified) {
            _lf_set_stop_tag( env,
                (tag_t){.time=env->current_tag.time, .microstep=env->current_tag.microstep+1}
            );
        }
    } else {
        next_tag.time = event->time;
        // Deduce the microstep
        if (next_tag.time == env->current_tag.time) {
            next_tag.microstep = env->current_tag.microstep + 1;
        } else {
            next_tag.microstep = 0;
        }
    }

    if (_lf_is_tag_after_stop_tag(env, next_tag)) {
        // Cannot process events after the stop tag.
        next_tag = env->stop_tag;
    }

    LF_PRINT_LOG("Next event (elapsed) time is " PRINTF_TIME ".", next_tag.time - start_time);
    // Wait until physical time >= event.time.
    int finished_sleep = wait_until(env, next_tag.time);
    LF_PRINT_LOG("Next event (elapsed) time is " PRINTF_TIME ".", next_tag.time - start_time);
    if (finished_sleep != 0) {
        LF_PRINT_DEBUG("***** wait_until was interrupted.");
        // Sleep was interrupted. This could happen when a physical action
        // gets scheduled from an interrupt service routine.
        // In this case, check the event queue again to make sure to
        // advance time to the correct tag.
        LF_CRITICAL_SECTION_EXIT(env);
        return 1;
    }
    // Advance current time to match that of the first event on the queue.
    // We can now leave the critical section. Any events that will be added
    // to the queue asynchronously will have a later tag than the current one.
    _lf_advance_logical_time(env, next_tag.time);
    
    // Trigger shutdown reactions if appropriate.
    if (lf_tag_compare(env->current_tag, env->stop_tag) >= 0) {        
        _lf_trigger_shutdown_reactions(env);
    }

    // Invoke code that must execute before starting a new logical time round,
    // such as initializing outputs to be absent.
    _lf_start_time_step(env);

    // Pop all events from event_q with timestamp equal to env->current_tag.time,
    // extract all the reactions triggered by these events, and
    // stick them into the reaction queue.
    _lf_pop_events(env);
    LF_CRITICAL_SECTION_EXIT(env);

    return _lf_do_step(env);
}

void lf_request_stop(void) {
    // There is only one enclave, so get its environment.
    environment_t *env;
    int num_environments = _lf_get_environments(&env);
    assert(num_environments == 1);

	tag_t new_stop_tag;
	new_stop_tag.time = env->current_tag.time;
	new_stop_tag.microstep = env->current_tag.microstep + 1;
	_lf_set_stop_tag(env, new_stop_tag);
}

/**
 * Return false.
 * @param reaction The reaction.
 */
bool _lf_is_blocked_by_executing_reaction(void) {
    return false;
}

/**
 * The main loop of the LF program.
 *
 * An unambiguous function name that can be called
 * by external libraries.
 *
 * Note: In target languages that use the C core library,
 * there should be an unambiguous way to execute the LF
 * program's main function that will not conflict with
 * other main functions that might get resolved and linked
 * at compile time.
 */
int lf_reactor_c_main(int argc, const char* argv[]) {
    // Invoke the function that optionally provides default command-line options.
    _lf_set_default_command_line_options();
    _lf_initialize_clock();

    LF_PRINT_DEBUG("Processing command line arguments.");
    if (process_args(default_argc, default_argv)
            && process_args(argc, argv)) {
        LF_PRINT_DEBUG("Processed command line arguments.");
        LF_PRINT_DEBUG("Registering the termination function.");
        if (atexit(termination) != 0) {
            lf_print_warning("Failed to register termination function!");
        }
        // The above handles only "normal" termination (via a call to exit).
        // As a consequence, we need to also trap Ctrl-C, which issues a SIGINT,
        // and cause it to call exit.
        // Embedded platforms with NO_TTY have no concept of a signal; for those, we exclude this call.
#ifndef NO_TTY
        signal(SIGINT, exit);
#endif
        // Create and initialize the environment
        _lf_create_environments();   // code-generated function
        environment_t *env;
        int num_environments = _lf_get_environments(&env);
        LF_ASSERT(num_environments == 1,
            "Found %d environments. Only 1 can be used with the single-threaded runtime", num_environments);
        
        LF_PRINT_DEBUG("Initializing.");
        initialize_global();
        // Set start time
        start_time = lf_time_physical();

        LF_PRINT_DEBUG("NOTE: FOREVER is displayed as " PRINTF_TAG " and NEVER as " PRINTF_TAG,
                FOREVER_TAG.time - start_time, FOREVER_TAG.microstep,
                NEVER_TAG.time - start_time, 0);

        environment_init_tags(env, start_time, duration);
        // Start tracing if enabled.
        start_trace(env->trace);
#ifdef MODAL_REACTORS
        // Set up modal infrastructure
        _lf_initialize_modes(env);
#endif
        _lf_trigger_startup_reactions(env);
        _lf_initialize_timers(env);
        // If the stop_tag is (0,0), also insert the shutdown
        // reactions. This can only happen if the timeout time
        // was set to 0.
        if (lf_tag_compare(env->current_tag, env->stop_tag) >= 0) {
            _lf_trigger_shutdown_reactions(env);
        }
        LF_PRINT_DEBUG("Running the program's main loop.");
        // Handle reactions triggered at time (T,m).
        env->execution_started = true;
        if (_lf_do_step(env)) {
            while (next(env) != 0);
        }
        _lf_normal_termination = true;
        return 0;
    } else {
        return -1;
    }
}

/**
 * @brief Notify of new event by calling the single-threaded platform API
 * @param env Environment in which we are executing.
 */
int lf_notify_of_event(environment_t* env) {
    return _lf_single_threaded_notify_of_event();
}

int lf_critical_section_enter(environment_t* env) {
    return lf_disable_interrupts_nested();
}

int lf_critical_section_exit(environment_t* env) {
    return lf_enable_interrupts_nested();
}
#endif
