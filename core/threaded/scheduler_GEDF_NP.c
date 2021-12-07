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
#include "../utils/vector.c"
#include "../utils/pqueue_support.h"


/////////////////// External Variables /////////////////////////
extern pqueue_t* reaction_q;
extern lf_mutex_t mutex;
extern tag_t current_tag;
extern tag_t stop_tag;

/////////////////// External Functions /////////////////////////
/**
 * If there is at least one event in the event queue, then wait until
 * physical time matches or exceeds the time of the least tag on the event
 * queue; pop the next event(s) from the event queue that all have the same tag;
 * extract from those events the reactions that are to be invoked at this
 * logical time and insert them into the reaction queue. The event queue is
 * sorted by time tag.
 *
 * If there is no event in the queue and the keepalive command-line option was
 * not given, and this is not a federated execution with centralized coordination,
 * set the stop tag to the current tag.
 * If keepalive was given, then wait for either request_stop()
 * to be called or an event appears in the event queue and then return.
 *
 * Every time tag is advanced, it is checked against stop tag and if they are
 * equal, shutdown reactions are triggered.
 *
 * This does not acquire the mutex lock. It assumes the lock is already held.
 */
void _lf_next_locked();

/** 
 * Placeholder for code-generated function that will, in a federated
 * execution, be used to coordinate the advancement of tag. It will notify
 * the runtime infrastructure (RTI) that all reactions at the specified
 * logical tag have completed. This function should be called only while
 * holding the mutex lock.
 * @param tag_to_send The tag to send.
 */
void logical_tag_complete(tag_t tag_to_send);

/////////////////// Scheduler Variables and Structs /////////////////////////
/**
 * @brief Atomically keep track of how many worker threads are idle.
 *
 * Initially assumed that there are 0 idle threads.
 */
semaphore_t* _lf_sched_semaphore; 


volatile bool _lf_sched_should_stop = false;

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
 * 
 * Only reading and writing the 'is_idle' field strictly requires acquiring the
 * 'mutex' in this struct.
 */
