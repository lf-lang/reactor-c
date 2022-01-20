/* Global Earliest Deadline First (GEDF) non-preemptive scheduler with chain ID
for the threaded runtime of the C target of Lingua Franca. */

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
 * Global Earliest Deadline First (GEDF) non-preemptive scheduler with chain ID
 * for the threaded runtime of the C target of Lingua Franca.
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
#include "../utils/semaphore.h"
#include "../utils/vector.h"
#include "../utils/pqueue_support.h"
#include "sync_tag_advance.c"
#include <assert.h>


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
 * @brief Queue of triggered reactions at the current tag.
 * 
 */
pqueue_t* reaction_q;


/**
 * @brief Queue used to keep reactions temporarily.
 * 
 */
pqueue_t* transfer_q;

/**
 * @brief Queue of currently executing reactions.
 * 
 * Sorted by index (precedence sort)
 */
pqueue_t* executing_q;

/**
 * @brief Information about one worker thread.
 */
typedef struct {    
    pqueue_t* output_reactions; // Reactions produced by the worker after 
                                // executing a reaction. The worker thread does
                                // not need to acquire any mutex lock to read
                                // this and the scheduler does not need to
                                // acquire any mutex lock to write to this as
                                // long as the worker thread is idle.
} _lf_sched_thread_info_t;

/**
 * @brief Information about worker threads. @see _lf_sched_thread_info_t.
 * 
 */
_lf_sched_thread_info_t* _lf_sched_threads_info;

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
 * @brief Mutex that must be acquired by workers before accessing the executing_q.
 * 
 */
lf_mutex_t _lf_sched_executing_q_mutex;

/////////////////// Scheduler Worker API (private) /////////////////////////
/**
 * @brief Distribute 'ready_reaction' to the best idle thread.
 * 
 * This assumes that the caller is holding 'mutex'.
 * 
 * @param ready_reaction A reaction that is ready to execute.
 */
static inline void _lf_sched_distribute_ready_reaction_locked(reaction_t* ready_reaction) {
    DEBUG_PRINT("Scheduler: Trying to distribute reaction %s.", ready_reaction->name);
    ready_reaction->status = running;
    if (pqueue_insert(executing_q, ready_reaction) != 0) {
        error_print_and_exit("Could not add reaction to the executing queue.");
    }
}

/**
 * Return true if the first reaction has precedence over the second, false otherwise.
 * @param r1 The first reaction.
 * @param r2 The second reaction.
 */
bool _lf_has_precedence_over(reaction_t* r1, reaction_t* r2) {
    if (LEVEL(r1->index) < LEVEL(r2->index)
            && OVERLAPPING(r1->chain_id, r2->chain_id)) {
        return true;
    }
    return false;
}


/**
 * If the reaction is blocked by a currently executing
 * reaction, return true. Otherwise, return false.
 * A reaction blocks the specified reaction if it has a
 * level less than that of the specified reaction and it also has
 * an overlapping chain ID, meaning that it is (possibly) upstream
 * of the specified reaction.
 * This function assumes the mutex is held because it accesses
 * the executing_q.
 * @param reaction The reaction.
 * @return true if this reaction is blocked, false otherwise.
 */
bool _lf_is_blocked_by_executing_or_blocked_reaction(reaction_t* reaction) {
    if (reaction == NULL) {
        return false;
    }
    // The head of the executing_q has the lowest level of anything
    // on the queue, and that level is also lower than anything on the
    // transfer_q (because reactions on the transfer queue are blocked
    // by reactions on the executing_q). Hence, if the candidate reaction
    // has a level less than or equal to that of the head of the
    // executing_q, then it is executable and we don't need to check
    // the contents of either queue further.
    if (pqueue_size(executing_q) > 0
            && reaction->index <= ((reaction_t*) pqueue_peek(executing_q))->index) {
        return false;
    }

    // Candidate reaction has a level larger than some executing reaction,
    // so we need to check whether it is blocked by any executing reaction
    // or any reaction that is is blocked by an executing reaction.

    // The following iterates over the elements of those queues in arbitrary
    // order, not in priority order.
    // NOTE: If chainID is disabled, this will never yield a false result
    // if the above test failed to yield a false result. But the check
    // is kept here in anticipation of chainID becoming enabled sometime
    // in the future.  It is relatively harmless because the calling thread
    // has nothing to do anyway.
    
    // NOTE: Element 0 of the pqueue is not used and will likely be null.
    for (size_t i = 1; i < executing_q->size; i++) {
        reaction_t* running = (reaction_t*) executing_q->d[i];
        if (_lf_has_precedence_over(running, reaction)) {
            DEBUG_PRINT("Reaction %s is blocked by executing reaction %s.", reaction->name, running->name);
            return true;
        }
    }
    for (size_t i = 1; i < transfer_q->size; i++) {
        reaction_t* blocked = (reaction_t*) transfer_q->d[i];
        if (_lf_has_precedence_over(blocked, reaction)) {
            DEBUG_PRINT("Reaction %s is blocked by blocked reaction %s.", reaction->name, blocked->name);
            return true;
        }
    }

    // printf("Not blocking for reaction with chainID %llu and level %llu\n", reaction->chain_id, reaction->index);
    // pqueue_dump(executing_q, stdout, executing_q->prt);
    return false;
}

