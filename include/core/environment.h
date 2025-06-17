/**
 * @file environment.h
 * @brief API for the environment data structure.
 * @ingroup Internal
 *
 * @author Erling R. Jellum
 *
 * This is an API for creating and destroying environments. An environment is the
 * "context" within which the reactors are executed. The environment contains data structures
 * which are shared among the reactors such as priority queues, the current logical tag,
 * the worker scheduler, and a lot of meta data. Each reactor stores a pointer to its
 * environment on its self-struct. If a LF program has multiple scheduling enclaves,
 * then each enclave will have its own environment.
 */
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "lf_types.h"
#include "low_level_platform.h"
#include "tracepoint.h"

// Forward declarations so that a pointers can appear in the environment struct.
typedef struct lf_scheduler_t lf_scheduler_t;
typedef struct mode_environment_t mode_environment_t;
typedef struct enclave_info_t enclave_info_t;
typedef struct watchdog_t watchdog_t;

/**
 * @brief The global environment.
 * @ingroup Internal
 *
 * Some operations are not specific to a particular scheduling enclave and therefore
 * have no associated environment. When invoking a function such as lf_critical_section_enter,
 * which requires an environment argument, it may be possible to pass this GLOBAL_ENVIRONMENT.
 * For @ref lf_critical_section_enter, for example, this may acquire a global mutex instead of
 * a mutex specific to a particular scheduling enclave. Most functions that take environment
 * arguments, however, cannot accept the GLOBAL_ENVIRONMENT argument, and passing it will
 * result in an assertion violation.
 */
#define GLOBAL_ENVIRONMENT NULL

/**
 * @brief Execution environment.
 * @ingroup Internal
 *
 * This struct contains information about the execution environment.
 * An execution environment maintains a notion of a "current tag"
 * and has its own event queue and scheduler.
 * Normally, there is only one execution environment, but if you use
 * scheduling enclaves, then there will be one for each enclave.
 */