typedef struct {    
    pqueue_t* output_reactions;  // Reactions produced by the worker after 
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


volatile size_t _lf_sched_number_of_idle_workers = 0;

/**
 * @brief Indicator that execution of at least one tag has completed.
 */
bool _lf_logical_tag_completed = false;

lf_mutex_t _lf_sched_executing_q_mutex;

///////////////////// Scheduler Runtime API (private) /////////////////////////
/**
 * @brief Transfer the contents of worker thread queues to the actual global queues.
 * 
 * FIXME
 * 
 */
void _lf_sched_update_reaction_q(size_t worker_number) {
    // Check if we have actually assigned work to this worker thread previously.
    reaction_t* reaction_to_add = NULL;
    lf_mutex_lock(&mutex);
    DEBUG_PRINT("Scheduler: Emptying queues of Worker %d.", worker_number);

    // Add output reactions to the reaction queue. This will swap the
    // two queues if the _lf_sched_threads_info[worker_number].output_reactions
    // has gotten larger than the reaction_q.
    if (pqueue_size(reaction_q) >= pqueue_size(_lf_sched_threads_info[worker_number].output_reactions)) {
        while ((reaction_to_add = (reaction_t*)pqueue_pop(_lf_sched_threads_info[worker_number].output_reactions)) != NULL) {
            lf_bool_compare_and_swap(&reaction_to_add->status, inactive, queued);
            DEBUG_PRINT(
                "Scheduler: Inserting reaction %s into the reaction queue.",
                reaction_to_add->name
            );
            pqueue_insert(reaction_q, reaction_to_add);
        }
    } else {
        pqueue_t* tmp;
        while ((reaction_to_add = (reaction_t*)pqueue_pop(reaction_q)) != NULL) {
            lf_bool_compare_and_swap(&reaction_to_add->status, inactive, queued);
            DEBUG_PRINT(
                "Scheduler: Inserting reaction %s into the reaction queue.",
                reaction_to_add->name
            );
            pqueue_insert(_lf_sched_threads_info[worker_number].output_reactions, reaction_to_add);
        }
        tmp = reaction_q;
        reaction_q = _lf_sched_threads_info[worker_number].output_reactions;
        _lf_sched_threads_info[worker_number].output_reactions = tmp;
    }
    lf_mutex_unlock(&mutex);
}

/////////////////// Scheduler Worker API (private) /////////////////////////
/**
 * @brief Distribute 'ready_reaction' to the best idle thread.
 * 
 * FIXME
 *
 * @param ready_reaction A reaction that is ready to execute.
 */
static inline void _lf_sched_distribute_ready_reaction(reaction_t* ready_reaction) {
    DEBUG_PRINT("Scheduler: Trying to distribute reaction %s.", ready_reaction->name);
    ready_reaction->status = running;
    lf_mutex_lock(&_lf_sched_executing_q_mutex);
    if (pqueue_insert(executing_q, ready_reaction) != 0) {
        error_print_and_exit("Could not add reaction to the executing queue.");
    }
    lf_mutex_unlock(&_lf_sched_executing_q_mutex);
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
 * This assumes that the caller is not holding any thread mutexes.
 * 
 * @return Number of reactions that were successfully distributed to worker threads.
 */ 
int _lf_sched_distribute_ready_reactions() {    
    reaction_t* r;
    reaction_t* b;
    // Keep track of the chain IDs of blocked reactions.
    unsigned long long mask = 0LL;

    int reactions_distributed = 0;

    // Find a reaction that is ready to execute.
    while ((r = (reaction_t*)pqueue_pop(reaction_q)) != NULL) {
        // Set the reaction aside if it is blocked, either by another
        // blocked reaction or by a reaction that is currently executing.
        if (!_lf_is_blocked_by_executing_or_blocked_reaction(r)) {
            _lf_sched_distribute_ready_reaction(r);
            reactions_distributed++;
            continue;
        }
        // Couldn't execute the reaction. Will have to put it back in the
        // reaction queue.
        pqueue_insert(transfer_q, (void*)r);
        mask = mask | r->chain_id;
    }

    // Push blocked reactions back onto the reaction queue.
    // This will swap the two queues if the transfer_q has
    // gotten larger than the reaction_q.
    if (pqueue_size(reaction_q) >= pqueue_size(transfer_q)) {
        while ((b = (reaction_t*)pqueue_pop(transfer_q)) != NULL) {
            pqueue_insert(reaction_q, b);
        }
    } else {
        pqueue_t* tmp;
        while ((b = (reaction_t*)pqueue_pop(reaction_q)) != NULL) {
            pqueue_insert(transfer_q, b);
        }
        tmp = reaction_q;
        reaction_q = transfer_q;
        transfer_q = tmp;
    }

    DEBUG_PRINT("Scheduler: Distributed %d reactions.", reactions_distributed);
    return reactions_distributed;
}


/**
 * Return true if the worker should stop now; false otherwise.
 * This function assumes the caller holds the mutex lock.
 */
bool _lf_sched_should_stop_locked() {
    // If this is not the very first step, notify that the previous step is complete
    // and check against the stop tag to see whether this is the last step.
    if (_lf_logical_tag_completed) {
        logical_tag_complete(current_tag);
        // If we are at the stop tag, do not call _lf_next_locked()
        // to prevent advancing the logical time.
        if (compare_tags(current_tag, stop_tag) >= 0) {
            return true;
        }
    }
    return false;
}

/**
 * Advance tag. This will also pop events for the newly acquired tag and put
 * the triggered reactions on the reaction queue.
 * 
 * This function assumes the caller holds the 'mutex' lock.
 * 
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_advance_tag_locked() {

    if (_lf_sched_should_stop_locked()) {
        return true;
    }

    _lf_logical_tag_completed = true;

    // Advance time.
    // _lf_next_locked() may block waiting for real time to pass or events to appear.
    // to appear on the event queue. Note that we already
    // hold the mutex lock.
    // tracepoint_worker_advancing_time_starts(worker_number); 
    // FIXME: Tracing should be updated to support scheduler events
    _lf_next_locked();

    DEBUG_PRINT("Scheduler: Done waiting for _lf_next_locked().");
    return false;
}

/**
 * @brief If there is work to be done, notify workers individually.
 * 
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers() {
    size_t workers_to_be_awaken = MIN(_lf_sched_number_of_idle_workers, pqueue_size(executing_q));
    DEBUG_PRINT("Notifying %d workers.", workers_to_be_awaken);
    lf_atomic_fetch_add(&_lf_sched_number_of_idle_workers, -1 * workers_to_be_awaken);
    if (workers_to_be_awaken > 1) {
        // Notify all the workers except the worker thread that has called this function. 
        lf_semaphore_release(_lf_sched_semaphore, (workers_to_be_awaken-1));
    }
}

/**
 * @brief Advance tag or distribute reactions to worker threads.
 *
 * Advance tag if there are no reactions in the reaction queue or in progress. If
 * there are such reactions, distribute them to worker threads. As part of its
 * book-keeping, this function will clear the output_reactions and
 * done_reactions queues of all idle worker threads if appropriate.
 * 
 * This function assumes the caller does not hold the 'mutex' lock.
 * 
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_try_advance_tag() {
    bool return_value = false;

    lf_mutex_lock(&mutex);
    while (pqueue_size(reaction_q) == 0 &&
         pqueue_size(executing_q) == 0) {
        // Nothing more happening at this logical time.

        // Protect against asynchronous events while advancing time by locking
        // the mutex.
        DEBUG_PRINT("Scheduler: Advancing time.");
        // This thread will take charge of advancing time.
        if (_lf_sched_advance_tag_locked()) {
            DEBUG_PRINT("Scheduler: Reached stop tag.");
            return_value = true;
            break;
        }
    }
    lf_mutex_unlock(&mutex);
    
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
 * @brief Wait until the scheduler assigns work.
 *
 * FIXME
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
void _lf_sched_wait_for_work(size_t worker_number) {
    
    _lf_sched_update_reaction_q(worker_number);
    
    size_t previous_idle_workers = lf_atomic_fetch_add(&_lf_sched_number_of_idle_workers, 1);

    // Check if this is the last worker thread to be idle
    if (previous_idle_workers == (_lf_sched_number_of_workers - 1)) {
        // Last thread to go idle
        DEBUG_PRINT("Scheduler: Worker %d is the last idle thread.", worker_number);
        if(_lf_sched_try_advance_tag()) {
            _lf_sched_signal_stop();
        }

        if (_lf_sched_distribute_ready_reactions() > 0) {
            _lf_sched_notify_workers();
        }
    } else {
        // Wait for work to be released
        lf_semaphore_acquire(_lf_sched_semaphore);
    }
}

///////////////////// Scheduler Init and Destroy API /////////////////////////
/**
 * @brief Initialize the scheduler.
 * 
 * This has to be called before the main thread of the scheduler is created.
 * 
 * @param number_of_workers Indicate how many workers this scheduler will be managing.
 */
void lf_sched_init(size_t number_of_workers) {
    DEBUG_PRINT("Scheduler: Initializing with %d workers", number_of_workers);
    
    _lf_sched_semaphore = lf_semaphore_new(0);
    _lf_sched_number_of_workers = number_of_workers;
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
 * This must be called after the main scheduler thread exits.
 * 
 */
void lf_sched_free() {
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        pqueue_free(_lf_sched_threads_info[i].output_reactions);
    }
    pqueue_free(transfer_q);
    pqueue_free(executing_q);
    if (lf_semaphore_destroy(_lf_sched_semaphore) != 0) {
        error_print_and_exit("Scheduler: Could not destroy my semaphore.");
    }
    free(_lf_sched_threads_info);
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
reaction_t* lf_sched_pop_ready_reaction(int worker_number) {
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

    // if (reaction_to_return == NULL && _lf_sched_number_of_workers > 1) {
    //     // Try to steal
    //     int index_to_steal = (worker_number + 1) % _lf_sched_number_of_workers;
    //     lf_mutex_lock(&_lf_sched_threads_info[index_to_steal].mutex);
    //     reaction_to_return = 
    //         pqueue_pop(_lf_sched_threads_info[index_to_steal].ready_reactions);
    //     if (reaction_to_return != NULL) {
    //         DEBUG_PRINT(
    //             "Worker %d: Had nothing on my ready queue. Stole reaction %s from %d", 
    //             worker_number,
    //             reaction_to_return->name,
    //             index_to_steal);
    //     }
    //     lf_mutex_unlock(&_lf_sched_threads_info[index_to_steal].mutex);
    // }

    // return reaction_to_return;
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
    if (done_reaction->status != running) {
        error_print_and_exit("Unexpected reaction status: %d. Expected %d.", 
            done_reaction->status,
            running);
    }
    done_reaction->status = inactive;
    return;
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * enqueue 'reaction'.
 * 
 * This enqueuing happens lazily (at a later point when the scheduler deems
 * appropriate), unless worker_number is set to -1. In that case, the enqueuing
 * of 'reaction' is done immediately.
 * 
 * @param reaction The reaction to enqueue.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 should be used if the scheduler should handle
 *  enqueuing the reaction immediately.
 */
void lf_sched_worker_trigger_reaction(int worker_number, reaction_t* reaction) {
    if (worker_number == -1) {
        // The scheduler should handle this immediately
        lf_mutex_lock(&mutex);
        // Protect against putting a reaction twice on the reaction queue by
        // checking its status.
        if (reaction != NULL && lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
            DEBUG_PRINT("Scheduler: Enqueing reaction %s, which has level %lld.",
                        reaction->name, reaction->index & 0xffffLL);
            // Immediately put 'reaction' on the reaction queue.
            pqueue_insert(reaction_q, reaction);
        }
        lf_mutex_unlock(&mutex);
        return;
    }
    // Protect against putting a reaction twice on the reaction queue by
    // checking its status.
    if (reaction != NULL && lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        DEBUG_PRINT("Scheduler: Worker %d: Enqueuing reaction %s, which has level %lld.",
        		worker_number, reaction->name, reaction->index & 0xffffLL);
        reaction->worker_affinity = worker_number;
        // Note: The scheduler will check that we don't enqueue this reaction
        // twice when it is actually pushing it to the global reaction queue.
        pqueue_insert(_lf_sched_threads_info[worker_number].output_reactions, (void*)reaction);
    }
}