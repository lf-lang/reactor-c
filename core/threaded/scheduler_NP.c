/* Non-preemptive scheduler for the threaded runtime of the C target of Lingua
Franca. */

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
 * Non-preemptive scheduler for the threaded runtime of the C target of Lingua
 * Franca.
 *  
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "scheduler.h"
#include "../platform.h"
#include "../utils/semaphore.c"
#include "../utils/vector.c"
#include "scheduler_sync_tag_advance.c"
#include <assert.h>

#ifndef MAX_REACTION_LEVEL
#define MAX_REACTION_LEVEL INITIAL_REACT_QUEUE_SIZE
#endif

/////////////////// External Variables /////////////////////////
extern lf_mutex_t mutex;

/////////////////// Scheduler Variables and Structs /////////////////////////
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
volatile bool _lf_sched_should_stop = false;

/**
 * @brief Array of reaction vectors.
 * 
 * Each element is a reaction vector for a reaction level.
 * 
 */
vector_t _lf_sched_array_of_reaction_vectors[MAX_REACTION_LEVEL + 1];

/**
 * @brief Mutexes for the reaction vectors.
 * 
 * In case all threads are idle, there is no need to lock any of these mutexes.
 * Otherwise, depending on the situation, the appropriate mutex for a given
 * level must be locked before accessing the reaction vector of that level.
 * 
 */
lf_mutex_t _lf_sched_array_of_reaction_vectors_mutexes[MAX_REACTION_LEVEL + 1];

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
volatile int _lf_sched_level_indexes[MAX_REACTION_LEVEL + 1] = {0};

/**
 * @brief Vector of currently executing reactions.
 */
vector_t* executing_q = NULL;

/**
 * @brief Number of workers that this scheduler is managing.
 * 
 */
size_t _lf_sched_number_of_workers = 1;

/**
 * @brief Number of workers that are idle.
 * 
 * Adding to/subtracting from this variable must be done atomically.
 * 
 */
volatile size_t _lf_sched_number_of_idle_workers = 0;

/**
 * @brief The next level of reactions to execute.
 * 
 */
volatile size_t _lf_sched_next_reaction_level = 1;

/////////////////// Scheduler Private API /////////////////////////
/**
 * @brief Insert 'reaction' into _lf_sched_array_of_reaction_vectors at the appropriate level.
 *  
 * @param reaction The reaction to insert.
 */
static inline void _lf_sched_insert_reaction(reaction_t* reaction) {
    size_t reaction_level = LEVEL(reaction->index);
    DEBUG_PRINT("Scheduler: Trying to lock the mutex for level %d.", reaction_level);
#ifdef FEDERATED
    // Lock the mutex if federated because a federate can insert reactions with
    // a level equal to the current level.
    size_t current_level = _lf_sched_next_reaction_level - 1;
    // There is a race condition here where `_lf_sched_next_reaction_level` can
    // change after it is cached here. In that case, if the cached value is
    // equal to `reaction_level`, the cost will be an additional unnecessary
    // mutex lock, but no logic error. If the cached value is not equal to
    // `reaction_level`, it can never become `reaction_level` because the
    // scheduler will only change the `_lf_sched_next_reaction_level` if it can
    // ensure that all worker threads are idle, and thus, none are triggering
    // reactions (and therefore calling this function).
    if (reaction_level == current_level) {
        lf_mutex_lock(&_lf_sched_array_of_reaction_vectors_mutexes[reaction_level]);
    }
    // The level index for the current level can sometimes become negative. Set
    // it back to zero before adding a reaction (otherwise worker threads will
    // not be able to see the added reaction).
    if (_lf_sched_level_indexes[reaction_level] < 0) {
        _lf_sched_level_indexes[reaction_level] = 0;
    }
#endif
    DEBUG_PRINT("Scheduler: Locked the mutex for level %d.", reaction_level);
    int reaction_q_level_index = _lf_sched_level_indexes[reaction_level]++;
    *vector_at(
        &_lf_sched_array_of_reaction_vectors[reaction_level], 
        reaction_q_level_index
    ) = (void*)reaction;
    DEBUG_PRINT("Scheduler: Index for level %d is at %d.", reaction_level, reaction_q_level_index);
#ifdef FEDERATED
    if (reaction_level == current_level) {
        lf_mutex_unlock(&_lf_sched_array_of_reaction_vectors_mutexes[reaction_level]);
    }
#endif
}

