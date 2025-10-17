/**
 * @file lf_types.h
 * @brief Type definitions that are widely used across different parts of the runtime.
 * @ingroup Internal
 *
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @author Chris Gill
 * @author Mehrdad Niknami
 *
 * <b>IMPORTANT:</b> Many of the structs defined here require matching layouts
 * and, if changed, will require changes in the code generator.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#include "modal_models/modes.h" // Modal model support
#include "utils/pqueue.h"
#include "utils/pqueue_tag.h"
#include "lf_token.h"
#include "tag.h"
#include "vector.h"

#ifndef _SYS_TYPES_H
/**
 * @brief Unsigned short type. Redefine here for portability if sys/types.h is not included.
 * @ingroup Internal
 *
 * @see sys/types.h
 *
 * @note using sizeof(ushort) should be okay but not sizeof ushort.
 */
typedef unsigned short int ushort;
#endif

/*
 * Define scheduler types as integers. This way we can conditionally
 * include/exclude code with the preprocessor with
 * #if SCHEDULER == SCHED_ADAPTIVE etc
 * This means that `lf_types.h` MUST be included before doing any preprocessing
 * on SCHEDULER compile def.
 */

/**
 * @brief Experimental adaptive scheduler.
 * @ingroup Internal
 */
#define SCHED_ADAPTIVE 1

/**
 * @brief Experimental GEDF-NP scheduler.
 * @ingroup Internal
 */
#define SCHED_GEDF_NP 2

/**
 * @brief Default non-preemptive scheduler.
 * @ingroup Internal
 */
#define SCHED_NP 3

/**
 * @brief A struct representing a barrier in threaded LF programs.
 * @ingroup Internal
 *
 * This will prevent advancement of the current tag if
 * the number of requestors is larger than 0 or the value of horizon is not (FOREVER, 0).
 */
typedef struct lf_tag_advancement_barrier_t {
  /**
   * @brief Number of requestors waiting at the barrier.
   * Used to track how many threads have requested the barrier to be raised.
   * The barrier will prevent tag advancement if this value is greater than 0.
   */
  int requestors;

  /**
   * @brief Tag horizon for barrier advancement.
   * If this value is not (FOREVER, 0), the runtime will not advance
   * its tag beyond this horizon.
   * Used to coordinate tag advancement across multiple threads.
   */
  tag_t horizon;
} lf_tag_advancement_barrier_t;

/**
 * @brief Policy for handling scheduled events that violate the specified
 * minimum interarrival time.
 * @ingroup Internal
 *
 * The default policy is `defer`: adjust the tag to that the minimum
 * interarrival time is satisfied.
 * The `drop` policy simply drops events that are scheduled too early.
 * The `replace` policy will attempt to replace the payload of
 * the preceding event. Unless the preceding event has already been
 * handled, it gets assigned the value of the new event. If the
 * preceding event has already been popped off the event queue, the
 * `defer` policy is fallen back to.
 * The `update` policy drops the preceding event, if it is still in the event queue, and updates it with
 * the newly scheduled event.
 */
typedef enum { defer, drop, replace, update } lf_spacing_policy_t;

/**
 * @brief Status of a given port at a given logical time.
 * @ingroup Internal
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
typedef enum { absent = false, present = true, unknown } port_status_t;

/**
 * @brief Status of a given reaction at a given logical time.
 * @ingroup Internal
 *
 * If a reaction is 'inactive', it is neither running nor queued.
 * If a reaction is 'queued', it is going to be executed at the current logical time,
 * but it has not started running yet.
 * If a reaction is 'running', its body is being executed.
 *
 * @note inactive must equal zero because it should be possible to allocate a reaction
 *  with default values using calloc.
 * @note The running state does not seem to be read.
 */
typedef enum { inactive = 0, queued, running } reaction_status_t;

/**
 * @brief Handles for scheduled triggers.
 * @ingroup Internal
 *
 * These handles are returned
 * by lf_schedule() functions. The intent is that the handle can be
 * used to cancel a future scheduled event, but this is not
 * implemented yet.
 */
typedef int trigger_handle_t;

#ifndef string
/**
 * @brief String type so that we don't have to use {= char* =}.
 * @ingroup API
 *
 * Use this as the type for ports sending and receiving strings that are not dynamically allocated.
 * For dynamically allocated strings that have to be freed after
 * being consumed downstream, use type `char*`.
 */
typedef char* string;
#else
#warning "string typedef has been previously given."
#endif

/**
 * @brief Topological order index for reactions.
 * @ingroup Internal
 */
typedef pqueue_pri_t index_t;

