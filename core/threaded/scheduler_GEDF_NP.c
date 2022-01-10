/* Global Earliest Deadline First (GEDF) non-preemptive scheduler for the
threaded runtime of the C target of Lingua Franca. */

/*************
Copyright (c) 2021, The University of Texas at Dallas.
Copyright (c) 2019, The University of California at Berkeley.

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
 * Global Earliest Deadline First (GEDF) non-preemptive scheduler for the
 * threaded runtime of the C target of Lingua Franca.
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
#include "../utils/pqueue_support.h"
#include "scheduler_sync_tag_advance.c"
#include <assert.h>

#ifndef MAX_REACTION_LEVEL
#define MAX_REACTION_LEVEL INITIAL_REACT_QUEUE_SIZE
#endif

/////////////////// External Variables /////////////////////////
extern lf_mutex_t mutex;

/////////////////// Scheduler Variables and Structs /////////////////////////
/**
 * @brief Atomically keep track of how many worker threads are idle.
 *
 * Initially assumed that there are 0 idle threads.
 */
semaphore_t* _lf_sched_semaphore; 

/**
 * @brief Indicate whether the program should stop
 * 
 */
volatile bool _lf_sched_should_stop = false;

/**
 * @brief Array of reaction queues.
 * 
 * Each element is a sorted reaction queue for a reaction level
 * 
 */
pqueue_t* _lf_sched_vector_of_reaction_qs[MAX_REACTION_LEVEL + 1];

/**
 * @brief Mutexes for the reaction queues.
 * 
 * In case all threads are idle, there is no need to lock any of these mutexes.
 * Otherwise, depending on the situation, the appropriate mutex for a given
 * level must be locked before accessing the reaction queue of that level.
 * 
 */
lf_mutex_t _lf_sched_vector_of_reaction_qs_mutexes[MAX_REACTION_LEVEL + 1];

/**
 * @brief Queue of currently executing reactions.
 * 
 * Sorted by index (precedence sort)
 */
pqueue_t* executing_q = NULL;

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
 * @brief Insert 'reaction' into _lf_sched_vector_of_reaction_qs at the appropriate level.
 *  
 * @param reaction The reaction to insert.
 */
static inline void _lf_sched_insert_reaction(reaction_t* reaction) {
    size_t reaction_level = LEVEL(reaction->index);
    DEBUG_PRINT("Scheduler: Trying to lock the mutex for level %d.", reaction_level);
    lf_mutex_lock(&_lf_sched_vector_of_reaction_qs_mutexes[reaction_level]);
    DEBUG_PRINT("Scheduler: Locked the mutex for level %d.", reaction_level);
    pqueue_insert(_lf_sched_vector_of_reaction_qs[reaction_level], (void*)reaction);
    lf_mutex_unlock(&_lf_sched_vector_of_reaction_qs_mutexes[reaction_level]);
}

/**
 * @brief Distribute any reaction that is ready to execute to idle worker thread(s).
 *  
 * @return Number of reactions that were successfully distributed to worker threads.
 */ 
