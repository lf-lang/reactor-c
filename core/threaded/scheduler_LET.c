// Logical Execution Time scheduler
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
 * Non-preemptive scheduler for the threaded runtime of the C target of Lingua
 * Franca.
 * 
 * @author{Erling R. Jellum <erling.r.jellum@ntnu.no>}
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include <assert.h>

#include "../platform.h"
#include "../utils/semaphore.h"
#include "reactor.h"
#include "scheduler.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.c"


#include "modal_models/modes.h"
/////////////////// External Variables /////////////////////////
extern lf_mutex_t mutex;
extern lf_cond_t event_q_changed;

/////////////////// Scheduler Variables and Structs /////////////////////////
_lf_sched_instance_t* _lf_sched_instance;
bool* _lf_sched_worker_is_in_workforce;
#ifdef MODAL_REACTORS
static vector_t _lf_sched_transitioning_reactors;
#define LF_SCHED_INTIAL_CAPACITY_TRANS_REACTORS 16
#endif

/////////////////// Scheduler Private API /////////////////////////
static void _lf_sched_mode_time_advance_prologue();
static void _lf_sched_mode_time_advance_epilogue();

/**
 * @brief Insert 'reaction' into
 * _lf_sched_instance->_lf_sched_triggered_reactions at the appropriate level.
 *
 * @param reaction The reaction to insert.
 */
static inline void _lf_sched_insert_reaction(reaction_t* reaction) {
    size_t reaction_level = LF_LEVEL(reaction->index);
#ifdef FEDERATED
    // Lock the mutex if federated because a federate can insert reactions with
    // a level equal to the current level.
    size_t current_level = _lf_sched_instance->_lf_sched_next_reaction_level - 1;
    // There is a race condition here where
    // `_lf_sched_instance->_lf_sched_next_reaction_level` can change after it is
    // cached here. In that case, if the cached value is equal to
    // `reaction_level`, the cost will be an additional unnecessary mutex lock,
    // but no logic error. If the cached value is not equal to `reaction_level`,
    // it can never become `reaction_level` because the scheduler will only
    // change the `_lf_sched_instance->_lf_sched_next_reaction_level` if it can
    // ensure that all worker threads are idle, and thus, none are triggering
    // reactions (and therefore calling this function).
    if (reaction_level == current_level) {
        LF_PRINT_DEBUG("Scheduler: Trying to lock the mutex for level %zu.",
                    reaction_level);
        lf_mutex_lock(
            &_lf_sched_instance->_lf_sched_array_of_mutexes[reaction_level]);
        LF_PRINT_DEBUG("Scheduler: Locked the mutex for level %zu.", reaction_level);
    }
    // The level index for the current level can sometimes become negative. Set
    // it back to zero before adding a reaction (otherwise worker threads will
    // not be able to see the added reaction).
    if (_lf_sched_instance->_lf_sched_indexes[reaction_level] < 0) {
        _lf_sched_instance->_lf_sched_indexes[reaction_level] = 0;
    }
#endif
    int reaction_q_level_index =
        lf_atomic_fetch_add(&_lf_sched_instance->_lf_sched_indexes[reaction_level], 1);
    assert(reaction_q_level_index >= 0);
    LF_PRINT_DEBUG(
        "Scheduler: Accessing triggered reactions at the level %zu with index %d.",
        reaction_level, 
        reaction_q_level_index
    );
    ((reaction_t***)_lf_sched_instance->_lf_sched_triggered_reactions)[reaction_level][reaction_q_level_index] = reaction;
    LF_PRINT_DEBUG("Scheduler: Index for level %zu is at %d.", reaction_level,
                reaction_q_level_index);
#ifdef FEDERATED
    if (reaction_level == current_level) {
        lf_mutex_unlock(
            &_lf_sched_instance->_lf_sched_array_of_mutexes[reaction_level]);
    }
#endif
}



/**
 * @brief Distribute any reaction that is ready to execute to idle worker
 * thread(s).
 *
 * @return 1 if any reaction is ready. 0 otherwise.
 */