/**
 * @brief Distribute any reaction that is ready to execute to idle worker thread(s).
 * 
 * @return Number of reactions that were successfully distributed to worker threads.
 */ 
int _lf_sched_distribute_ready_reactions() {
    // Note: All the threads are idle, which means that they are done inserting
    // reactions. Therefore, the reaction vectors can be accessed without locking
    // a mutex.
    for (;_lf_sched_next_reaction_level <= MAX_REACTION_LEVEL; _lf_sched_next_reaction_level++) { 
        executing_q = &_lf_sched_array_of_reaction_vectors[_lf_sched_next_reaction_level];
        size_t reactions_to_execute = vector_size(executing_q);
        if (reactions_to_execute) {
            _lf_sched_next_reaction_level++;
            return reactions_to_execute;
        }
    }
    
    return 0;
}

/**
 * @brief If there is work to be done, notify workers individually.
 * 
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers() {
    // Note: All threads are idle. Therefore, there is no need to lock the mutex
    // while accessing the executing vector (which is pointing to one of the
    // reaction vectors).
    size_t workers_to_awaken = MIN(_lf_sched_number_of_idle_workers, vector_size(executing_q));
    DEBUG_PRINT("Scheduler: Notifying %d workers.", workers_to_awaken);
    _lf_sched_number_of_idle_workers -= workers_to_awaken;
    DEBUG_PRINT("Scheduler: New number of idle workers: %u.", _lf_sched_number_of_idle_workers);
    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this function. 
        lf_semaphore_release(_lf_sched_semaphore, (workers_to_awaken-1));
    }
}

/**
 * @brief Signal all worker threads that it is time to stop.
 * 
 */
void _lf_sched_signal_stop() {
    _lf_sched_should_stop = true;
    lf_semaphore_release(_lf_sched_semaphore, (_lf_sched_number_of_workers - 1));
}

/**
 * @brief Advance tag or distribute reactions to worker threads.
 *
 * Advance tag if there are no reactions in the array of reaction vectors. If
 * there are such reactions, distribute them to worker threads.
 *
 * This function assumes the caller does not hold the 'mutex' lock.
 */
void _lf_sched_try_advance_tag_and_distribute() {
    // Reset the index
    _lf_sched_level_indexes[_lf_sched_next_reaction_level - 1] = 0;

    // Loop until it's time to stop or work has been distributed
    while (true) {
        if (_lf_sched_next_reaction_level == (MAX_REACTION_LEVEL + 1)) {
            _lf_sched_next_reaction_level = 0;
            lf_mutex_lock(&mutex);
            // Nothing more happening at this tag.
            DEBUG_PRINT("Scheduler: Advancing tag.");
            // This worker thread will take charge of advancing tag.
            if (_lf_sched_advance_tag_locked()) {
                DEBUG_PRINT("Scheduler: Reached stop tag.");
                _lf_sched_signal_stop();
                lf_mutex_unlock(&mutex);
                break;
            }
            lf_mutex_unlock(&mutex);
        }

        if (_lf_sched_distribute_ready_reactions() > 0) {
            _lf_sched_notify_workers();
            break;
        }
    }
}

/**
 * @brief Wait until the scheduler assigns work.
 *
 * If the calling worker thread is the last to become idle, it will call on the
 * scheduler to distribute work. Otherwise, it will wait on '_lf_sched_semaphore'.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
void _lf_sched_wait_for_work(size_t worker_number) {
    // Increment the number of idle workers by 1 and check if this is the last
    // worker thread to become idle.
    if (lf_atomic_add_fetch(&_lf_sched_number_of_idle_workers, 1) == _lf_sched_number_of_workers) {
        // Last thread to go idle
        DEBUG_PRINT("Scheduler: Worker %d is the last idle thread.", worker_number);
        // Call on the scheduler to distribute work or advance tag.
        _lf_sched_try_advance_tag_and_distribute();
    } else {
        // Not the last thread to become idle.
        // Wait for work to be released.
        DEBUG_PRINT("Scheduler: Worker %d is trying to acquire the scheduling semaphore.", worker_number);
        lf_semaphore_acquire(_lf_sched_semaphore);
        DEBUG_PRINT("Scheduler: Worker %d acquired the scheduling semaphore.", worker_number);
    }
}

///////////////////// Scheduler Init and Destroy API /////////////////////////
/**
 * @brief Initialize the scheduler.
 *
 * This has to be called before other functions of the scheduler can be used.
 *
 * @param number_of_workers Indicate how many workers this scheduler will be
 *  managing.
 * @param option Pointer to a `sched_options_t` struct containing additional
 *  scheduler options. Can be NULL.
 */
