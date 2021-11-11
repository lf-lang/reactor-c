/* Scheduler for the threaded version of the C target of Lingua Franca. */

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
 * Scheduler for the threaded version of the C target of Lingua Franca.
 *  
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "reactor.h"
#include "platform.h"
#include "pqueue.h"

extern pqueue_t* reaction_q;
extern pqueue_t* executing_q;
extern lf_mutex_t mutex;

lf_cond_t _lf_sched_need_more_work;

pqueue_t* transfer_q;
// Queue of currently executing reactions.
pqueue_t* executing_q; // Sorted by index (precedence sort)

size_t _lf_sched_balancing_index = 0;

typedef struct {
    lf_mutex_t mutex;
    lf_cond_t cond;
    pqueue_t* ready_reactions;
    pqueue_t* done_reactions;
    bool should_stop;
} _lf_sched_thread_info_t;

_lf_sched_thread_info_t* _lf_sched_threads_info;

size_t _lf_sched_number_of_workers = 1;

///////////////////// Scheduler Worker APIs /////////////////////////
reaction_t* _lf_sched_pop_ready_reaction(int worker_number) {
    lf_mutex_lock(&_lf_sched_threads_info[worker_number].mutex);
    reaction_t* reaction_to_return = 
        pqueue_pop(_lf_sched_threads_info[worker_number].ready_reactions);
    lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);

    if (reaction_to_return == NULL && _lf_sched_number_of_workers > 1) {
        // Try to steal
        int index_to_steal = (worker_number + 1) % _lf_sched_number_of_workers;
        lf_mutex_lock(&_lf_sched_threads_info[index_to_steal].mutex);
        reaction_to_return = 
            pqueue_pop(_lf_sched_threads_info[index_to_steal].ready_reactions);
        if (reaction_to_return != NULL) {
            DEBUG_PRINT(
                "Worker %d: Had nothing on my ready queue. Stole reaction %s from %d", 
                worker_number,
                reaction_to_return->name,
                index_to_steal);
        }
        lf_mutex_unlock(&_lf_sched_threads_info[index_to_steal].mutex);
    }

    return reaction_to_return;
}

void _lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    lf_mutex_lock(&_lf_sched_threads_info[worker_number].mutex);
    pqueue_insert(_lf_sched_threads_info[worker_number].done_reactions, done_reaction);
    lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);
}


void _lf_sched_worker_ask_for_work(size_t worker_number) {
    lf_mutex_lock(&mutex);
    DEBUG_PRINT("Worker %d: Asking the scheduler for more work", worker_number);
    lf_cond_signal(&_lf_sched_need_more_work);
    lf_mutex_unlock(&mutex);
}

void _lf_sched_worker_wait_for_work(size_t worker_number) {
    _lf_sched_worker_ask_for_work(worker_number);
    lf_mutex_lock(&_lf_sched_threads_info[worker_number].mutex);
    if (pqueue_size(
            _lf_sched_threads_info[worker_number].ready_reactions
            ) > 0 // More work to be done
            || _lf_sched_threads_info[worker_number].should_stop // Time to stop
        ) {
        lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);
        return;
    }
    DEBUG_PRINT("Worker %d: Waiting on work to be handed out.", worker_number);
    lf_cond_wait(&_lf_sched_threads_info[worker_number].cond, &_lf_sched_threads_info[worker_number].mutex);
    lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);
}

bool _lf_sched_worker_should_stop(size_t worker_number) {
    lf_mutex_lock(&_lf_sched_threads_info[worker_number].mutex);
    bool should_stop = _lf_sched_threads_info[worker_number].should_stop;
    lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);
    return should_stop;
}


///////////////////// Scheduler Init /////////////////////////
void _lf_sched_init(size_t number_of_workers) {
    DEBUG_PRINT("Scheduler: Initializing with %d workers", number_of_workers);
    
    lf_cond_init(&_lf_sched_need_more_work);
    
    _lf_sched_number_of_workers = number_of_workers;
    
    _lf_sched_threads_info = 
        (_lf_sched_thread_info_t*)malloc(
            sizeof(_lf_sched_thread_info_t) * _lf_sched_number_of_workers);
    
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        lf_cond_init(&_lf_sched_threads_info[i].cond);
        lf_mutex_init(&_lf_sched_threads_info[i].mutex);
        _lf_sched_threads_info[i].ready_reactions = 
            pqueue_init(
                INITIAL_REACT_QUEUE_SIZE, 
                in_reverse_order, 
                get_reaction_index,
                get_reaction_position, 
                set_reaction_position, 
                reaction_matches, 
                print_reaction
            );
        _lf_sched_threads_info[i].done_reactions = 
            pqueue_init(
                INITIAL_REACT_QUEUE_SIZE, 
                in_reverse_order, 
                get_reaction_index,
                get_reaction_position, 
                set_reaction_position, 
                reaction_matches, 
                print_reaction
            );
        _lf_sched_threads_info[i].should_stop = false;
    }
    transfer_q = pqueue_init(INITIAL_REACT_QUEUE_SIZE, in_reverse_order, get_reaction_index,
            get_reaction_position, set_reaction_position, reaction_matches, print_reaction);
    // Create a queue on which to put reactions that are currently executing.
    executing_q = pqueue_init(_lf_number_of_threads, in_reverse_order, get_reaction_index,
        get_reaction_position, set_reaction_position, reaction_matches, print_reaction);
}

