/**
 * @file
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @author Chris Gill
 * @author Mehrdad Niknami
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Definitions for the C target of Lingua Franca shared by threaded and unthreaded versions.
 * 
 * This header file defines functions that programmers use in the body of reactions for reading and
 * writing inputs and outputs and scheduling future events. Other functions that might be useful to
 * application programmers are also defined here.
 * 
 * Many of these functions have macro wrappers defined in reaction_macros.h.
 */

#ifndef REACTOR_H
#define REACTOR_H

#include "lf_types.h"
#include "modes.h"     // Modal model support
#include "port.h"
#include "tag.h"       // Time-related functions.
#include "clock.h"       // Time-related functions.
#include "trace.h"
#include "util.h"

//////////////////////  Function Declarations  //////////////////////

/**
 * @brief Return true if the provided tag is after stop tag.
 * @param env Environment in which we are executing.
 * @param tag The tag to check against stop tag
 */
bool lf_is_tag_after_stop_tag(environment_t* env, tag_t tag);

/**
 * @brief Mark the given port's is_present field as true.
 * @param port A pointer to the port struct as an `lf_port_base_t*`.
 */
void lf_set_present(lf_port_base_t* port);

/**
 * @brief Set the stop tag if it is less than the stop tag of the specified environment.
 * @note In threaded programs, the environment's mutex must be locked before calling this function.
 */
void lf_set_stop_tag(environment_t* env, tag_t tag);

#ifdef FEDERATED_DECENTRALIZED

/**
 * @brief Return the global STP offset on advancement of logical time for federated execution.
 */
interval_t lf_get_stp_offset(void);

/**
 * @brief Set the global STP offset on advancement of logical time for federated execution.
 * @param offset A positive time value to be applied as the STP offset.
 */
void lf_set_stp_offset(interval_t offset);

#endif FEDERATED_DECENTRALIZED

/**
 * @brief Print a snapshot of the priority queues used during execution (for debugging).
 * @param env The environment in which we are executing.
 */
void lf_print_snapshot(environment_t* env);

/**
 * @brief Request a stop to execution as soon as possible.
 * 
 * In a non-federated execution with only a single enclave, this will occur
 * one microstep later than the current tag. In a federated execution or when
 * there is more than one enclave, it will likely occur at a later tag determined
 * by the RTI so that all federates and enclaves stop at the same tag.
 */
void lf_request_stop(void);

/**
 * @brief Allocate memory and record on the specified allocation record (a self struct).
 * 
 * This will allocate memory using calloc (so the allocated memory is zeroed out)
 * and record the allocated memory on the specified self struct so that
 * it will be freed when calling {@link free_reactor(self_base_t)}.
 * 
 * @param count The number of items of size 'size' to accomodate.
 * @param size The size of each item.
 * @param head Pointer to the head of a list on which to record
 *  the allocation, or NULL to not record it.
 * @return A pointer to the allocated memory.
 */
void* lf_allocate(size_t count, size_t size, struct allocation_record_t** head);

/**
 * @brief Free memory on the specified allocation record (a self struct).
 * 
 * This will mark the allocation record empty by setting `*head` to NULL.
 * If the argument is NULL, do nothing.
 * 
 * @param head Pointer to the head of a list on which allocations are recorded.
 */
void lf_free(struct allocation_record_t** head);

/**
 * @brief Allocate memory for a new runtime instance of a reactor.
 * 
 * This records the reactor on the list of reactors to be freed at
 * termination of the program. If you plan to free the reactor before
 * termination of the program, use
 * {@link lf_allocate(size_t, size_t, allocation_record_t**)}
 * with a null last argument instead.
 * 
 * @param size The size of the self struct, obtained with sizeof().
 */
void* lf_new_reactor(size_t size);

/**
 * @brief Free all the reactors that are allocated with {@link #lf_new_reactor(size_t)}.
 */
void lf_free_all_reactors(void);

/**
 * @brief Free the specified reactor.
 * 
 * This will free the memory recorded on the allocations list of the specified reactor
 * and then free the specified self struct.
 * @param self The self struct of the reactor.
 */
void lf_free_reactor(self_base_t *self);

/**
 * Generated function that resets outputs to be absent at the
 * start of a new time step.
 * @param env The environment in which we are executing
 */
void _lf_start_time_step(environment_t *env);

/**
 * Generated function that produces a table containing all triggers
 * (i.e., inputs, timers, and actions).
 */
void _lf_initialize_trigger_objects();

/**
 * Pop all events from event_q with timestamp equal to current_time, extract all
 * the reactions triggered by these events, and stick them into the reaction
 * queue.
 * @param env The environment in which we are executing
 */
