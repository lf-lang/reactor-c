// Scheduler for the threaded version of the C target of Lingua Franca


#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "reactor.h"
#include "platform.h"
#include "pqueue.h"

extern pqueue_t* reaction_q;
extern pqueue_t* executing_q;
extern lf_cond_t reaction_q_changed;
extern lf_cond_t executing_q_emptied;
extern lf_mutex_t mutex;

typedef struct {
    lf_mutex_t mutex;
    lf_cond_t cond;
    pqueue_t* ready_reactions;
    pqueue_t* done_reactions;
} _lf_sched_thread_info_t;

_lf_sched_thread_info_t* _lf_sched_threads_info;

size_t _lf_sched_number_of_workers = 1;

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
                index_to_steal
            );
        }
        lf_mutex_unlock(&_lf_sched_threads_info[index_to_steal].mutex);
    }
    return reaction_to_return;
}

void _lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    pqueue_insert(_lf_sched_threads_info[worker_number].done_reactions, done_reaction);
}

size_t _lf_sched_balancing_index = 0;

static inline void _lf_sched_distribute_ready_reaction(size_t worker_number, reaction_t* ready_reaction) {
    DEBUG_PRINT("Worker %d: Trying to distribute reaction %s.", worker_number, ready_reaction->name);    
    lf_mutex_lock(&_lf_sched_threads_info[_lf_sched_balancing_index].mutex);
    DEBUG_PRINT(
        "Worker %d: Assigning reaction %s to worker %d.",
        worker_number,
        ready_reaction->name,
        _lf_sched_balancing_index
    );
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
 * Advance tag if there are no reactions in the reaction queue or in progress. If 
 * there are such reactions or if another thread is already advancing the tag, wait 
 * until something on the reaction queue is changed.
 * This function assumes the caller holds the mutex lock.
 * 
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_try_advance_tag_or_distributed(int worker_number) {
    while(true) {
        if (pqueue_size(reaction_q) == 0
                && pqueue_size(reaction_q) == 0) {
            DEBUG_PRINT("Worker %d: Trying to advance time.", worker_number);
            // Nothing more happening at this logical time.
            if (!_lf_advancing_time) {
                DEBUG_PRINT("Worker %d: Advancing time.", worker_number);
                // This thread will take charge of advancing time.
                if (_lf_sched_advance_tag_locked(worker_number)) {
                    DEBUG_PRINT("Worker %d: Reached stop tag.", worker_number);
                    return true;
                } else {
                    return false;
                }
            } else if (compare_tags(current_tag, stop_tag) >= 0) {
                // At the stop tag so we can exit the worker thread.
                DEBUG_PRINT("Worker %d: Reached stop tag.", worker_number);
                return true;
            }
        }

        if (distribute_ready_reactions(worker_number) != 0) {
            return false;
        }

        DEBUG_PRINT("Worker %d: Waiting for items on the reaction queue.", worker_number);
        tracepoint_worker_wait_starts(worker_number);

        lf_cond_wait(&reaction_q_changed, &mutex);
        tracepoint_worker_wait_ends(worker_number);
        DEBUG_PRINT("Worker %d: Scheduler done waiting.", worker_number);
    }
}

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
    while(!_lf_sched_instance->_lf_sched_should_stop) {
        lf_mutex_lock(&_lf_sched_instance->_lf_sched_array_of_mutexes[0]);
        reaction_t* reaction_to_return = (reaction_t*)pqueue_pop(
            (pqueue_t*)_lf_sched_instance->_lf_sched_executing_reactions);
        lf_mutex_unlock(&_lf_sched_instance->_lf_sched_array_of_mutexes[0]);

        if (reaction_to_return != NULL) {
            // Got a reaction
            return reaction_to_return;
        }

        LF_PRINT_DEBUG("Worker %d is out of ready reactions.", worker_number);

        // Ask the scheduler for more work and wait
        tracepoint_worker_wait_starts(worker_number);
        _lf_sched_wait_for_work(worker_number);
        tracepoint_worker_wait_ends(worker_number);
    }

    // It's time for the worker thread to stop and exit.
    return NULL;
}