/**
 * @brief Reaction function type.
 * @ingroup Internal
 *
 * The argument passed to one of
 * these reaction functions is a pointer to the self struct
 * for the reactor.
 */
typedef void (*reaction_function_t)(void*);

/* Trigger struct representing an output, timer, action, or input. */
typedef struct trigger_t trigger_t;

typedef struct reaction_t reaction_t;

/**
 * @brief Reaction activation record to push onto the reaction queue.
 * @ingroup Internal
 *
 * Some of the information in this struct is common among all instances
 * of the reactor, and some is specific to each particular instance.
 * These are marked below COMMON or INSTANCE accordingly.
 * The COMMON information is set in the constructor.
 * The fields marked RUNTIME have values that change
 * during execution.
 * Instances of this struct are put onto the reaction queue by the scheduler.
 */
struct reaction_t {
  /**
   * @brief The reaction function to be executed.
   * COMMON: Set during reactor construction.
   * This is the actual function that implements the reaction's behavior.
   */
  reaction_function_t function;

  /**
   * @brief Pointer to the reactor's state struct.
   * INSTANCE: Specific to each reactor instance.
   * Contains the state variables and other instance-specific data.
   */
  void* self;

  /**
   * @brief The reaction number within its reactor.
   * INSTANCE: Specific to each reactor instance.
   * Zero-based index indicating the reaction's position in the reactor.
   */
  int number;

  /**
   * @brief Inverse priority determined by dependency analysis.
   * INSTANCE: Specific to each reactor instance.
   * Used by the scheduler to determine execution order.
   */
  index_t index;

  /**
   * @brief Current position in the priority queue.
   * RUNTIME: Changes during execution.
   * Used by the scheduler to track the reaction's position in the queue.
   */
  size_t pos;

  /**
   * @brief Pointer to the last enabling reaction.
   * INSTANCE: Specific to each reactor instance.
   * Points to the last reaction that enables this one, or NULL if none.
   * Used for optimization purposes.
   */
  reaction_t* last_enabling_reaction;

  /**
   * @brief Number of outputs that may be produced by this reaction.
   * COMMON: Set during reactor construction.
   * Defines the size of the output_produced array.
   */
  size_t num_outputs;

  /**
   * @brief Array of pointers to booleans indicating output production.
   * COMMON: Set during reactor construction.
   * Each element indicates whether the corresponding output was produced.
   */
  bool** output_produced;

  /**
   * @brief Array of trigger counts per output.
   * INSTANCE: Specific to each reactor instance.
   * Contains the number of triggers for each output port.
   */
  int* triggered_sizes;

  /**
   * @brief Array of trigger arrays for each output.
   * INSTANCE: Specific to each reactor instance.
   * Three-dimensional array: [output][trigger_count][trigger]
   * Contains pointers to all triggers that are triggered by each output.
   */
  trigger_t*** triggers;

  /**
   * @brief Current status of the reaction.
   * RUNTIME: Changes during execution.
   * Indicates whether the reaction is inactive, queued, or running.
   * @see reaction_status_t for possible values.
   */
  reaction_status_t status;

  /**
   * @brief Deadline relative to the reaction's invocation time.
   * INSTANCE: Specific to each reactor instance.
   * Used to detect deadline violations.
   */
  interval_t deadline;

  /**
   * @brief Flag indicating STP violation in input triggers.
   * COMMON: Set during reactor construction.
   * True if any trigger to this reaction was triggered at a later logical time
   * than originally anticipated. Currently only possible with logical
   * connections in decentralized federated execution.
   */
  bool is_STP_violated;

  /**
   * @brief Function to handle deadline violations.
   * COMMON: Set during reactor construction.
   * Called when the reaction's deadline is violated.
   */
  reaction_function_t deadline_violation_handler;

  /**
   * @brief Function to handle STP violations.
   * COMMON: Set during reactor construction.
   * Called when a trigger to this reaction was triggered at a later logical time
   * than originally intended. Currently only possible with logical
   * connections in decentralized federated execution.
   */
  reaction_function_t STP_handler;

  /**
   * @brief Flag indicating if this is a network input reaction.
   * COMMON: Set during reactor construction.
   * True if this reaction is a network input reaction of a federate.
   * Default is false.
   */
  bool is_an_input_reaction;

  /**
   * @brief Worker thread affinity suggestion.
   * RUNTIME: Changes during execution.
   * The worker number of the thread that scheduled this reaction.
   * Used as a suggestion to the scheduler for thread assignment.
   */
  size_t worker_affinity;