void lf_sched_init(
    size_t number_of_workers, 
    sched_options_t* options
) {
    DEBUG_PRINT("Scheduler: Initializing with %d workers", number_of_workers);
    if (options == NULL || options.max_reactions_per_level == NULL) {
        error_print_and_exit(
            "Scheduler: Internal error. The NP scheduler "
            "requires options.max_reactions_per_level to be set."
        );
    }
    assert(options.max_reactions_per_level_size == (MAX_REACTION_LEVEL+1));
    
    _lf_sched_semaphore = lf_semaphore_new(0);
    _lf_sched_number_of_workers = number_of_workers;

    size_t queue_size = INITIAL_REACT_QUEUE_SIZE;
    for (size_t i = 0; i <= MAX_REACTION_LEVEL; i++) {
        if (options != NULL) {
            if (options.max_reactions_per_level != NULL) {
                queue_size = options.max_reactions_per_level[i];
            }
        }
        // Initialize the reaction vectors
        _lf_sched_array_of_reaction_vectors[i] = vector_new(queue_size);
        // Initialize the mutexes for the reaction vectors
        lf_mutex_init(&_lf_sched_array_of_reaction_vectors_mutexes[i]);
    }
    
    executing_q = &_lf_sched_array_of_reaction_vectors[0];
    
    _lf_sched_should_stop = false;
}

/**
 * @brief Free the memory used by the scheduler.
 * 
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    // for (size_t j = 0; j <= MAX_REACTION_LEVEL; j++) {
    //     vector_free(_lf_sched_array_of_reaction_vectors[j]); FIXME: This is causing weird
    //     memory errors.
    // }
    vector_free(executing_q);
    lf_semaphore_destroy(_lf_sched_semaphore);
}

///////////////////// Scheduler Worker API (public) /////////////////////////
/**
 * @brief Ask the scheduler for one more reaction.
 * 
 * This function blocks until it can return a ready reaction for worker thread
 * 'worker_number' or it is time for the worker thread to stop and exit (where a
 * NULL value would be returned).
 * 
 * @param worker_number 
 * @return reaction_t* A reaction for the worker to execute. NULL if the calling
 * worker thread should exit.
 */
reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    // Iterate until the stop tag is reached or reaction vectors are empty
    while (!_lf_sched_should_stop) {
        // Calculate the current level of reactions to execute
        size_t current_level = _lf_sched_next_reaction_level - 1;
        reaction_t* reaction_to_return = NULL;
#ifdef FEDERATED
        // Need to lock the mutex because federate.c could trigger reactions at
        // the current level (if there is a causality loop)
        lf_mutex_lock(&_lf_sched_array_of_reaction_vectors_mutexes[current_level]);
#endif
        int current_level_q_index = lf_atomic_add_fetch(&_lf_sched_level_indexes[current_level], -1);        
        if (current_level_q_index >= 0) {
            DEBUG_PRINT(
                "Scheduler: Worker %d popping reaction with level %d, index for level: %d.", 
                worker_number, 
                current_level, 
                current_level_q_index
            );
            reaction_to_return = *(reaction_t**)vector_at(executing_q, current_level_q_index);
        }
#ifdef FEDERATED
        lf_mutex_unlock(&_lf_sched_array_of_reaction_vectors_mutexes[current_level]);
#endif

        if (reaction_to_return != NULL) {
            // Got a reaction
            return reaction_to_return;
        }

        DEBUG_PRINT("Worker %d is out of ready reactions.", worker_number);

        // Ask the scheduler for more work and wait
        _lf_sched_wait_for_work(worker_number);
    }

    // It's time for the worker thread to stop and exit.
    return NULL;
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 * 
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    if (!lf_bool_compare_and_swap(&done_reaction->status, queued, inactive)) {
        error_print_and_exit("Unexpected reaction status: %d. Expected %d.", 
            done_reaction->status,
            queued);
    }
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 * 
 * If a worker number is not available (e.g., this function is not called by a
 * worker thread), -1 should be passed as the 'worker_number'.
 * 
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 * 
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 * 
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    // Protect against putting a reaction twice in the reaction vectors by
    // checking its status.
    if (reaction != NULL && lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        DEBUG_PRINT("Scheduler: Enqueing reaction %s, which has level %lld.",
                    reaction->name, LEVEL(reaction->index));
        _lf_sched_insert_reaction(reaction);
    }
}