typedef struct environment_t {
  /**
   * @brief Indicates whether the environment has been initialized.
   *
   * Set to true after environment_init() completes successfully.
   * Used to prevent multiple initializations and ensure proper setup.
   */
  bool initialized;

  /**
   * @brief Indicates whether execution has started.
   *
   * Set to true when events at the start tag have been pulled from
   * the event queue. Used to track the execution state of the environment.
   */
  bool execution_started;

  /**
   * @brief Name identifier for this environment.
   *
   * Used for debugging and tracing purposes. Should be unique
   * across all environments in the program.
   */
  char* name;

  /**
   * @brief Unique numeric identifier for this environment.
   *
   * Used to distinguish between different environments,
   * particularly in multi-enclave setups.
   */
  int id;

  /**
   * @brief The current logical tag being processed.
   *
   * Represents the current logical time and microstep being
   * executed in this environment.
   */
  tag_t current_tag;

  /**
   * @brief The tag at which execution should stop.
   *
   * When current_tag reaches this value, execution will
   * terminate after processing all events at this tag.
   */
  tag_t stop_tag;

  /**
   * @brief Priority queue for pending events.
   *
   * Stores events ordered by their logical tags. Events are
   * processed in tag order, with earlier tags taking precedence.
   */
  pqueue_tag_t* event_q;

  /**
   * @brief Priority queue for recycling events.
   *
   * Used to efficiently reuse event structures after they
   * have been processed, reducing memory allocation overhead.
   */
  pqueue_tag_t* recycle_q;

  /**
   * @brief Array of is_present fields for ports.
   *
   * Tracks whether each port has a value present at the current tag.
   * The size of this array is stored in is_present_fields_size.
   */
  bool** is_present_fields;

  /**
   * @brief Number of is_present fields allocated.
   *
   * Indicates the size of the is_present_fields array.
   */
  int is_present_fields_size;

  /**
   * @brief Abbreviated version of is_present fields.
   *
   * Used for optimization when tracking port presence.
   * The size of this array is stored in is_present_fields_abbreviated_size.
   */
  bool** is_present_fields_abbreviated;

  /**
   * @brief Number of abbreviated is_present fields allocated.
   *
   * Indicates the size of the is_present_fields_abbreviated array.
   */
  int is_present_fields_abbreviated_size;

  /**
   * @brief Vector storing sizes of sparse I/O records.
   *
   * Used to track the size of I/O records for each port
   * in sparse I/O mode.
   */
  vector_t sparse_io_record_sizes;

  /**
   * @brief Handle for the environment's trigger.
   *
   * Used to identify and manage the environment's trigger
   * in the runtime system.
   */
  trigger_handle_t _lf_handle;

  /**
   * @brief Array of timer triggers.
   *
   * Stores all timer triggers associated with this environment.
   * The size of this array is stored in timer_triggers_size.
   */
  trigger_t** timer_triggers;

  /**
   * @brief Number of timer triggers allocated.
   *
   * Indicates the size of the timer_triggers array.
   */
  int timer_triggers_size;

  /**
   * @brief Array of startup reactions.
   *
   * Contains all reactions that should be executed when
   * the environment starts. The size is stored in startup_reactions_size.
   */
  reaction_t** startup_reactions;

  /**
   * @brief Number of startup reactions allocated.
   *
   * Indicates the size of the startup_reactions array.
   */
  int startup_reactions_size;

  /**
   * @brief Array of shutdown reactions.
   *
   * Contains all reactions that should be executed when
   * the environment shuts down. The size is stored in shutdown_reactions_size.
   */
  reaction_t** shutdown_reactions;

  /**
   * @brief Number of shutdown reactions allocated.
   *
   * Indicates the size of the shutdown_reactions array.
   */
  int shutdown_reactions_size;

  /**
   * @brief Array of reset reactions.
   *
   * Contains all reactions that should be executed when
   * the environment resets. The size is stored in reset_reactions_size.
   */
  reaction_t** reset_reactions;

  /**
   * @brief Number of reset reactions allocated.
   *
   * Indicates the size of the reset_reactions array.
   */
  int reset_reactions_size;

  /**
   * @brief Mode environment for modal reactors.
   *
   * Contains state and configuration for modal reactors
   * in this environment.
   */
  mode_environment_t* modes;

  /**
   * @brief Number of watchdogs allocated.
   *
   * Indicates the size of the watchdogs array.
   */
  int watchdogs_size;

  /**
   * @brief Array of watchdog timers.
   *
   * Contains all watchdog timers associated with this
   * environment for monitoring execution progress.
   */
  watchdog_t** watchdogs;

  /**
   * @brief Number of worker threads in this environment.
   *
   * Indicates how many threads are available for
   * parallel execution of reactions.
   */
  int worker_thread_count;

#if defined(LF_SINGLE_THREADED)
  /**
   * @brief Priority queue for reactions in single-threaded mode.
   *
   * Used to schedule and execute reactions in order when
   * running in single-threaded mode.
   */
  pqueue_t* reaction_q;
#else
  /**
   * @brief Number of worker threads.
   *
   * Indicates the total number of worker threads available
   * for parallel execution in this environment.
   */
  int num_workers;

  /**
   * @brief Array of worker thread IDs.
   *
   * Stores the thread identifiers for all worker threads
   * in this environment.
   */
  lf_thread_t* thread_ids;

  /**
   * @brief Mutex for synchronizing access to the environment.
   *
   * Used to protect shared data structures and ensure
   * thread-safe operations.
   */
  lf_mutex_t mutex;

  /**
   * @brief Condition variable for event queue changes.
   *
   * Used to notify worker threads when the event queue
   * has been modified.
   */
  lf_cond_t event_q_changed;

  /**
   * @brief Scheduler for managing worker threads.
   *
   * Responsible for assigning reactions to worker threads
   * and managing their execution.
   */
  lf_scheduler_t* scheduler;

  /**
   * @brief Barrier for tag advancement.
   *
   * Ensures all worker threads have completed processing
   * the current tag before advancing to the next tag.
   */
  lf_tag_advancement_barrier_t barrier;

  /**
   * @brief Condition variable for tag barrier requestors.
   *
   * Used to coordinate tag advancement when all requestors
   * have reached zero.
   */
  lf_cond_t global_tag_barrier_requestors_reached_zero;
#endif // LF_SINGLE_THREADED

#if defined(FEDERATED)
  /**
   * @brief Array of intended tag fields for federated execution.
   *
   * Used to track the intended logical tags for each federate
   * in a distributed system. The size is stored in _lf_intended_tag_fields_size.
   * @note Only used in federated execution.
   */
  tag_t** _lf_intended_tag_fields;

  /**
   * @brief Number of intended tag fields allocated.
   *
   * Indicates the size of the _lf_intended_tag_fields array.
   * @note Only used in federated execution.
   */
  int _lf_intended_tag_fields_size;

  /**
   * @brief Flag indicating if a logical tag complete (LTC) message needs to be sent.
   *
   * Used in federated execution to coordinate tag advancement
   * between federates.
   * @note Only used in federated execution.
   */
  bool need_to_send_LTC;
#endif // FEDERATED

#ifdef LF_ENCLAVES
  /**
   * @brief Information about the scheduling enclave.
   *
   * Contains configuration and state specific to the
   * scheduling enclave this environment belongs to.
   * @note Only used in enclave execution.
   */
  enclave_info_t* enclave_info;
#endif
} environment_t;