  /**
   * @brief Full name of the reaction for logging purposes.
   * COMMON: Set during reactor construction.
   * Points to the full name of the reactor followed by the reaction number.
   * Only used when logging level is set to LOG or higher.
   */
  const char* name;

  /**
   * @brief Pointer to the enclosing mode of this reaction.
   * COMMON: Set during reactor construction.
   * If the reaction is enclosed in multiple modes, this points to the innermost mode.
   * Only present when modal reactors are enabled.
   */
  reactor_mode_t* mode;
};

/**
 * @brief Event activation record for storing event queue entries.
 * @ingroup Internal
 *
 * This type is used for storing activation records in the event queue.
 * Each event represents a scheduled trigger with its associated data.
 */
typedef struct event_t event_t;

/**
 * @brief Event activation record to push onto the event queue.
 * @ingroup Internal
 */
struct event_t {
  /**
   * @brief Base priority queue element containing tag and position.
   * Contains the tag of release and position in the priority queue.
   * Used by the priority queue implementation for ordering and management.
   */
  pqueue_tag_element_t base;

  /**
   * @brief Pointer to the associated trigger.
   * Points to the trigger that generated this event.
   * NULL if this is a dummy event.
   */
  trigger_t* trigger;

  /**
   * @brief Pointer to the token wrapping the event's value.
   * Contains the actual data associated with this event.
   * The token provides type information and value storage.
   */
  lf_token_t* token;

#ifdef FEDERATED
  /**
   * @brief The intended tag for this event.
   * Used in federated execution to track the original intended
   * logical time of the event.
   * @note Only present when federated execution is enabled.
   */
  tag_t intended_tag;
#endif
};

/**
 * @brief Trigger struct representing an output, timer, action, or input.
 * @ingroup Internal
 */
struct trigger_t {
  /**
   * @brief Type and token information template.
   * Contains type information and token handling details for the trigger.
   * Note: 'template' is a C++ keyword, hence the abbreviated name.
   */
  token_template_t tmplt;

  /**
   * @brief Array of pointers to reactions that are sensitive to this trigger.
   * These reactions will be executed when the trigger fires.
   */
  reaction_t** reactions;

  /**
   * @brief Number of reactions that are sensitive to this trigger.
   * Indicates the size of the reactions array.
   */
  int number_of_reactions;

  /**
   * @brief Flag indicating whether this trigger is a timer.
   * True if this is a timer (a special kind of action), false otherwise.
   */
  bool is_timer;

  /**
   * @brief Minimum delay for an action trigger.
   * For timers, this also represents the maximum delay.
   */
  interval_t offset;

  /**
   * @brief Minimum interarrival time for an action trigger.
   * For timers, this also represents the maximum interarrival time.
   */
  interval_t period;

  /**
   * @brief Flag indicating whether this trigger represents a physical action.
   * True if this denotes a physical action, false otherwise.
   */
  bool is_physical;

  /**
   * @brief Tag of the last event scheduled for this action.
   * Only used for actions and will be NEVER otherwise.
   */
  tag_t last_tag;

  /**
   * @brief Policy for handling events scheduled too early.
   * Determines how to handle events that violate the minimum interarrival time.
   * @see lf_spacing_policy_t for available policies.
   */
  lf_spacing_policy_t policy;

  /**
   * @brief Current status of the port at the current logical time.
   * Needs to be reset at the beginning of each logical time.
   * Particularly important for distributed execution to handle message coordination.
   * @see port_status_t for possible values.
   */
  port_status_t status;

  /**
   * @brief Pointer to the enclosing mode of this trigger.
   * If the trigger is enclosed in multiple modes, this points to the innermost mode.
   * Only present when modal reactors are enabled.
   */
  reactor_mode_t* mode;

#ifdef FEDERATED
  /**
   * @brief Last known status tag of the port.
   * Records the last known status via timed message, port absent, or TAG from RTI.
   * Only present in federated execution.
   */
  tag_t last_known_status_tag;

  /**
   * @brief The intended trigger time of this trigger.
   * Represents the discrepancy between intended and actual trigger time.
   * Currently only relevant for logical connections using decentralized coordination.
   * @see https://github.com/icyphy/lingua-franca/wiki/Logical-Connections
   */
  tag_t intended_tag;

  /**
   * @brief Physical time at which the message was received on the network.
   * Based on the local clock. Note: This time is only passed down one level of hierarchy.
   * Default value is NEVER.
   * Only present in federated execution.
   */
  instant_t physical_time_of_arrival;
#endif
};

/**
 * @brief Allocation record to keep track of dynamically-allocated memory.
 * @ingroup Internal
 *
 * An allocation record that is used by a destructor for a reactor
 * to free memory that has been dynamically allocated for the particular
 * instance of the reactor.  This will be an element of linked list.
 * The `allocated` pointer points to the allocated memory, and the `next`
 * pointer points to the next allocation record (or NULL if there are no more).
 */
