/**
 * @file scheduler_instance.h
 * @author Soroush Bateni
 * @author Edward A. Lee
 *
 * @brief Common scheduler parameters.
 *
 * This file defines data types and functions that are common across multiple schedulers.
 * @ingroup Internal
 */

#ifndef LF_SCHEDULER_PARAMS_H
#define LF_SCHEDULER_PARAMS_H

#ifndef NUMBER_OF_WORKERS // Enable thread-related platform functions
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <stdbool.h>
#include <stddef.h> // for size_t

#define DEFAULT_MAX_REACTION_LEVEL 100

// Forward declarations
typedef struct environment_t environment_t;
typedef struct custom_scheduler_data_t custom_scheduler_data_t;

/**
 * @brief Parameters used in schedulers of the threaded reactor C runtime.
 * @ingroup Internal
 *
 * @note Members of this struct are added based on existing schedulers' needs.
 *  These should be expanded to accommodate new schedulers.
 */
typedef struct lf_scheduler_t {
  /**
   * @brief Pointer to the environment.
   */
  struct environment_t* env;

  /**
   * @brief Maximum number of levels for reactions in the program.
   */
  size_t max_reaction_level;

  /**
   * @brief Indicate whether the program should stop
   */
  volatile bool should_stop;

  /**
   * @brief An array of atomic indexes.
   *
   * Can be used to avoid race conditions. Schedulers are allowed to to use as
   * many indexes as they deem fit.
   */
  volatile int* indexes;

  /**
   * @brief Hold reactions temporarily.
   */
  void* transfer_reactions;

  /**
   * @brief Number of workers that this scheduler is managing.
   */
  size_t number_of_workers;

  /**
   * @brief Number of workers that are idle.
   *
   * Adding to/subtracting from this variable must be done atomically.
   */
  volatile size_t number_of_idle_workers;

  /**
   * @brief Pointer to an optional custom data structure that each scheduler can define.
   *
   * The type is forward declared here and must be declared again in the scheduler source file
   * Is not touched by `init_sched_instance` and must be initialized by each scheduler that needs it
   */
  custom_scheduler_data_t* custom_data;
} lf_scheduler_t;

/**
 * @brief Struct representing the most common scheduler parameters.
 * @ingroup Internal
 */
typedef struct {
  /**
   * @brief An array of non-negative integers, where each element represents a reaction level.
   *
   * The reaction level is its index, and the value of the element represents the
   * maximum number of reactions in the program for that level. For example,
   * `num_reactions_per_level = { 2, 3 }` indicates that there will be a maximum of
   * 2 reactions in the program with a level of 0, and a maximum of 3 reactions
   * in the program with a level of 1. This element can be NULL.
   */
  size_t* num_reactions_per_level;

  /**
   * @brief The size of the `num_reactions_per_level` array.
   *
   * If set, it should be the maximum level over all reactions in the program plus 1.
   * If not set, @ref DEFAULT_MAX_REACTION_LEVEL will be used.
   */
  size_t num_reactions_per_level_size;
} sched_params_t;

/**
 * @brief Initialize `instance` using the provided information.
 *
 * This is a no-op if `instance` is already initialized (i.e., not NULL).
 * This function assumes that mutex is allowed to be recursively locked.
 *
 * @param env The environment to initialize the scheduler for.
 * @param instance The `lf_scheduler_t` object to initialize.
 * @param number_of_workers The number of workers in the program.
 * @param params Reference to scheduler parameters in the form of a `sched_params_t`.
 *  This can be NULL.
 * @return `true` if initialization was performed. `false` if instance is already
 *  initialized (checked in a thread-safe way).
 */
bool init_sched_instance(struct environment_t* env, lf_scheduler_t** instance, size_t number_of_workers,
                         sched_params_t* params);

#endif // LF_SCHEDULER_PARAMS_H