void _lf_sched_free() {
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        pqueue_free(_lf_sched_threads_info[i].ready_reactions);
        pqueue_free(_lf_sched_threads_info[i].done_reactions);
    }
    pqueue_free(transfer_q);
    pqueue_free(executing_q);
    free(_lf_sched_threads_info);
}

///////////////////// Scheduler APIs /////////////////////////
static inline void _lf_sched_distribute_ready_reaction(reaction_t* ready_reaction) {
    DEBUG_PRINT("Scheduler: Trying to distribute reaction %s.", ready_reaction->name);
    lf_mutex_lock(&_lf_sched_threads_info[_lf_sched_balancing_index].mutex);
    DEBUG_PRINT(
        "Scheduler: Assigning reaction %s to worker %d.",
        ready_reaction->name,
        _lf_sched_balancing_index);
    pqueue_insert(
        _lf_sched_threads_info[_lf_sched_balancing_index].ready_reactions,
        ready_reaction
    );
    lf_cond_signal(&_lf_sched_threads_info[_lf_sched_balancing_index].cond);
    lf_mutex_unlock(&_lf_sched_threads_info[_lf_sched_balancing_index++].mutex);

    // Push the reaction on the executing queue in order to prevent any
    // reactions that may depend on it from executing before this reaction is finished.
    pqueue_insert(executing_q, ready_reaction);
    
    if (_lf_sched_balancing_index == _lf_sched_number_of_workers) {
        _lf_sched_balancing_index = 0;
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
bool _lf_sched_is_blocked_by_executing_reaction(reaction_t* reaction) {
    if (reaction == NULL) {
        return false;
    }
    for (size_t i = 1; i < executing_q->size; i++) {
        reaction_t* running = (reaction_t*) executing_q->d[i];
        if (_lf_has_precedence_over(running, reaction)) {
            DEBUG_PRINT("Reaction %s is blocked by reaction %s.", reaction->name, running->name);
            return true;
        }
    }
    // NOTE: checks against the transfer_q are not performed in 
    // this function but at its call site (where appropriate).

    // printf("Not blocking for reaction with chainID %llu and level %llu\n", reaction->chain_id, reaction->index);
    // pqueue_dump(executing_q, stdout, executing_q->prt);
    return false;
}

// Indicator that execution at at least one tag has completed.
bool _lf_logical_tag_completed = false;


/** 
 * Placeholder for code-generated function that will, in a federated
 * execution, be used to coordinate the advancement of tag. It will notify
 * the runtime infrastructure (RTI) that all reactions at the specified
 * logical tag have completed. This function should be called only while
 * holding the mutex lock.
 * @param tag_to_send The tag to send.
 */
void logical_tag_complete(tag_t tag_to_send);

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

void _lf_next_locked();

/**
 * Return the first ready (i.e., unblocked) reaction in the reaction queue if
 * there is one. Return `NULL` if all pending reactions are blocked.
 *
 * The reaction queue is sorted by index, where a lower index appears
 * earlier in the queue. The first reaction in the reaction queue is
 * ready to execute if it is not blocked by any reaction that is currently
 * executing in another thread. If that first reaction is blocked, then
 * the second reaction is ready to execute if it is not blocked by any
 * reaction that is currently executing (if it is blocked by the first
 * reaction, then it is also blocked by a currently executing reaction because
 * the first reaction is blocked).
 *
 * The upper 48 bits of the index are the deadline and the
 * lower 16 bits denote a level in the precedence graph.  Reactions that
 * do not depend on any upstream reactions have level 0, and greater values
 * indicate the length of the longest upstream path to a reaction with level 0.
 * If a reaction has no specified deadline and is not upstream of any reaction
 * with a specified deadline, then its deadline is the largest 48 bit number.
 * Also, all reactions that precede a reaction r that has a deadline D
 * are are assigned a deadline D' <= D.
 *
 * A reaction r is blocked by an executing reaction e if e has a lower level
 * and the chain ID of e overlaps (shares at least one bit) with the chain ID
 * of r. If the two chain IDs share no bits, then we are assured that e is not
 * upstream of r and hence cannot block r.
 *
 * This function assumes the mutex is held.
 *
 * @return the first-ranked reaction that is ready to execute, NULL if there is
 * none.
 */ 
int distribute_ready_reactions() {    
    reaction_t* r;
    reaction_t* b;
    // Keep track of the chain IDs of blocked reactions.
    unsigned long long mask = 0LL;

    int reactions_distributed = 0;

    // Find a reaction that is ready to execute.
    while ((r = (reaction_t*)pqueue_pop(reaction_q)) != NULL) {
        // Set the reaction aside if it is blocked, either by another
        // blocked reaction or by a reaction that is currently executing.
        if (OVERLAPPING(mask, r->chain_id)) {
            pqueue_insert(transfer_q, r);
            DEBUG_PRINT("Reaction %s is blocked by a reaction that is also blocked.", r->name);
        } else {
            if (_lf_sched_is_blocked_by_executing_reaction(r)) {
                pqueue_insert(transfer_q, r);
            } else {
                reactions_distributed++;
                _lf_sched_distribute_ready_reaction(r);
                continue;
            }
        }
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

    return reactions_distributed;
}

/**
 * Advance tag. This will also pop events for the newly acquired tag and put
 * the triggered reactions on the reaction queue.
 * This function assumes the caller holds the mutex lock.
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
    //tracepoint_worker_advancing_time_starts(worker_number);
    _lf_next_locked();

    DEBUG_PRINT("Scheduler: Done waiting for _lf_next_locked().");
    return false;
}

void _lf_sched_cleanup_done_reactions_locked() {
    for (int i=0; i< _lf_sched_number_of_workers; i++) {
        reaction_t* reaction_to_remove = NULL;
        lf_mutex_lock(&_lf_sched_threads_info[i].mutex);
        while(
            (reaction_to_remove = 
            (reaction_t*)pqueue_pop(_lf_sched_threads_info[i].done_reactions))
            != NULL) {
            DEBUG_PRINT(
                "Scheduler: Removing reaction %s from executing queue.",
                reaction_to_remove->name
            );
            if (pqueue_remove(executing_q, reaction_to_remove) != 0) {
                error_print_and_exit("Could not properly clear the executing queue.");
            }
        }
        lf_mutex_unlock(&_lf_sched_threads_info[i].mutex);
    }
}

/**
 * Advance tag if there are no reactions in the reaction queue or in progress. If
 * there are such reactions or if another thread is already advancing the tag, wait
 * until something on the reaction queue is changed.
 * This function assumes the caller holds the mutex lock.
 * 
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_try_advance_tag_and_distribute() {
    bool return_value = false;

    _lf_sched_cleanup_done_reactions_locked();

    if (pqueue_size(reaction_q) == 0
            && pqueue_size(executing_q) == 0) {
        // Nothing more happening at this logical time.
        DEBUG_PRINT("Scheduler: Advancing time.");
        // This thread will take charge of advancing time.
        if (_lf_sched_advance_tag_locked()) {
            DEBUG_PRINT("Scheduler: Reached stop tag.");
            return_value = true;
        }
    }
    
    if (pqueue_size(reaction_q) > 0) {
        DEBUG_PRINT("Scheduler: Reaction queue is not empty. Distributing work.");
        distribute_ready_reactions();
    }
    DEBUG_PRINT("Scheduler: Executing queue size is %u.", pqueue_size(executing_q));
    return return_value;
}

void _lf_sched_wait_for_threads_asking_for_more_work() {
    // If there is more work to be handed out wait for threads to ask for more
    // work
    if (pqueue_size(reaction_q) == 0 && pqueue_size(executing_q) == 0) {
        // No work to be handed out
        return;
    }
    DEBUG_PRINT("Scheduler: Waiting for threads to ask for more work");
    // lf_cond_timedwait(&_lf_sched_need_more_work, &mutex, MSEC(10));
    lf_cond_wait(&_lf_sched_need_more_work, &mutex);
}

void _lf_sched_signal_stop() {
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        lf_mutex_lock(&_lf_sched_threads_info[i].mutex);
        _lf_sched_threads_info[i].should_stop = true;
        lf_cond_signal(&_lf_sched_threads_info[i].cond);
        lf_mutex_unlock(&_lf_sched_threads_info[i].mutex);
    }
}

void* _lf_sched_do_scheduling() {
    lf_mutex_lock(&mutex);
    while(!_lf_sched_try_advance_tag_and_distribute()) {
        _lf_sched_wait_for_threads_asking_for_more_work();
    }
    lf_mutex_unlock(&mutex);
    _lf_sched_signal_stop();
    return NULL;
}