void _lf_pop_events(environment_t *env);

/**
 * Internal version of the lf_schedule() function, used by generated
 * _lf_start_timers() function.
 * @param env The environment in which we are executing
 * @param trigger The action or timer to be triggered.
 * @param delay Offset of the event release.
 * @param token The token payload.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t _lf_schedule(environment_t* env, trigger_t* trigger, interval_t delay, lf_token_t* token);

/**
 * Function to initialize mutexes for watchdogs
 */
void _lf_initialize_watchdog_mutexes(void);

/**
 * @brief Get the array of ids of enclaves directly upstream of the specified enclave.
 * This updates the specified result pointer to point to a statically allocated array of IDs
 * and returns the length of the array. The implementation is code-generated.
 * 
 * @param enclave_id The enclave for which to report upstream IDs.
 * @param result The pointer to dereference and update to point to the resulting array.
 * @return The number of direct upstream enclaves.
 */
int _lf_get_upstream_of(int enclave_id, int** result);

/**
 * @brief Get the array of ids of enclaves directly downstream of the specified enclave.
 * This updates the specified result pointer to point to a statically allocated array of IDs
 * and returns the length of the array. The implementation is code-generated.
 * 
 * @param enclave_id The enclave for which to report downstream IDs.
 * @param result The pointer to dereference and update to point to the resulting array.
 * @return The number of direct downstream enclaves.
 */
int _lf_get_downstream_of(int enclave_id, int** result);

/**
 * @brief Retrive the delays on the connections to direct upstream enclaves.
 * This updates the result pointer to point to a statically allocated array of delays.
 * The implementation is code-generated.
 * 
 * @param enclave_id The enclave for which to search for upstream delays.
 * @param result The pointer to dereference and update to point to the resulting array.
 * @return int The number of direct upstream enclaves.
 */
int _lf_get_upstream_delay_of(int enclave_id, interval_t** result);

/**
 * Function (to be code generated) to terminate execution.
 * This will be invoked after all shutdown actions have completed.
 * @param env The environment in which we are executing
 */
void terminate_execution(environment_t* env);

/**
 * Schedule the specified action with an integer value at a later logical
 * time that depends on whether the action is logical or physical and
 * what its parameter values are. This wraps a copy of the integer value
 * in a token. See schedule_token() for more details.
 * @param action The action to be triggered.
 * @param extra_delay Extra offset of the event release above that in the action.
 * @param value The value to send.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t _lf_schedule_int(lf_action_base_t* action, interval_t extra_delay, int value);

/**
 * Create a dummy event to be used as a spacer in the event queue.
 */
event_t* _lf_create_dummy_event(trigger_t* trigger, instant_t time, event_t* next, unsigned int offset);

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
trigger_handle_t _lf_schedule_token(lf_action_base_t* action, interval_t extra_delay, lf_token_t* token);

/**
 * Variant of schedule_token that creates a token to carry the specified value.
 * The value is required to be malloc'd memory with a size equal to the
 * element_size of the specifies action times the length parameter.
 * See _lf_schedule_token() for details.
 * @param action The action to be triggered.
 * @param extra_delay Extra offset of the event release above that in the action.
 * @param value Dynamically allocated memory containing the value to send.
 * @param length The length of the array, if it is an array, or 1 for a
 *  scalar and 0 for no payload.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t _lf_schedule_value(lf_action_base_t* action, interval_t extra_delay, void* value, size_t length);

/**
 * Schedule an action to occur with the specified value and time offset
 * with a copy of the specified value. If the value is non-null,
 * then it will be copied into newly allocated memory under the assumption
 * that its size is given in the trigger's token object's element_size field
 * multiplied by the specified length.
 * See _lf_schedule_token(), which this uses, for details.
 * @param action Pointer to an action on a self struct.
 * @param offset The time offset over and above that in the action.
 * @param value A pointer to the value to copy.
 * @param length The length, if an array, 1 if a scalar, and 0 if value is NULL.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
trigger_handle_t _lf_schedule_copy(lf_action_base_t* action, interval_t offset, void* value, size_t length);

/// \cond INTERNAL  // Doxygen conditional.

/**
 * @brief Create and initialize the required number of environments for the program.
 * @note Will be code generated by the compiler
 */
void _lf_create_environments(void);

/**
 * @brief Generated function that optionally sets default command-line options.
 */
void _lf_set_default_command_line_options(void);

/// \endcond // INTERNAL

#endif /* REACTOR_H */
/** @} */
