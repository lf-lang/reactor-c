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
 * This header file defines the functions and macros that programmers use
 * in the body of reactions for reading and writing inputs and outputs and
 * scheduling future events. The LF compiler does not parse that C code.
 * This fact strongly affects the design.
 *
 * The intent of the C target for Lingua Franca not to provide a safe
 * programming environment (The C++ and TypeScript targets are better
 * choices for that), but rather to find the lowest possible overhead
 * implementation of Lingua Franca. The API herein can easily be misused,
 * leading to memory leaks, nondeterminism, or program crashes.
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

//////////////////////  Constants  //////////////////////

/**
 * @brief Macro giving the minimum amount of time to sleep to wait for physical time to reach a logical time.
 * 
 * Unless the "fast" option is given, an LF program will wait until
 * physical time matches logical time before handling an event with
 * a given logical time. The amount of time is less than this given
 * threshold, then no wait will occur. The purpose of this is
 * to prevent unnecessary delays caused by simply setting up and
 * performing the wait.
 */
#define MIN_SLEEP_DURATION USEC(10)

/// \cond INTERNAL  // Doxygen conditional.

/**
 * @brief Mark the given port's is_present field as true.
 * @param port A pointer to the port struct as an `lf_port_base_t*`.
 */
void lf_set_present(lf_port_base_t* port);

/**
 * @brief Forward declaration for the executable preamble;
 * @param env Environment in which to execute to preamble
 * 
 */
void _lf_executable_preamble(environment_t* env);

/// \endcond // INTERNAL

//////////////////////  Macros for reading and writing ports  //////////////////////
// NOTE: Ports passed to these macros can be cast to:
// lf_port_base_t: which has the field bool is_present (and more);
// token_template_t: which has a lf_token_t* token field; or
// token_type_t: Which has element_size, destructor, and copy_constructor fields.

/**
 * Macro for extracting the deadline from the index of a reaction.
 * The reaction queue is sorted according to this index, and the
 * use of the deadline here results in an earliest deadline first
 * (EDF) scheduling poicy.
 */
#define DEADLINE(index) (index & 0x7FFFFFFFFFFF0000)

/**
 * Macro for determining whether two reactions are in the
 * same chain (one depends on the other). This is conservative.
 * If it returns false, then they are surely not in the same chain,
 * but if it returns true, they may be in the same chain.
 * This is in reactor_threaded.c to execute reactions in parallel
 * on multiple cores even if their levels are different.
 */
#define OVERLAPPING(chain1, chain2) ((chain1 & chain2) != 0)

//  ======== Function Declarations ========  //

/**
 * Return the global STP offset on advancement of logical
 * time for federated execution.
 */
interval_t lf_get_stp_offset(void);

/**
 * Set the global STP offset on advancement of logical
 * time for federated execution.
 *
 * @param offset A positive time value to be applied
 *  as the STP offset.
 */
void lf_set_stp_offset(interval_t offset);

/**
 * Print a snapshot of the priority queues used during execution
 * (for debugging).
 * @param env The environment in which we are executing.
 */
void lf_print_snapshot(environment_t* env);

/**
 * Request a stop to execution as soon as possible.
 * In a non-federated execution with only a single enclave, this will occur
 * one microstep later than the current tag. In a federated execution or when
 * there is more than one enclave, it will likely occur at a later tag determined
 * by the RTI so that all federates and enclaves stop at the same tag.
 */
void lf_request_stop(void);

/**
 * Allocate zeroed-out memory and record the allocated memory on
 * the specified list so that it will be freed when calling
 * {@link _lf_free(allocation_record_t**)}.
 * @param count The number of items of size 'size' to accomodate.
 * @param size The size of each item.
 * @param head Pointer to the head of a list on which to record
 *  the allocation, or NULL to not record it.
 */
void* _lf_allocate(
		size_t count, size_t size, struct allocation_record_t** head);

/**
 * Free memory allocated using
 * {@link _lf_allocate(size_t, size_t, allocation_record_t**)}
 * and mark the list empty by setting `*head` to NULL.
 * @param head Pointer to the head of a list on which to record
 *  the allocation, or NULL to not record it.
 */
void _lf_free(struct allocation_record_t** head);

/**
 * Allocate memory for a new runtime instance of a reactor.
 * This records the reactor on the list of reactors to be freed at
 * termination of the program. If you plan to free the reactor before
 * termination of the program, use
 * {@link _lf_allocate(size_t, size_t, allocation_record_t**)}
 * with a null last argument instead.
 * @param size The size of the self struct, obtained with sizeof().
 */
void* _lf_new_reactor(size_t size);

/**
 * Free all the reactors that are allocated with
 * {@link #_lf_new_reactor(size_t)}.
 */
void _lf_free_all_reactors(void);

/**
 * Free memory recorded on the allocations list of the specified reactor.
 * @param self The self struct of the reactor.
 */
void _lf_free_reactor(self_base_t *self);

/**
 * Generated function that optionally sets default command-line options.
 */
void _lf_set_default_command_line_options(void);

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

void termination();

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

/**
 * @brief Will create and initialize the required number of environments for the program
 * @note Will be code generated by the compiler
 */
void _lf_create_environments();


/**
 * These functions must be implemented by both threaded and single-threaded
 * runtime. Should be routed to appropriate API calls in platform.h
*/

#endif /* REACTOR_H */
/** @} */
