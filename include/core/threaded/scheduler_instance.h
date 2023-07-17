/*************
Copyright (c) 2022, The University of Texas at Dallas. Copyright (c) 2022, The
University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/**
 * @file scheduler_params.h
 * @author Soroush Bateni <soroush@utdallas.edu>
 * @brief Scheduler parameters.
 *
 * Meant for book-keeping in the threaded schedulers in the reactor C runtime.
 *
 * @copyright Copyright (c) 2022, The University of Texas at Dallas.
 * @copyright Copyright (c) 2022, The University of California at Berkeley.
 */

#ifndef LF_SCHEDULER_PARAMS_H
#define LF_SCHEDULER_PARAMS_H

#ifndef NUMBER_OF_WORKERS // Enable thread-related platform functions
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include "semaphore.h"
#include <stdbool.h>

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
    struct environment_t * env;
    /**
     * @brief Maximum number of levels for reactions in the program.
     *
     */
    size_t max_reaction_level;

    /**
     * @brief Used by the scheduler to signal the maximum number of worker
     * threads that should be executing work at the same time.
     *
     * Initially, the count is set to 0. Maximum value of count should be
     * `number_of_workers`.
     *
     * For example, if the scheduler releases the semaphore with a count of 4,
     * no more than 4 worker threads should wake up to process reactions.
     *
     * FIXME: specific comment
     */
    lf_semaphore_t* semaphore;

    /**
     * @brief Indicate whether the program should stop
     */
    volatile bool should_stop;

    /**
     * @brief Hold triggered reactions.
     */
    void* triggered_reactions;

    /**
     * @brief An array of mutexes.
     *
     * Can be used to avoid race conditions. Schedulers are allowed to
     * initialize as many mutexes as they deem fit.
     */
    lf_mutex_t* array_of_mutexes;

    /**
     * @brief An array of atomic indexes.
     *
     * Can be used to avoid race conditions. Schedulers are allowed to to use as
     * many indexes as they deem fit.
     */
    volatile int* indexes;

    /**
     * @brief Hold currently executing reactions.
     */
    void* executing_reactions;

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

    /**
     * @brief The next level of reactions to execute.
     */
    volatile size_t next_reaction_level;

    // Pointer to an optional custom data structure that each scheduler can define.
    // The type is forward declared here and must be declared again in the scheduler source file
    // Is not touched by `init_sched_instance` and must be initialized by each scheduler that needs it
    custom_scheduler_data_t * custom_data;
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
bool init_sched_instance(
    struct environment_t* env,
    lf_scheduler_t** instance,
    size_t number_of_workers,
    sched_params_t* params);

#endif // LF_SCHEDULER_PARAMS_H