/**
 * @brief Distribute any reaction that is ready to execute to idle worker thread(s).
 * 
 * This assumes that the caller is holding 'mutex' and is not holding any thread mutexes.
 * 
 * @return Number of reactions that were successfully distributed to worker threads.
 */ 
int _lf_sched_distribute_ready_reactions_locked() {    
    reaction_t* r;

    // Keep track of the number of reactions distributed
    int reactions_distributed = 0;

    // Find a reaction that is ready to execute.
    while ((r = (reaction_t*)pqueue_pop(reaction_q)) != NULL) {
        // Set the reaction aside if it is blocked, either by another
        // blocked reaction or by a reaction that is currently executing.
        if (!_lf_is_blocked_by_executing_or_blocked_reaction(r)) {
            _lf_sched_distribute_ready_reaction_locked(r);
            reactions_distributed++;
            continue;
        }
        // Couldn't execute the reaction. Will have to put it back in the
        // reaction queue.
        pqueue_insert(transfer_q, (void*)r);
    }

    // Push blocked reactions back onto the reaction queue.
    // This will swap the two queues if the transfer_q has
    // gotten larger than the reaction_q.
    pqueue_empty_into(&reaction_q, &transfer_q);

    DEBUG_PRINT("Scheduler: Distributed %d reactions.", reactions_distributed);
    return reactions_distributed;
}

/**
 * @brief If there is work to be done, notify workers individually.
 * 
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers() {
    size_t workers_to_awaken = MIN(_lf_sched_number_of_idle_workers, pqueue_size(executing_q));
    DEBUG_PRINT("Notifying %d workers.", workers_to_awaken);
    lf_atomic_fetch_add(&_lf_sched_number_of_idle_workers, -1 * workers_to_awaken);
    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this function. 
        lf_semaphore_release(_lf_sched_semaphore, (workers_to_awaken-1));
    }
}

/**
 * @brief Advance tag or distribute reactions to worker threads.
 *
 * Advance tag if there are no reactions on the reaction queue. If
 * there are such reactions, distribute them to worker threads.
 * 
 * This function assumes the caller does not hold the 'mutex' lock.
 * 
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_try_advance_tag_and_distribute() {
    bool return_value = false;

    // Executing queue must be empty when this is called.
    assert(pqueue_size(executing_q) == 0);

    lf_mutex_lock(&mutex);
    while (pqueue_size(reaction_q) == 0) {
        // Nothing more happening at this tag.
        DEBUG_PRINT("Scheduler: Advancing tag.");
        // This worker thread will take charge of advancing tag.
        if (_lf_sched_advance_tag_locked()) {
            DEBUG_PRINT("Scheduler: Reached stop tag.");
            return_value = true;
            break;
        }
    }

    int reactions_distributed = _lf_sched_distribute_ready_reactions_locked();
    lf_mutex_unlock(&mutex);

    if (reactions_distributed) {
        _lf_sched_notify_workers();
    }
    
    // pqueue_dump(executing_q, print_reaction);
    return return_value;
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
 * @brief Transfer the contents of the 'output_reactions' queue to the 'reaction_q'.
 * 
 * This function assumes that the 'mutex' is not locked.
 * 
 * @param worker_number The worker number of the calling worker thread.
 */
