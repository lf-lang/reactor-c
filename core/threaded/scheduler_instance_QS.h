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

#include <stdint.h>
#include "../utils/semaphore.h"
#include "scheduler.h"

extern lf_mutex_t mutex;


/**
 * @brief Parameters used in schedulers of the threaded reactor C runtime.
 *
 * @note Members of this struct are added based on existing schedulers' needs.
 *  These should be expanded to accommodate new schedulers.
 */
typedef struct {
    /**
     * @brief Points to a read-only array of static schedules.
     * 
     */
    uint32_t* _lf_sched_static_schedules;

    /**
     * @brief Points to a read-only array of lengths of the static schedules.
     * 
     */
    uint32_t* _lf_sched_schedule_lengths;

    /**
     * @brief Points to an array of semaphores, one for each reaction.
     * 
     */
    semaphore_t* _lf_sched_semaphores;

    /**
     * @brief Indicate whether the program should stop
     *
     */
    volatile bool _lf_sched_should_stop;

    /**
     * @brief Number of workers that this scheduler is managing.
     *
     */
    size_t _lf_sched_number_of_workers;
} _lf_sched_instance_t;

/**
 * @brief Initialize `instance` using the provided information.
 * 
 * No-op if `instance` is already initialized (i.e., not NULL).
 * This function assumes that mutex is allowed to be recursively locked.
 * 
 * @param instance The `_lf_sched_instance_t` object to initialize.
 * @param number_of_workers  Number of workers in the program.
 * @param params Reference to scheduler parameters in the form of a
 * `sched_params_t`. Can be NULL.
 * @return `true` if initialization was performed. `false` if instance is already
 *  initialized (checked in a thread-safe way).
 */
bool init_sched_instance(
    _lf_sched_instance_t** instance,
    size_t number_of_workers,
    sched_params_t* params) {

    return true;
}

#endif // LF_SCHEDULER_PARAMS_H
