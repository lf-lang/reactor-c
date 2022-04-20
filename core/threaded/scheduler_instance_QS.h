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
 * @file scheduler_params_QS.h (modeled after scheduler_params.h)
 * @author Shaokai Lin <shaokai@berkeley.edu>
 * @brief Scheduler parameters.
 *
 * Define the scheduler struct for the quasi-static scheduler.
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
#include "static_schedule.h"

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
    const uint32_t*** static_schedules;

    /**
     * @brief Points to a read-only array of static schedules.
     * 
     */
    const uint32_t** current_schedule;

    /**
     * @brief Points to a read-only array of lengths of the static schedules.
     * 
     */
    const uint32_t** schedule_lengths;

    /**
     * @brief Points to an array of program counters for each worker.
     * 
     */
    size_t* pc;

    /**
     * @brief Points to an array of semaphores, one for each reaction.
     * 
     * The indices that correspond to different reactions can be stored
     * in the reaction instances. The initial count of the semaphores
     * should also be stored in the reaction instances (in reactor.c).
     * 
     */
    semaphore_t** semaphores;

    /**
     * @brief Indicate whether the program should stop
     *
     */
    volatile bool should_stop;

    /**
     * @brief Number of workers that this scheduler is managing.
     *
     */
    size_t number_of_workers;
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

    // Check if the instance is already initialized
    lf_mutex_lock(&mutex); // Safeguard against multiple threads calling this 
                           // function.
    if (*instance != NULL) {
        // Already initialized
        lf_mutex_unlock(&mutex);
        return false;
    } else {
        *instance =
            (_lf_sched_instance_t*)calloc(1, sizeof(_lf_sched_instance_t));
    }

    (*instance)->static_schedules = static_schedules;
    (*instance)->current_schedule = NULL;
    (*instance)->schedule_lengths = schedule_lengths;
    (*instance)->pc = calloc(number_of_workers, sizeof(size_t));
    (*instance)->semaphores = (semaphore_t**)malloc(number_of_workers * sizeof(semaphore_t*));
    for (int w = 0; w < number_of_workers; w++) {
        // FIXME: Get count from each reaction.
        (*instance)->semaphores[w] = lf_semaphore_new();
    }

    lf_mutex_unlock(&mutex);
    return true;
}

#endif // LF_SCHEDULER_PARAMS_H