void _lf_sched_update_reaction_q(size_t worker_number) {
    lf_mutex_lock(&mutex);
    DEBUG_PRINT("Scheduler: Emptying the output reaction queue of Worker %d.", worker_number);
    pqueue_empty_into(&reaction_q, &_lf_sched_threads_info[worker_number].output_reactions);
    lf_mutex_unlock(&mutex);
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
    
    // First, empty the 'output_reactions' for this worker thread into the 'reaction_q'.
    _lf_sched_update_reaction_q(worker_number);
    
    // Second, increment the number of idle workers by 1 and
    // check if this is the last worker thread to become idle.
    if (lf_atomic_add_fetch(&_lf_sched_number_of_idle_workers, 1) == _lf_sched_number_of_workers) {
        // Last thread to go idle
        DEBUG_PRINT("Scheduler: Worker %d is the last idle thread.", worker_number);
        // Call on the scheduler to distribute work or advance tag.
        if(_lf_sched_try_advance_tag_and_distribute()) {
            // It's time to stop
            _lf_sched_signal_stop();
        }
    } else {
        // Not the last thread to become idle.
        // Wait for work to be released.
        lf_semaphore_acquire(_lf_sched_semaphore);
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

    // Reaction queue ordered first by deadline, then by level.
    // The index of the reaction holds the deadline in the 48 most significant bits,
    // the level in the 16 least significant bits.
    reaction_q = pqueue_init(INITIAL_REACT_QUEUE_SIZE, in_reverse_order, get_reaction_index,
            get_reaction_position, set_reaction_position, reaction_matches, print_reaction); 
    transfer_q = pqueue_init(_lf_number_of_threads, in_reverse_order, get_reaction_index,
        get_reaction_position, set_reaction_position, reaction_matches, print_reaction);
    // Create a queue on which to put reactions that are currently executing.
    executing_q = pqueue_init(_lf_number_of_threads, in_reverse_order, get_reaction_index,
        get_reaction_position, set_reaction_position, reaction_matches, print_reaction);
    
    _lf_sched_threads_info = 
        (_lf_sched_thread_info_t*)calloc(
            sizeof(_lf_sched_thread_info_t), _lf_sched_number_of_workers);
    
    _lf_sched_should_stop = false;
    
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        _lf_sched_threads_info[i].output_reactions = 
            pqueue_init(INITIAL_REACT_QUEUE_SIZE, in_reverse_order, get_reaction_index,
            get_reaction_position, set_reaction_position, reaction_matches, print_reaction);
    }

    lf_mutex_init(&_lf_sched_executing_q_mutex);
}

/**
 * @brief Free the memory used by the scheduler.
 * 
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        pqueue_free(_lf_sched_threads_info[i].output_reactions);
    }
    // pqueue_free(reaction_q); FIXME: This might be causing weird memory errors
    pqueue_free(transfer_q);
    pqueue_free(executing_q);
    lf_semaphore_destroy(_lf_sched_semaphore);
    free(_lf_sched_threads_info);
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
    // Iterate until the stop_tag is reached or reaction queue is empty
    while (!_lf_sched_should_stop) {
        lf_mutex_lock(&_lf_sched_executing_q_mutex);
        reaction_t* reaction_to_return = (reaction_t*)pqueue_pop(executing_q);
        lf_mutex_unlock(&_lf_sched_executing_q_mutex);
        
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
 * @param done_reaction The reaction is that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    if (!lf_bool_compare_and_swap(&done_reaction->status, running, inactive)) {
        error_print_and_exit("Unexpected reaction status: %d. Expected %d.", 
            done_reaction->status,
            running);
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
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    if (worker_number == -1) {
        // The scheduler should handle this immediately
        // Protect against putting a reaction twice on the reaction queue by
        // checking its status.
        if (reaction != NULL && lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
            lf_mutex_lock(&mutex);
            DEBUG_PRINT("Scheduler: Enqueing reaction %s, which has level %lld.",
                        reaction->name, LEVEL(reaction->index));
            // Immediately put 'reaction' on the reaction queue.
            pqueue_insert(reaction_q, (void*)reaction);
            lf_mutex_unlock(&mutex);
        }
        return;
    }
    // Protect against putting a reaction twice on the reaction queue by
    // checking its status.
    if (reaction != NULL && lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        DEBUG_PRINT("Scheduler: Worker %d: Enqueuing reaction %s, which has level %lld.",
        		worker_number, reaction->name, LEVEL(reaction->index));
        reaction->worker_affinity = worker_number;
        // Note: The scheduler has already checked that we are not enqueueing
        // this reaction twice.
        pqueue_insert(_lf_sched_threads_info[worker_number].output_reactions, (void*)reaction);
    }
}