typedef struct allocation_record_t {
  void* allocated;
  struct allocation_record_t* next;
} allocation_record_t;

typedef struct environment_t environment_t;

/**
 * @brief The base type for all reactor self structs.
 * @ingroup Internal
 *
 * The first element of every self struct defined in generated code
 * will be a pointer to an allocation record, which is either NULL
 * or the head of a NULL-terminated linked list of allocation records.
 * This list is used to free memory that has been dynamically allocated.
 * This struct also provides a pointer to the currently executing reaction,
 * to the environment in which the reaction is executing, and to the mutex
 * that is used to protect the reactor.  If modal models are being used,
 * it also records the current mode.
 */
typedef struct self_base_t {
  /**
   * @brief Pointer to the head of a NULL-terminated linked list of allocation records.
   * Used to track and free dynamically allocated memory for this reactor instance.
   */
  struct allocation_record_t* allocations;

  /**
   * @brief Pointer to the currently executing reaction of the reactor.
   * This field is updated during reaction execution to track which reaction is running.
   */
  struct reaction_t* executing_reaction;

  /**
   * @brief Pointer to the environment in which the reactor is executing.
   * Contains runtime context and configuration for the reactor.
   */
  environment_t* environment;

  /**
   * @brief The name of the reactor instance.
   * For bank reactors, this will be appended with [index].
   */
  char* name;

  /**
   * @brief The full hierarchical name of the reactor.
   * This will be NULL unless lf_reactor_full_name() is called.
   */
  char* full_name;

  /**
   * @brief Pointer to the parent reactor of this reactor instance.
   * Used to maintain the reactor hierarchy.
   */
  self_base_t* parent;

#if !defined(LF_SINGLE_THREADED)
  /**
   * @brief Mutex used to protect the reactor from concurrent access.
   * If not null, this is expected to point to an lf_mutex_t.
   * Not declared as lf_mutex_t to avoid dependency on platform.h.
   */
  void* reactor_mutex;
#endif

#if defined(MODAL_REACTORS)
  /**
   * @brief The current mode state for modal models.
   * Only present when modal reactors are enabled.
   */
  reactor_mode_state_t _lf__mode_state;
#endif
} self_base_t;

/**
 * @brief Base type for actions.
 * @ingroup Internal
 *
 * Action structs are customized types because their payloads are type
 * specific. This struct represents their common features. Given any
 * pointer to an action struct, it can be cast to lf_action_base_t,
 * to token_template_t, or to token_type_t to access these common fields.
 */
typedef struct {
  /**
   * @brief Type and token information template.
   * Contains type information and token handling details for the action.
   * Note: 'template' is a C++ keyword, hence the abbreviated name.
   * This field must match the layout of token_template_t for proper casting.
   */
  token_template_t tmplt;

  /**
   * @brief Flag indicating whether the action has a value at the current logical time.
   * True if the action is present at the current logical time, false otherwise.
   */
  bool is_present;

  /**
   * @brief Pointer to the trigger associated with this action.
   * This field must match the layout of lf_action_internal_t for proper casting.
   * @see trigger_t for details about the trigger structure.
   */
  trigger_t* trigger;

  /**
   * @brief Pointer to the parent reactor's self struct.
   * Provides access to the reactor instance that owns this action.
   */
  self_base_t* parent;

  /**
   * @brief Flag indicating whether the action has a value.
   * True if the action has a value, false otherwise.
   * This is distinct from is_present as it indicates value availability rather than temporal presence.
   */
  bool has_value;

  /**
   * @brief Source identifier for federated network input actions.
   * Used to identify the source of network input actions in federated execution.
   * Only meaningful for federated network input actions.
   */
  int source_id;
} lf_action_base_t;

/**
 * Internal part of the action structs.
 */
typedef struct {
  trigger_t* trigger;
} lf_action_internal_t;

/**
 * @brief Internal part of the port structs.
 *
 * HAS TO MATCH lf_port_base_t after tmplt and is_present.
 */
typedef struct {
  lf_sparse_io_record_t* sparse_record; // NULL if there is no sparse record.
  int destination_channel;              // -1 if there is no destination.
  int num_destinations;                 // The number of destination reactors this port writes to.
  self_base_t* source_reactor;          // Pointer to the self struct of the reactor that provides data to this port.
                                        // If this is an input, that reactor will normally be the container of the
                                        // output port that sends it data.
} lf_port_internal_t;

#endif