int _lf_sched_distribute_ready_reactions() {
    // Note: All the threads are idle, which means that they are done inserting
    // reactions. Therefore, the reaction vectors can be accessed without
    // locking a mutex.
    for (; _lf_sched_instance->_lf_sched_next_reaction_level <=
           _lf_sched_instance->max_reaction_level;
         _lf_sched_instance->_lf_sched_next_reaction_level++
    ) {

        _lf_sched_instance->_lf_sched_executing_reactions =
            (void*)((reaction_t***)_lf_sched_instance->_lf_sched_triggered_reactions)[
                _lf_sched_instance->_lf_sched_next_reaction_level
            ];
            
        if (((reaction_t**)_lf_sched_instance->_lf_sched_executing_reactions)[0] != NULL) {
            // There is at least one reaction to execute
            _lf_sched_instance->_lf_sched_next_reaction_level++;
            return 1;
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
    // Calculate the number of workers that we need to wake up, which is the
    // Need mutex because LET worker also accesses this variable
    lf_mutex_lock(&mutex);
    size_t workers_to_awaken =
        LF_MIN(_lf_sched_instance->_lf_sched_number_of_idle_workers,
            _lf_sched_instance->_lf_sched_indexes[
                _lf_sched_instance->_lf_sched_next_reaction_level - 1 // Current 
                                                                      // reaction
                                                                      // level
                                                                      // to execute.                                                         
            ]);
    LF_PRINT_DEBUG("Scheduler: Notifying %zu workers.", workers_to_awaken);

    _lf_sched_instance->_lf_sched_number_of_idle_workers -= workers_to_awaken;
    LF_PRINT_DEBUG("Scheduler: New number of idle workers: %zu.",
                _lf_sched_instance->_lf_sched_number_of_idle_workers);

    lf_mutex_unlock(&mutex);

    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this
        // function.
        lf_semaphore_release(_lf_sched_instance->_lf_sched_semaphore,
                             (workers_to_awaken - 1));
    }
}

/**
 * @brief Signal all worker threads that it is time to stop.
 *
 */
void _lf_sched_signal_stop() {
    _lf_sched_instance->_lf_sched_should_stop = true;
    lf_semaphore_release(_lf_sched_instance->_lf_sched_semaphore,
                         (_lf_sched_instance->_lf_sched_number_of_workers - 1));
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
    _lf_sched_instance
        ->_lf_sched_indexes[_lf_sched_instance->_lf_sched_next_reaction_level -
                            1] = 0;

    // Loop until it's time to stop or work has been distributed
    while (true) {
        if (_lf_sched_instance->_lf_sched_next_reaction_level ==
            (_lf_sched_instance->max_reaction_level + 1)) {
            _lf_sched_instance->_lf_sched_next_reaction_level = 0;
            
            #ifdef MODAL_REACTORS
                _lf_sched_mode_time_advance_prologue();
            #endif
            
            lf_mutex_lock(&mutex);
            // Nothing more happening at this tag.
            
            LF_PRINT_DEBUG("Scheduler: Trying to advance tag.");
            // This worker thread will take charge of advancing tag.
            if (_lf_sched_advance_tag_locked()) {
                LF_PRINT_DEBUG("Scheduler: Reached stop tag.");
                _lf_sched_signal_stop();
                lf_mutex_unlock(&mutex);
            
                #ifdef MODAL_REACTORS
                    _lf_sched_mode_time_advance_epilogue();
                #endif
            
                break;
            }
            lf_mutex_unlock(&mutex);
            
            #ifdef MODAL_REACTORS
                _lf_sched_mode_time_advance_epilogue();
            #endif
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
 * scheduler to distribute work. Otherwise, it will wait on
 * '_lf_sched_instance->_lf_sched_semaphore'.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
void _lf_sched_wait_for_work(size_t worker_number) {
    // Increment the number of idle workers by 1 and check if this is the last
    // worker thread to become idle.

    
    lf_mutex_lock(&mutex);
    _lf_sched_instance->_lf_sched_number_of_idle_workers+=1;
    if (_lf_sched_instance->_lf_sched_number_of_idle_workers == _lf_sched_instance->_lf_sched_number_of_workers) {
        lf_mutex_unlock(&mutex);
        // Last thread to go idle
        LF_PRINT_DEBUG("Scheduler: Worker %zu is the last idle thread.",
                    worker_number);
        // Call on the scheduler to distribute work or advance tag.
        _lf_sched_try_advance_tag_and_distribute();
    } else {
        // Not the last thread to become idle. Wait for work to be released.
        LF_PRINT_DEBUG(
            "Scheduler: Worker %zu is trying to acquire the scheduling "
            "semaphore.",
            worker_number);
        lf_mutex_unlock(&mutex);
        lf_semaphore_acquire(_lf_sched_instance->_lf_sched_semaphore);
        LF_PRINT_DEBUG("Scheduler: Worker %zu acquired the scheduling semaphore.",
                    worker_number);
    }
}

///////////////////// Scheduler Init and Destroy API /////////////////////////
/**
 * @brief Initialize the scheduler.
 *
 * This has to be called before other functions of the scheduler can be used.
 * If the scheduler is already initialized, this will be a no-op.
 *
 * @param number_of_workers Indicate how many workers this scheduler will be
 *  managing.
 * @param option Pointer to a `sched_params_t` struct containing additional
 *  scheduler parameters.
 */
void lf_sched_init(
    size_t number_of_workers, 
    sched_params_t* params
) {
    LF_PRINT_DEBUG("Scheduler: Initializing with %zu workers", number_of_workers);

    // This scheduler is unique in that it requires `num_reactions_per_level` to
    // work correctly.
    if (init_sched_instance(&_lf_sched_instance, number_of_workers, params)) {
        // Scheduler has not been initialized before.
        if (params == NULL || params->num_reactions_per_level == NULL) {
            lf_print_error_and_exit(
                "Scheduler: Internal error. The NP scheduler "
                "requires params.num_reactions_per_level to be set.");
        }
    } else {
        // Already initialized
        return;
    }

    LF_PRINT_DEBUG("Scheduler: Max reaction level: %zu", _lf_sched_instance->max_reaction_level);

    _lf_sched_instance->_lf_sched_triggered_reactions =
        calloc((_lf_sched_instance->max_reaction_level + 1), sizeof(reaction_t**));

    _lf_sched_instance->_lf_sched_array_of_mutexes = (lf_mutex_t*)calloc(
        (_lf_sched_instance->max_reaction_level + 1), sizeof(lf_mutex_t));

    _lf_sched_instance->_lf_sched_indexes = (volatile int*)calloc(
        (_lf_sched_instance->max_reaction_level + 1), sizeof(volatile int));

    size_t queue_size = INITIAL_REACT_QUEUE_SIZE;
    for (size_t i = 0; i <= _lf_sched_instance->max_reaction_level; i++) {
        if (params != NULL) {
            if (params->num_reactions_per_level != NULL) {
                queue_size = params->num_reactions_per_level[i];
            }
        }
        // Initialize the reaction vectors
        ((reaction_t***)_lf_sched_instance->_lf_sched_triggered_reactions)[i] =
            (reaction_t**)calloc(queue_size, sizeof(reaction_t*));
        
        LF_PRINT_DEBUG(
            "Scheduler: Initialized vector of reactions for level %zu with size %zu",
            i,
            queue_size
        );
        
        // Initialize the mutexes for the reaction vectors
        lf_mutex_init(&_lf_sched_instance->_lf_sched_array_of_mutexes[i]);
    }

    // Allocate array to hold information about what workers are in the workforce
    _lf_sched_worker_is_in_workforce = (bool *) malloc(number_of_workers * sizeof(bool));
    for (int i = 0; i< number_of_workers; i++) {
        _lf_sched_worker_is_in_workforce[i] = true;
    }

    #ifdef MODAL_REACTORS
    // Allocate array to hold information about which local mutexes were acquired
    //  due to mode transitions
    _lf_sched_transitioning_reactors = vector_new(LF_SCHED_INTIAL_CAPACITY_TRANS_REACTORS);
    #endif

    _lf_sched_instance->_lf_sched_executing_reactions = 
        (void*)((reaction_t***)_lf_sched_instance->
            _lf_sched_triggered_reactions)[0];
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    free(_lf_sched_instance->_lf_sched_triggered_reactions);
    free(_lf_sched_instance->_lf_sched_executing_reactions);
    lf_semaphore_destroy(_lf_sched_instance->_lf_sched_semaphore);
    free(_lf_sched_worker_is_in_workforce);
    #ifdef MODAL_REACTORS
    vector_free(&_lf_sched_transitioning_reactors);
    #endif
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
    while (!_lf_sched_instance->_lf_sched_should_stop) {
        // Coordinate rejoining the workforce. 
        // FIXME: Allow worker to join on the CURRENT level instead of waiting for next
        if (!_lf_sched_worker_is_in_workforce[worker_number]) {
            _lf_sched_worker_is_in_workforce[worker_number] = true;
            LF_PRINT_DEBUG("Worker %d goes to sleep until next level", worker_number);
            lf_semaphore_acquire(_lf_sched_instance->_lf_sched_semaphore);
            LF_PRINT_DEBUG("Worker %d has awoken. Back to work", worker_number);
            continue;
        }    
        // Calculate the current level of reactions to execute
        size_t current_level =
            _lf_sched_instance->_lf_sched_next_reaction_level - 1;
        reaction_t* reaction_to_return = NULL;
#ifdef FEDERATED
        // Need to lock the mutex because federate.c could trigger reactions at
        // the current level (if there is a causality loop)
        lf_mutex_lock(
            &_lf_sched_instance->_lf_sched_array_of_mutexes[current_level]);
#endif
        int current_level_q_index = lf_atomic_add_fetch(
            &_lf_sched_instance->_lf_sched_indexes[current_level], -1);
        if (current_level_q_index >= 0) {
            LF_PRINT_DEBUG(
                "Scheduler: Worker %d popping reaction with level %zu, index "
                "for level: %d.",
                worker_number, current_level, current_level_q_index
            );
            reaction_to_return = 
                ((reaction_t**)_lf_sched_instance->
                    _lf_sched_executing_reactions)[current_level_q_index];
            ((reaction_t**)_lf_sched_instance->
                    _lf_sched_executing_reactions)[current_level_q_index] = NULL;
        }
#ifdef FEDERATED
        lf_mutex_unlock(
            &_lf_sched_instance->_lf_sched_array_of_mutexes[current_level]);
#endif

        if (reaction_to_return != NULL) {
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

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 *
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number,
                                 reaction_t* done_reaction) {
    // This is performed in the reaction epilogue
    // if (!lf_bool_compare_and_swap(&done_reaction->status, queued, inactive)) {
    //     lf_print_error_and_exit("Unexpected reaction status: %d. Expected %d.",
    //                          done_reaction->status, queued);
    // }
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 *
 * If a worker number is not available (e.g., this function is not called by a
 * worker thread), -1 should be passed as the 'worker_number'.
 * 
 * This scheduler ignores the worker number.
 *
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 *
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should
 *  be used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 *
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    if (reaction == NULL || !lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        return;
    }
    LF_PRINT_DEBUG("Scheduler: Enqueing reaction %s, which has level %lld.",
            reaction->name, LF_LEVEL(reaction->index));
    _lf_sched_insert_reaction(reaction);
}

#ifdef MODAL_REACTORS
/**
 * @brief This function takes a self pointer and locks, recursively,
 *  all the parent reactors which are modal. It effectively walks
 *  up the containment hierarchy and locks any modal reactor.
 * 
 * @param reactor 
 */
static void _lf_sched_lock_modal_parents(self_base_t* reactor, int worker_number) {
    self_base_t *parent = reactor->parent;
    // If has a parent/containing reactor
    if (parent) {
        // Check if it is modal
        if (parent->_lf__mode_state.current_mode) {
            if (!parent->has_mutex) {
                lf_print_error_and_exit("Runtime error. Modal reactor did not have a mutex");
            }
            LF_PRINT_DEBUG("Worker %i: Locking parent reactor %p", worker_number, parent);
            lf_mutex_lock(&parent->mutex);
            LF_PRINT_DEBUG("Worker %i: Locked parent reactor %p", worker_number, parent);
        }
        _lf_sched_lock_modal_parents(parent, worker_number);
    }
}
/**
 * @brief This function walks up the containment hierarchy and unlocks
 *  any modal reactor.
 * 
 * @param reactor 
 */
static void _lf_sched_unlock_modal_parents(self_base_t* reactor, int worker_number) {
    self_base_t *parent = reactor->parent;
    // If has a parent/containing reactor
    if (parent) {
        // Check if it is modal
        if (parent->_lf__mode_state.current_mode) {
            if (!parent->has_mutex) {
                lf_print_error_and_exit("Runtime error. Modal reactor did not have a mutex");
            }
            LF_PRINT_DEBUG("Worker %i: Unlocking parent reactor %p", worker_number, parent);
            lf_mutex_unlock(&parent->mutex);
        }
        _lf_sched_lock_modal_parents(parent, worker_number);
    }
}
#endif

void lf_sched_wait_for_reactor(self_base_t *reactor) {
    assert(reactor->has_mutex);
    lf_mutex_lock(&reactor->mutex);
    lf_mutex_unlock(&reactor->mutex);
}

void lf_sched_wait_for_reactor_locked(self_base_t *reactor) {
    assert(reactor->has_mutex);
    lf_mutex_unlock(&mutex);
    lf_mutex_lock(&reactor->mutex);
    lf_mutex_unlock(&reactor->mutex);
    lf_mutex_lock(&mutex);
}

static void _lf_sched_wait_on_downstream_let(reaction_t* reaction, int worker_number) {
    for (int i = 0; i<reaction->num_downstream_let_reactors; i++) {
        self_base_t* downstream = reaction->downstream_let_reactors[i];
        if (downstream->executing_reaction) {
            LF_PRINT_DEBUG("Worker %d waiting on mutex of downstream let reactor %p", worker_number, downstream);
            lf_sched_wait_for_reactor(downstream);
            LF_PRINT_DEBUG("Worker %d finished waiting on mutex of downstream let reactor %p", worker_number, downstream);
        }
    }
}

/**
 * @brief Lock mutexes needed to execute the specified reaction.
 * If the reactor containing the specified reaction has a local mutex, lock it.
 * Also, if the reaction has a logical execution time greater than zero, then
 * lock all modal reactors higher in the hierarchy and set a global logical time
 * barrier for the end time of the LET.
 * execution   
 * 
 * @param reaction 
 * @param worker_number 
 */
void lf_sched_reaction_prologue(reaction_t * reaction, int worker_number) {
    self_base_t *self = (self_base_t *) reaction->self;
    
    // Wait on any directly downstream LET reactors w
    _lf_sched_wait_on_downstream_let(reaction, worker_number);
    
    // Take local mutex
    if (self->has_mutex) {
        LF_PRINT_DEBUG("Worker %d tries to locks local mutex", worker_number);
        lf_mutex_lock(&self->mutex);
        LF_PRINT_DEBUG("Worker %d locked local mutex", worker_number);
    }

    
    // If LET reaction, lock any modal parent, increment global tag barrier and remove worker from workforce
    if (reaction->let > 0) {
        // Lock any containing reactor which is modal to avoid mode changes while LET is executing
        #ifdef MODAL_REACTORS
            _lf_sched_lock_modal_parents(self, worker_number);
        #endif


        // Acquire the global mutex to: 1. Increment global barrier and 2. Update the scheduler variables.
        LF_PRINT_DEBUG("Worker %d tries to lock global mutex", worker_number);
        lf_mutex_lock(&mutex);
        LF_PRINT_DEBUG("Worker %d locked global mutex", worker_number);
        if (reaction->let < FOREVER) {
            LF_PRINT_DEBUG("Worker %d Increment global barrier", worker_number);
            tag_t finish_tag = {current_tag.time + reaction->let, 0UL};
            // FIXME: Use pqueue for tracking tag barrier. Only notify the cond_var when the head of the
            //  pqueue is removed.
            lf_increment_global_tag_barrier_locked(finish_tag);
        }

        // Remove worker from workforce. By both updating the local array holding workers that are in the workforce
        //  and decrementing the total number of workers. The first is needed for the rejoining process 
        _lf_sched_worker_is_in_workforce[worker_number] = false;
        _lf_sched_instance->_lf_sched_number_of_workers--;

        LF_PRINT_DEBUG("Worker %d removed from pool. %zu left", worker_number, _lf_sched_instance->_lf_sched_number_of_workers);
        // If all other workers are sleeping then wake one up to advance time
        if (_lf_sched_instance->_lf_sched_number_of_idle_workers == _lf_sched_instance->_lf_sched_number_of_workers) {
            LF_PRINT_DEBUG("Worker %d Wakes up 1 sleeping thread before going to LET work", worker_number);
            _lf_sched_instance->_lf_sched_number_of_idle_workers--;
            lf_semaphore_release(_lf_sched_instance->_lf_sched_semaphore, 1);
        }
        // Set the executing reaction field BEFORE releasing the global mutex
        //  After we release global mutex the runtime might advance time. And it might 
        //  preempt our current thread before it reaches it reaction invokation
        self->executing_reaction = reaction;

        // Release global mutex
        lf_mutex_unlock(&mutex);
    }
}

void lf_sched_reaction_epilogue(reaction_t * reaction, int worker_number) {
    self_base_t *self = (self_base_t *) reaction->self;
    // First set reaction status to inactive. This is done here, while holding the local mutex
    //  because when it is released, we might release a worker advancing time trying to trigger
    //  this very reaction for a future tag. We must be inactive when this worker is released.
    //  If not we could drop a future event.
    
    // FIXME: Can I do this here. Other schedulers do this in an atomic instruction later...
    reaction->status = inactive;
    // Unlock local mutex to allow interrupting reactions+mode changes
    if (self->has_mutex) {
        lf_mutex_unlock(&self->mutex);
        LF_PRINT_DEBUG("Worker %d unlocked local mutex", worker_number);
    }

    // Do half of the rejoining step. Increment num_workers and num_idle workers
    //  This will not have any effect on the operation of the other workers. They will atomically see num_workers
    //  and num_idle being incremented. This worker will rejoin the worker pool upon calling `lf_sched_get_ready_reaction`
    //  Note that the GLOBAL mutex is used to increment the scheduler varibles.
    if(reaction->let > 0) {
        lf_mutex_lock(&mutex);
        _lf_sched_instance->_lf_sched_number_of_workers+=1;
        _lf_sched_instance->_lf_sched_number_of_idle_workers+=1;
        // Reactions without any effects are assigned a LET of FOREVER, and did not setup any barrier
        if (reaction->let < FOREVER) {
            lf_decrement_global_tag_barrier_locked();
        }
        lf_mutex_unlock(&mutex);

        // unlock any containing reactor which is modal to enable mode changes in parents
        #ifdef MODAL_REACTORS
            _lf_sched_unlock_modal_parents(self, worker_number);
        #endif
    }
}

#ifdef MODAL_REACTORS
/**
 * @brief Acquire the local mutex for any reactor that has pending mode changes.
 * This function should be invoked by the worker thread about to advance time BEFORE it acquires the global mutex.
 *  It is only relevant if we have modal reactors. 
 * It gathers all the Reactors which are about to perform mode changes, and reactors contained within them,
 *  and acquires their local mutex to make sure there are no LET reactions currently executing in any of them.
 *  In the case that there are LET reactions, this will block the advancement of time until the LET reaction completes.
 */
static void _lf_sched_mode_time_advance_prologue() {
    int n_reactors = _lf_mode_get_transitioning_reactors((void *) &_lf_sched_transitioning_reactors);

    LF_PRINT_DEBUG("There are %u transitioning reactors whose locks we must acquire", n_reactors);
    for (int i = 0; i<n_reactors; i++) {
        self_base_t *reactor = (self_base_t *) (*vector_at(&_lf_sched_transitioning_reactors, i));
        LF_PRINT_DEBUG("Modes: lock mutex for %p", reactor); 
        lf_mutex_lock(&reactor->mutex);
        LF_PRINT_DEBUG("Modes: locked mutex for %p", reactor);
    }

}

static void _lf_sched_mode_time_advance_epilogue() {
    int n_reactors = vector_size(&_lf_sched_transitioning_reactors);
    LF_PRINT_DEBUG("Unlocking %u reactors", n_reactors);
    for (int i = 0; i<n_reactors; i++) {
        self_base_t *reactor = (self_base_t *) vector_pop(&_lf_sched_transitioning_reactors);
        LF_PRINT_DEBUG("Modes: unlocking mutex for %p", reactor);
        lf_mutex_unlock(&reactor->mutex);
    }
}



#endif