#if defined(MODAL_REACTORS)
/**
 * @brief Environment for modal reactors.
 * @ingroup Modal
 *
 * Contains state and configuration for modal reactors in this environment.
 * This struct manages the state transitions and reaction triggering in modal reactors.
 * @note Only used in modal reactor execution.
 */
struct mode_environment_t {
  /**
   * @brief Bitmap indicating which reactions have been requested to trigger.
   *
   * Each bit in this 8-bit value represents a different reaction that
   * has been requested to execute. Used to coordinate the execution of
   * reactions during mode transitions.
   */
  uint8_t triggered_reactions_request;

  /**
   * @brief Array of modal reactor states.
   *
   * Stores the current state of each modal reactor in the environment.
   * The size of this array is stored in modal_reactor_states_size.
   * Each entry contains the current mode and transition information
   * for a specific modal reactor.
   */
  reactor_mode_state_t** modal_reactor_states;

  /**
   * @brief Number of modal reactor states allocated.
   *
   * Indicates the size of the modal_reactor_states array.
   * This should match the number of modal reactors in the environment.
   */
  int modal_reactor_states_size;

  /**
   * @brief Array of state variable reset data.
   *
   * Contains information about how state variables should be reset
   * during mode transitions. Each entry specifies the reset behavior
   * for a particular state variable.
   */
  mode_state_variable_reset_data_t* state_resets;

  /**
   * @brief Number of state reset entries allocated.
   *
   * Indicates the size of the state_resets array.
   * This should match the number of state variables that need
   * reset behavior defined.
   */
  int state_resets_size;
};
#endif

/**
 * @brief Initialize an environment struct with parameters given in the arguments.
 * @ingroup Internal
 * @param env The environment to initialize.
 * @param name The name of the environment.
 * @param id The ID of the environment.
 * @param num_workers The number of worker threads in the environment.
 * @param num_timers The number of timer triggers in the environment.
 * @param num_startup_reactions The number of startup reactions in the environment.
 * @param num_shutdown_reactions The number of shutdown reactions in the environment.
 * @param num_reset_reactions The number of reset reactions in the environment.
 * @param num_is_present_fields The number of is_present fields in the environment.
 * @param num_modes The number of modes in the environment.
 * @param num_state_resets The number of state resets in the environment.
 * @param num_watchdogs The number of watchdogs in the environment.
 * @param trace_file_name The name of the trace file to use.
 * @return int 0 on success, -1 on failure.
 */
int environment_init(environment_t* env, const char* name, int id, int num_workers, int num_timers,
                     int num_startup_reactions, int num_shutdown_reactions, int num_reset_reactions,
                     int num_is_present_fields, int num_modes, int num_state_resets, int num_watchdogs,
                     const char* trace_file_name);

/**
 * @brief Verify that the environment is correctly set up.
 * @ingroup Internal
 * @param env The environment to verify.
 */
void environment_verify(environment_t* env);

/**
 * @brief Free the dynamically allocated memory on the environment struct.
 * @ingroup Internal
 * @param env The environment in which we are executing.
 */
void environment_free(environment_t* env);

/**
 * @brief Initialize the start and stop tags on the environment struct.
 * @ingroup Internal
 * @param env The environment to initialize.
 * @param start_time The start time of the environment.
 * @param duration The duration of the environment.
 */
void environment_init_tags(environment_t* env, instant_t start_time, interval_t duration);

/**
 * @brief Update the argument to point to the beginning of the array of environments in this program.
 * @ingroup Internal
 * @note Is code-generated by the compiler
 * @param envs A double pointer which will be dereferenced and modified
 * @return int The number of environments in the array
 */
int _lf_get_environments(environment_t** envs);

#endif // ENVIRONMENT_H
