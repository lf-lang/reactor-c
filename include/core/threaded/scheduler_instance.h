/**
 * @file
 * @author Soroush Bateni
 * @author Edward A. Lee
 * @copyright (c) 2022-2024, The University of Texas at Dallas and The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Common scheduler parameters.
 *
 * This file defines data types and functions that are common across multiple schedulers.
 */

#ifndef LF_SCHEDULER_PARAMS_H
#define LF_SCHEDULER_PARAMS_H

#ifndef NUMBER_OF_WORKERS // Enable thread-related platform functions
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <stdbool.h>
#include <stddef.h> // for size_t

#if SCHEDULER == SCHED_STATIC
// Forward declaration so that lf_scheduler_t is visible in
// scheduler_instructions.h
typedef struct lf_scheduler_t lf_scheduler_t;
#include "lf_types.h"
#include "scheduler_instructions.h"
#endif

#define DEFAULT_MAX_REACTION_LEVEL 100

// Forward declarations
typedef struct environment_t environment_t;
typedef struct custom_scheduler_data_t custom_scheduler_data_t;

/**
 * @brief Paramters used in schedulers of the threaded reactor C runtime.
 *
 * @note Members of this struct are added based on existing schedulers' needs.
 *  These should be expanded to accommodate new schedulers.
 */
typedef struct lf_scheduler_t { 
    /**
     * @brief Environment which the scheduler has access to.
     * 
     */
    struct environment_t * env;

    /**
     * @brief Maximum number of levels for reactions in the program.
     *
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
   * Adding to/subtracting from this variable must be done atomically.
   */
  volatile size_t number_of_idle_workers;

#if SCHEDULER == SCHED_STATIC

    /**
     * @brief Points to an array of program counters for each worker.
     * 
     */
    size_t* pc;

    /**
     * @brief Points to a read-only array of static schedules.
     * 
     */
    const inst_t** static_schedules;

    /**
     * @brief Points to an array of pointers to reactor self instances.
     * 
     */
    self_base_t** reactor_self_instances;

    /**
     * @brief The total number of reactor self instances.
     * 
     */
    size_t num_reactor_self_instances;

    /**
     * @brief Points to an array of pointers to reaction instances.
     * 
     */
    reaction_t** reaction_instances;

    /**
     * @brief Points to an array of integer counters.
     * 
     */
    volatile uint32_t* counters;

#endif
  // Pointer to an optional custom data structure that each scheduler can define.
  // The type is forward declared here and must be declared again in the scheduler source file
  // Is not touched by `init_sched_instance` and must be initialized by each scheduler that needs it
  custom_scheduler_data_t* custom_data;
} lf_scheduler_t;

/**
 * @brief Struct representing the most common scheduler parameters.
 *
 * @param num_reactions_per_level Optional. Default: NULL. An array of
 *  non-negative integers, where each element represents a reaction level
 *  (corresponding to the index), and the value of the element represents the
 *  maximum number of reactions in the program for that level. For example,
 *  num_reactions_per_level = { 2, 3 } indicates that there will be a maximum of
 *  2 reactions in the program with a level of 0, and a maximum of 3 reactions
 *  in the program with a level of 1. Can be NULL.
 * @param num_reactions_per_level_size Optional. The size of the
 * `num_reactions_per_level` array if it is not NULL. If set, it should be the
 * maximum level over all reactions in the program plus 1. If not set,
 * `DEFAULT_MAX_REACTION_LEVEL` will be used.
 */
typedef struct {
  size_t* num_reactions_per_level;
  size_t num_reactions_per_level_size;
} sched_params_t;

/**
 * @brief Initialize `instance` using the provided information.
 *
 * No-op if `instance` is already initialized (i.e., not NULL).
 * This function assumes that mutex is allowed to be recursively locked.
 *
 * @param instance The `lf_scheduler_t` object to initialize.
 * @param number_of_workers  Number of workers in the program.
 * @param params Reference to scheduler parameters in the form of a
 * `sched_params_t`. Can be NULL.
 * @return `true` if initialization was performed. `false` if instance is already
 *  initialized (checked in a thread-safe way).
 */
bool init_sched_instance(struct environment_t* env, lf_scheduler_t** instance, size_t number_of_workers,
                         sched_params_t* params);

#if SCHEDULER == SCHED_STATIC
/**
 * @brief Initialize the static schedule by filling in placeholders which are
 * not considered "compile-time constants" by the compiler.
 */
void initialize_static_schedule();
#endif

#endif // LF_SCHEDULER_PARAMS_H
