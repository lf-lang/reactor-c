/**
 * @file lf_types.h
 * @brief Type definitions that are widely used across different parts of the runtime.
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
 * @ingroup IntTypes
 */
#define SCHED_ADAPTIVE 1

/**
 * @brief Experimental GEDF-NP scheduler.
 * @ingroup IntTypes
 */
#define SCHED_GEDF_NP 2

/**
 * @brief Default non-preemptive scheduler.
 * @ingroup IntTypes
 */
#define SCHED_NP 3

/**
 * @brief A struct representing a barrier in threaded LF programs.
 * @ingroup IntTypes
 *
 * This will prevent advancement of the current tag if
 * the number of requestors is larger than 0 or the value of horizon is not (FOREVER, 0).
 */
typedef struct lf_tag_advancement_barrier_t {
  int requestors; // Used to indicate the number of
                  // requestors that have asked
                  // for a barrier to be raised
                  // on tag.
  tag_t horizon;  // If semaphore is larger than 0
                  // then the runtime should not
                  // advance its tag beyond the
                  // horizon.
} lf_tag_advancement_barrier_t;

/**
 * @brief Policy for handling scheduled events that violate the specified
 * minimum interarrival time.
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
 * @ingroup Types
 *
 * Use this as the type for ports sending and receiving strings that are not dynamically allocated.
 * For dynamically allocated strings that have to be freed after
 * being consumed downstream, use type `char*`.
 */
typedef char* string;
#else
#warning "string typedef has been previously given."
#endif

/** Topological order index for reactions. */
typedef pqueue_pri_t index_t;

/**
 * @brief Reaction function type.
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
  reaction_function_t function; // The reaction function. COMMON.
  void* self;                   // Pointer to a struct with the reactor's state. INSTANCE.
  int number;                   // The number of the reaction in the reactor (0 is the first reaction).
  index_t index;                // Inverse priority determined by dependency analysis. INSTANCE.
  size_t pos;                   // Current position in the priority queue. RUNTIME.
  reaction_t*
      last_enabling_reaction; // The last enabling reaction, or NULL if there is none. Used for optimization. INSTANCE.
  size_t num_outputs;         // Number of outputs that may possibly be produced by this function. COMMON.
  bool** output_produced;     // Array of pointers to booleans indicating whether outputs were produced. COMMON.
  int* triggered_sizes;       // Pointer to array of ints with number of triggers per output. INSTANCE.
  trigger_t*** triggers;      // Array of pointers to arrays of pointers to triggers triggered by each output. INSTANCE.
  reaction_status_t status;   // Indicator of whether the reaction is inactive, queued, or running. RUNTIME.
  interval_t deadline;        // Deadline relative to the time stamp for invocation of the reaction. INSTANCE.
  bool is_STP_violated; // Indicator of STP violation in one of the input triggers to this reaction. default = false.
                        // Value of True indicates to the runtime that this reaction contains trigger(s)
                        // that are triggered at a later logical time that was originally anticipated.
                        // Currently, this is only possible if logical
                        // connections are used in a decentralized federated
                        // execution. COMMON.
  reaction_function_t deadline_violation_handler; // Deadline violation handler. COMMON.
  reaction_function_t STP_handler;                // STP handler. Invoked when a trigger to this reaction
                                                  // was triggered at a later logical time than originally
                                                  // intended. Currently, this is only possible if logical
                                                  // connections are used in a decentralized federated
                                                  // execution. COMMON.
  bool is_an_input_reaction; // Indicates whether this reaction is a network input reaction of a federate. Default is
                             // false.
  size_t worker_affinity;    // The worker number of the thread that scheduled this reaction. Used
                             // as a suggestion to the scheduler.
  const char* name;          // If logging is set to LOG or higher, then this will
                             // point to the full name of the reactor followed by
                             // the reaction number.
  reactor_mode_t* mode;      // The enclosing mode of this reaction (if exists).
                             // If enclosed in multiple, this will point to the innermost mode.
};

/** Typedef for event_t struct, used for storing activation records. */
typedef struct event_t event_t;

/**
 * @brief Event activation record to push onto the event queue.
 * @ingroup IntTypes
 */
struct event_t {
  pqueue_tag_element_t base; // Elements of pqueue_tag. It contains tag of release and position in the priority queue.
  trigger_t* trigger;        // Associated trigger, NULL if this is a dummy event.
  lf_token_t* token;         // Pointer to the token wrapping the value.
#ifdef FEDERATED
  tag_t intended_tag; // The intended tag.
#endif
};

/**
 * @brief Trigger struct representing an output, timer, action, or input.
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
 * @ingroup IntTypes
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
