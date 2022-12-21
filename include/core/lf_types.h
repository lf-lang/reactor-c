/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Marten Lohstroh (marten@berkeley.edu)
 * @author Chris Gill (cdgill@wustl.edu)
 * @author Mehrdad Niknami (mniknami@berkeley.edu)
 *
 * @section LICENSE
 * Copyright (c) 2019, The University of California at Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 *
 * Type definitions that are widely used across different parts of the runtime.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#include "modes.h" // Modal model support
#include "pqueue.h"
#include "platform.h"
#include "tag.h"

/**
 * ushort type. Redefine here for portability if sys/types.h is not included.
 * @see sys/types.h
 *
 * @note using sizeof(ushort) should be okay but not sizeof ushort.
 */
#ifndef _SYS_TYPES_H
typedef unsigned short int ushort;
#endif

/**
 * Policy for handling scheduled events that violate the specified
 * minimum interarrival time.
 * The default policy is `defer`: adjust the tag to that the minimum
 * interarrival time is satisfied.
 * The `drop` policy simply drops events that are scheduled too early.
 * The `replace` policy will attempt to replace the value of the event
 * that it preceded it. Unless the preceding event has already been
 * handled, its gets assigned the value of the new event. If the
 * preceding event has already been popped off the event queue, the
 * `defer` policy is fallen back to.
 */
typedef enum {defer, drop, replace} lf_spacing_policy_t;

/**
 * An enum that enables the C core library to
 * ignore freeing the void* inside a token if the void*
 * value is garbage collected by an external controller
 */
typedef enum {no=0, token_and_value, token_only} ok_to_free_t;

/**
 * Status of a given port at a given logical time.
 *
 * If the value is 'present', it is an indicator that the port is present at the given logical time.
 * If the value is 'absent', it is an indicator that the port is absent at the given logical time.
 * If the value is 'unknown', it is unknown whether the port is present or absent (e.g., in a distributed application).
 *
 * @note For non-network ports, unknown is unused.
 * @note The absent and present fields need to be compatible with false and true
 *  respectively because for non-network ports, the status can either be present
 *  or absent (no possibility of unknown).
 */
typedef enum {absent = false, present = true, unknown} port_status_t;

/**
 * Status of a given reaction at a given logical time.
 *
 * If a reaction is 'inactive', it is neither running nor queued.
 * If a reaction is 'queued', it is going to be executed at the current logical time,
 * but it has not started running yet.
 * If a reaction is 'running', its body is being executed.
 *
 * @note inactive must equal zero because it should be possible to allocate a reaction
 *  with default values using calloc.
 * FIXME: The running state does not seem to be read.
 */
typedef enum {inactive = 0, queued, running} reaction_status_t;

/**
 * Handles for scheduled triggers. These handles are returned
 * by lf_schedule() functions. The intent is that the handle can be
 * used to cancel a future scheduled event, but this is not
 * implemented yet.
 */
typedef int trigger_handle_t;

/**
 * String type so that we don't have to use {= char* =}.
 * Use this for strings that are not dynamically allocated.
 * For dynamically allocated strings that have to be freed after
 * being consumed downstream, use type char*.
 */
#ifndef string
typedef char* string;
#else
#warning "string typedef has been previously given."
#endif

/** Topological order index for reactions. */
typedef pqueue_pri_t index_t;

/**
 * Reaction function type. The argument passed to one of
 * these reaction functions is a pointer to the self struct
 * for the reactor.
 */
typedef void(*reaction_function_t)(void*);

/** Trigger struct representing an output, timer, action, or input. See below. */
typedef struct trigger_t trigger_t;

/**
 * Token type for dynamically allocated arrays and structs sent as messages.
 *
 * In the C LF target, a type for an output that ends in '*' is
 * treated specially. The value carried by the output is assumed
 * to be in dynamically allocated memory, and, using reference
 * counting, after the last downstream reader of the value has
 * finished, the memory will be freed.  To prevent this freeing
 * from occurring, the output type can be specified using the
 * syntax {= type* =}; this will not be treated as dynamically
 * allocated memory. Alternatively, the programmer can give a typedef
 * in the preamble that masks the trailing *.
 *
 * This struct is the wrapper around the dynamically allocated memory
 * that carries the message.  The message can be an array of values,
 * where the size of each value is element_size (in bytes). If it is
 * not an array, the length == 1.
 */
