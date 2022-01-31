/*************
Copyright (c) 2022, The University of Texas at Dallas.
Copyright (c) 2022, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

/**
 * @brief Paramters used in schedulers in the threaded reactor C runtime.
 * 
 */
typedef struct {
    /**
     * @brief Maximum number of levels for reactions in the program.
     * 
     */
    size_t max_reaction_level;

    /**
     * @brief Used by the scheduler to signal the maximum number of worker threads
     * that should be executing work at the same time.
     *
     * Initially, the count is set to 0. Maximum value of count should be
     * `_lf_sched_number_of_workers`.
     *
     * For example, if the scheduler releases the semaphore with a count of 4, no
     * more than 4 worker threads should wake up to process reactions.
     */
    semaphore_t* _lf_sched_semaphore; 

    /**
     * @brief Indicate whether the program should stop
     * 
     */
    volatile bool _lf_sched_should_stop;

    /**
     * @brief Array of reaction vectors.
     * 
     * Each element is a reaction vector for a reaction level.
     * 
     */
    vector_t* _lf_sched_array_of_reaction_vectors;

    /**
     * @brief Mutexes for the reaction vectors.
     * 
     * In case all threads are idle, there is no need to lock any of these mutexes.
     * Otherwise, depending on the situation, the appropriate mutex for a given
     * level must be locked before accessing the reaction vector of that level.
     * 
     */
    lf_mutex_t* _lf_sched_array_of_reaction_vectors_mutexes;

    /**
     * @brief Indexes for each reaction vector (one per reaction level).
     * 
     * Used for accessing `_lf_sched_array_of_reaction_vectors`. Note that race
     * conditions can occur when accessing this index. This must be avoided either
     * by using atomic operations, locking one of the
     * `_lf_sched_array_of_reaction_vectors_mutexes`, or ensuring that no other
     * worker thread can access this index (e.g., by making sure that all worker
     * threads are idle).
     */
    volatile int* _lf_sched_level_indexes;

    /**
     * @brief Vector of currently executing reactions.
     */
    vector_t* executing_v;

    /**
     * @brief Number of workers that this scheduler is managing.
     * 
     */
    size_t _lf_sched_number_of_workers;

    /**
     * @brief Number of workers that are idle.
     * 
     * Adding to/subtracting from this variable must be done atomically.
     * 
     */
    volatile size_t _lf_sched_number_of_idle_workers;

    /**
     * @brief The next level of reactions to execute.
     * 
     */
    volatile size_t _lf_sched_next_reaction_level;
} _lf_sched_params_t;

#endif // LF_SCHEDULER_PARAMS_H