int _lf_sched_distribute_ready_reactions() {
    pqueue_t* tmp_queue = NULL;
    // Note: All the threads are idle, which means that they are done inserting
    // reactions. Therefore, the reaction queues can be accessed without locking
    // a mutex.
    for (;_lf_sched_next_reaction_level <= MAX_REACTION_LEVEL; _lf_sched_next_reaction_level++) { 
        tmp_queue = _lf_sched_vector_of_reaction_qs[_lf_sched_next_reaction_level];
        size_t reactions_to_execute = pqueue_size(tmp_queue);
        if (reactions_to_execute) {
            executing_q = tmp_queue;
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
    // while accessing the executing queue (which is pointing to one of the
    // reaction queues).
    size_t workers_to_be_awaken = MIN(_lf_sched_number_of_idle_workers, pqueue_size(executing_q));
    DEBUG_PRINT("Scheduler: Notifying %d workers.", workers_to_be_awaken);
    _lf_sched_number_of_idle_workers -= workers_to_be_awaken;
    DEBUG_PRINT("Scheduler: New number of idle workers: %u.", _lf_sched_number_of_idle_workers);
    if (workers_to_be_awaken > 1) {
        // Notify all the workers except the worker thread that has called this function. 
        lf_semaphore_release(_lf_sched_semaphore, (workers_to_be_awaken-1));
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
 * Advance tag if there are no reactions on the reaction queue. If
 * there are such reactions, distribute them to worker threads.
 * 
 * This function assumes the caller does not hold the 'mutex' lock.
 */
void _lf_sched_try_advance_tag_and_distribute() {
    // if (pqueue_size(executing_q) != 0) {
    //     error_print_and_exit("Scheduler: Executing queue is not empty.");
    // }

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
 * @param number_of_workers Indicate how many workers this scheduler will be managing.
 */
void lf_sched_init(size_t number_of_workers) {
    DEBUG_PRINT("Scheduler: Initializing with %d workers", number_of_workers);
    
    _lf_sched_semaphore = lf_semaphore_new(0);
    _lf_sched_number_of_workers = number_of_workers;

    for (size_t i = 0; i <= MAX_REACTION_LEVEL; i++) {
        // Initialize the reaction queues
        _lf_sched_vector_of_reaction_qs[i] = pqueue_init(INITIAL_REACT_QUEUE_SIZE, in_reverse_order, get_reaction_index,
                get_reaction_position, set_reaction_position, reaction_matches, print_reaction);
        // Initialize the mutexes for the reaction queues
        lf_mutex_init(&_lf_sched_vector_of_reaction_qs_mutexes[i]);
    }
    
    executing_q = _lf_sched_vector_of_reaction_qs[0];
    
    _lf_sched_should_stop = false;
}

/**
 * @brief Free the memory used by the scheduler.
 * 
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    // for (size_t j = 0; j <= MAX_REACTION_LEVEL; j++) {
    //     pqueue_free(_lf_sched_vector_of_reaction_qs[j]); FIXME: This is causing weird
    //     memory errors.
    // }
    pqueue_free(executing_q);
    if (lf_semaphore_destroy(_lf_sched_semaphore) != 0) {
        error_print_and_exit("Scheduler: Could not destroy my semaphore.");
    }
}

///////////////////// Scheduler Worker API (public) /////////////////////////
/**
 * @brief Ask the scheduler for one more reaction.
 * 
 * If there is a ready reaction for worker thread 'worker_number', then a
 * reaction will be returned. If not, this function will block and ask the
 * scheduler for more work. Once work is delivered, it will return a ready
 * reaction. When it's time for the worker thread to stop and exit, it will
 * return NULL.
 * 
 * @param worker_number 
 * @return reaction_t* A reaction for the worker to execute. NULL if the calling
 * worker thread should exit.
 */
reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    // Iterate until the stop_tag is reached or reaction queue is empty
    while (!_lf_sched_should_stop) {
        // Need to lock the mutex for the current level
        size_t current_level = _lf_sched_next_reaction_level - 1;
        DEBUG_PRINT("Scheduler: Worker %d trying to lock the mutex for level %d.", worker_number, current_level);
        lf_mutex_lock(&_lf_sched_vector_of_reaction_qs_mutexes[current_level]);
        DEBUG_PRINT("Scheduler: Worker %d locked the mutex for level %d.", worker_number, current_level);
        reaction_t* reaction_to_return = (reaction_t*)pqueue_pop(executing_q);
        lf_mutex_unlock(&_lf_sched_vector_of_reaction_qs_mutexes[current_level]);
        
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
 * This triggering happens lazily (at a later point when the scheduler deems
 * appropriate), unless worker_number is set to -1 (which indicates an anonymous
 * caller). In that case, the triggering of 'reaction' is done immediately.
 * 
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 * 
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    // Protect against putting a reaction twice on the reaction queue by
    // checking its status.
    if (reaction != NULL && lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        DEBUG_PRINT("Scheduler: Enqueing reaction %s, which has level %lld.",
                    reaction->name, LEVEL(reaction->index));
        _lf_sched_insert_reaction(reaction);
    }
}