typedef struct lf_token_t {
    /** Pointer to dynamically allocated memory containing a message. */
    void* value;
    /** Size of the struct or array element. */
    size_t element_size;
    /** Length of the array or 1 for a struct. */
    size_t length;
    /** The number of input ports that have not already reacted to the message. */
    int ref_count;
    /** The destructor or NULL to use the default free(). */
    void (*destructor) (void* value);
    /** The copy constructor or NULL to use memcpy. */
    void* (*copy_constructor) (void* value);
    /**
     * Indicator of whether this token is expected to be freed.
     * Tokens that are created at the start of execution and associated with output
     * ports or actions are not expected to be freed. They can be reused instead.
     */
    ok_to_free_t ok_to_free;
    /** For recycling, a pointer to the next token in the recycling bin. */
    struct lf_token_t* next_free;
} lf_token_t;

/** A struct with a pointer to a lf_token_t and an _is_present variable
 *  for use to initialize actions in start_time_step().
 */
typedef struct token_present_t {
    lf_token_t** token;
    port_status_t* status; // FIXME: This structure is used to present the status of tokens
                           // for both ports and actions.
    bool reset_is_present; // True to set is_present to false after calling _lf_done_using().
} token_present_t;

/**
 * Reaction activation record to push onto the reaction queue.
 * Some of the information in this struct is common among all instances
 * of the reactor, and some is specific to each particular instance.
 * These are marked below COMMON or INSTANCE accordingly.
 * The COMMON information is set in the constructor.
 * The fields marked RUNTIME have values that change
 * during execution.
 * Instances of this struct are put onto the reaction queue by the scheduler.
 */
typedef struct reaction_t reaction_t;
struct reaction_t {
    reaction_function_t function; // The reaction function. COMMON.
    void* self;    // Pointer to a struct with the reactor's state. INSTANCE.
    int number;    // The number of the reaction in the reactor (0 is the first reaction).
    index_t index; // Inverse priority determined by dependency analysis. INSTANCE.
    // Binary encoding of the branches that this reaction has upstream in the dependency graph. INSTANCE.
    unsigned long long chain_id;
    size_t pos;       // Current position in the priority queue. RUNTIME.
    reaction_t* last_enabling_reaction; // The last enabling reaction, or NULL if there is none. Used for optimization. INSTANCE.
    size_t num_outputs;  // Number of outputs that may possibly be produced by this function. COMMON.
    bool** output_produced;   // Array of pointers to booleans indicating whether outputs were produced. COMMON.
    int* triggered_sizes;     // Pointer to array of ints with number of triggers per output. INSTANCE.
    trigger_t ***triggers;    // Array of pointers to arrays of pointers to triggers triggered by each output. INSTANCE.
    reaction_status_t status; // Indicator of whether the reaction is inactive, queued, or running. RUNTIME.
    interval_t deadline;      // Deadline relative to the time stamp for invocation of the reaction. INSTANCE.
    bool is_STP_violated;     // Indicator of STP violation in one of the input triggers to this reaction. default = false.
                              // Value of True indicates to the runtime that this reaction contains trigger(s)
                              // that are triggered at a later logical time that was originally anticipated.
                              // Currently, this is only possible if logical
                              // connections are used in a decentralized federated
                              // execution. COMMON.
    reaction_function_t deadline_violation_handler; // Deadline violation handler. COMMON.
    reaction_function_t STP_handler;   // STP handler. Invoked when a trigger to this reaction
                                       // was triggered at a later logical time than originally
                                       // intended. Currently, this is only possible if logical
                                       // connections are used in a decentralized federated
                                       // execution. COMMON.
    bool is_a_control_reaction; // Indicates whether this reaction is a control reaction. Control
                                // reactions will not set ports or actions and don't require scheduling
                                // any output reactions. Default is false.
    size_t worker_affinity;     // The worker number of the thread that scheduled this reaction. Used
                                // as a suggestion to the scheduler.
    const char* name;                 // If logging is set to LOG or higher, then this will
                                // point to the full name of the reactor followed by
                                // the reaction number.
    reactor_mode_t* mode;       // The enclosing mode of this reaction (if exists).
                                // If enclosed in multiple, this will point to the innermost mode.
};

/** Typedef for event_t struct, used for storing activation records. */
typedef struct event_t event_t;

/** Event activation record to push onto the event queue. */
struct event_t {
    instant_t time;           // Time of release.
    trigger_t* trigger;       // Associated trigger, NULL if this is a dummy event.
    size_t pos;               // Position in the priority queue.
    lf_token_t* token;        // Pointer to the token wrapping the value.
    bool is_dummy;            // Flag to indicate whether this event is merely a placeholder or an actual event.
#ifdef FEDERATED
    tag_t intended_tag;       // The intended tag.
#endif
    event_t* next;            // Pointer to the next event lined up in superdense time.
};

/**
 * Trigger struct representing an output, timer, action, or input.
 * Instances of this struct are put onto the event queue (event_q).
 */
struct trigger_t {
    reaction_t** reactions;   // Array of pointers to reactions sensitive to this trigger.
    int number_of_reactions;  // Number of reactions sensitive to this trigger.
    bool is_timer;            // True if this is a timer (a special kind of action), false otherwise.
    interval_t offset;        // Minimum delay of an action. For a timer, this is also the maximum delay.
    interval_t period;        // Minimum interarrival time of an action. For a timer, this is also the maximal interarrival time.
    lf_token_t* token;           // Pointer to a token wrapping the payload (or NULL if there is none).
    bool is_physical;         // Indicator that this denotes a physical action.
    event_t* last;            // Pointer to the last event that was scheduled for this action.
    lf_spacing_policy_t policy;          // Indicates which policy to use when an event is scheduled too early.
    size_t element_size;      // The size of the payload, if there is one, zero otherwise.
                              // If the payload is an array, then this is the size of an element of the array.
    port_status_t status;     // Determines the status of the port at the current logical time. Therefore, this
                              // value needs to be reset at the beginning of each logical time.
                              //
                              // This status is especially needed for the distributed execution because the receiver logic will need
                              // to know what it should do if it receives a message with 'intended tag = current tag' from another
                              // federate.
                              // - If status is 'unknown', it means that the federate has still no idea what the status of
                              //   this port is and thus has refrained from executing any reaction that has that port as its input.
                              //   This means that the receiver logic can directly inject the triggered reactions into the reaction
                              //   queue at the current logical time.
                              // - If the status is absent, it means that the federate has assumed that the port is 'absent'
                              //   for the current logical time. Therefore, receiving a message with 'intended tag = current tag'
                              //   is an error that should be handled, for example, as a violation of the STP offset in the decentralized
                              //   coordination.
                              // - Finally, if status is 'present', then this is an error since multiple
                              //   downstream messages have been produced for the same port for the same logical time.
    reactor_mode_t* mode;     // The enclosing mode of this reaction (if exists).
                              // If enclosed in multiple, this will point to the innermost mode.
#ifdef FEDERATED
    tag_t last_known_status_tag;        // Last known status of the port, either via a timed message, a port absent, or a
                                        // TAG from the RTI.
    bool is_a_control_reaction_waiting; // Indicates whether at least one control reaction is waiting for this trigger
                                        // if it belongs to a network input port. Must be false by default.
    tag_t intended_tag;                 // The amount of discrepency in logical time between the original intended
                                        // trigger time of this trigger and the actual trigger time. This currently
                                        // can only happen when logical connections are used using a decentralized coordination
                                        // mechanism (@see https://github.com/icyphy/lingua-franca/wiki/Logical-Connections).
    instant_t physical_time_of_arrival; // The physical time at which the message has been received on the network according to the local clock.
                                        // Note: The physical_time_of_arrival is only passed down one level of the hierarchy. Default: NEVER.
#endif
};

/**
 * An allocation record that is used by a destructor for a reactor
 * to free memory that has been dynamically allocated for the particular
 * instance of the reactor.  This will be an element of linked list.
 */
typedef struct allocation_record_t {
	void* allocated;
	struct allocation_record_t *next;
} allocation_record_t;

/**
 * The first element of every self struct defined in generated code
 * will be a pointer to an allocation record, which is either NULL
 * or the head of a NULL-terminated linked list of allocation records.
 * Casting the self struct to this type enables access to this list
 * by the function {@link _lf_free_reactor(self_base_t*)}. To allocate memory
 * for the reactor that will be freed by that function, allocate the
 * memory using {@link _lf_allocate(size_t,size_t,self_base_t*)}.
 */
typedef struct self_base_t {
	struct allocation_record_t *allocations;
	struct reaction_t *executing_reaction;   // The currently executing reaction of the reactor.
#ifdef MODAL_REACTORS
    reactor_mode_state_t _lf__mode_state;    // The current mode (for modal models).
#endif
} self_base_t;

#endif
