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
 * Global Earliest Deadline First (GEDF) non-preemptive scheduler with chain ID
 * for the threaded runtime of the C target of Lingua Franca.
 *
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#include "lf_types.h"
#if SCHEDULER == GEDF_NP_CI
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include <assert.h>

#include "platform.h"
#include "pqueue.h"
#include "reactor.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "semaphore.h"
#include "trace.h"
#include "util.h"
#include "vector.h"

#ifndef MAX_REACTION_LEVEL
#define MAX_REACTION_LEVEL INITIAL_REACT_QUEUE_SIZE
#endif

/////////////////// Scheduler Variables and Structs /////////////////////////

/**
 * @brief Information about one worker thread.
 */

typedef struct custom_scheduler_data_t {
    pqueue_t** output_reactions;  // Reactions produced by the worker after
                                 // executing a reaction. The worker thread does
                                 // not need to acquire any mutex lock to read
                                 // this and the scheduler does not need to
                                 // acquire any mutex lock to write to this as
                                // long as the worker thread is idle.
    int output_reactions_size;
} custom_scheduler_data_t;


/////////////////// Scheduler Worker API (private) /////////////////////////
/**
 * @brief Distribute 'ready_reaction' to the best idle thread.
 *
 * This assumes that the caller is holding 'mutex'.
 *
 * @param ready_reaction A reaction that is ready to execute.
 */
static inline void _lf_sched_distribute_ready_reaction_locked(
    lf_scheduler_t* scheduler,
    reaction_t* ready_reaction) {
    LF_PRINT_DEBUG("Scheduler: Trying to distribute reaction %s.",
                ready_reaction->name);
    ready_reaction->status = running;
    if (pqueue_insert(
            (pqueue_t*)scheduler->executing_reactions,
            ready_reaction) != 0) {
        lf_print_error_and_exit("Could not add reaction to the executing queue.");
    }
}

/**
 * Return true if the first reaction has precedence over the second, false
 * otherwise.
 * @param r1 The first reaction.
 * @param r2 The second reaction.
 */
bool _lf_has_precedence_over(reaction_t* r1, reaction_t* r2) {
    if (LF_LEVEL(r1->index) < LF_LEVEL(r2->index) &&
        OVERLAPPING(r1->chain_id, r2->chain_id)) {
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
 * the scheduler->executing_reactions.
 * @param reaction The reaction.
 * @return true if this reaction is blocked, false otherwise.
 */
bool _lf_is_blocked_by_executing_or_blocked_reaction(lf_scheduler_t* scheduler, reaction_t* reaction) {
    if (reaction == NULL) {
        return false;
    }
    // The head of the scheduler->executing_reactions has the
    // lowest level of anything on the queue, and that level is also lower than
    // anything on the scheduler->transfer_reactions (because
    // reactions on the transfer queue are blocked by reactions on the
    // scheduler->executing_reactions). Hence, if the candidate
    // reaction has a level less than or equal to that of the head of the
    // scheduler->executing_reactions, then it is executable
    // and we don't need to check the contents of either queue further.
    if (pqueue_size(
            (pqueue_t*)scheduler->executing_reactions) > 0 &&
        reaction->index <=
            ((reaction_t*)pqueue_peek(
                 (pqueue_t*)scheduler->executing_reactions))
                ->index) {
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
    for (size_t i = 1;
         i < ((pqueue_t*)scheduler->executing_reactions)->size;
         i++) {
        reaction_t* running =
            (reaction_t*)((pqueue_t*)
                              scheduler->executing_reactions)
                ->d[i];
        if (_lf_has_precedence_over(running, reaction)) {
            LF_PRINT_DEBUG("Reaction %s is blocked by executing reaction %s.",
                        reaction->name, running->name);
            return true;
        }
    }
    for (size_t i = 1;
         i < ((pqueue_t*)scheduler->transfer_reactions)->size;
         i++) {
        reaction_t* blocked =
            (reaction_t*)((pqueue_t*)
                              scheduler->transfer_reactions)
                ->d[i];
        if (_lf_has_precedence_over(blocked, reaction)) {
            LF_PRINT_DEBUG("Reaction %s is blocked by blocked reaction %s.",
                        reaction->name, blocked->name);
            return true;
        }
    }

    // printf("Not blocking for reaction with chainID %llu and level %llu\n",
    // reaction->chain_id, reaction->index);
    // pqueue_dump(scheduler->executing_reactions, stdout,
    // scheduler->executing_reactions->prt);
    return false;
}

/**
 * @brief Distribute any reaction that is ready to execute to idle worker
 * thread(s).
 *
 * This assumes that the caller is holding 'mutex' and is not holding any thread
 * mutexes.
 *
 * @return Number of reactions that were successfully distributed to worker
 * threads.
 */
int _lf_sched_distribute_ready_reactions_locked(lf_scheduler_t* scheduler) {
    reaction_t* r;

    // Keep track of the number of reactions distributed
    int reactions_distributed = 0;

    // Find a reaction that is ready to execute.
    while ((r = (reaction_t*)pqueue_pop(
                (pqueue_t*)scheduler->triggered_reactions)) !=
           NULL) {
        // Set the reaction aside if it is blocked, either by another
        // blocked reaction or by a reaction that is currently executing.
        if (!_lf_is_blocked_by_executing_or_blocked_reaction(scheduler, r)) {
            _lf_sched_distribute_ready_reaction_locked(scheduler, r);
            reactions_distributed++;
            continue;
        }
        // Couldn't execute the reaction. Will have to put it back in the
        // reaction queue.
        pqueue_insert((pqueue_t*)scheduler->transfer_reactions,
                      (void*)r);
    }

    // Push blocked reactions back onto the reaction queue.
    // This will swap the two queues if the
    // scheduler->transfer_reactions has gotten larger than the
    // scheduler->triggered_reactions.
    pqueue_empty_into(
        (pqueue_t**)&scheduler->triggered_reactions,
        (pqueue_t**)&scheduler->transfer_reactions);

    LF_PRINT_DEBUG("Scheduler: Distributed %d reactions.", reactions_distributed);
    return reactions_distributed;
}

/**
 * @brief If there is work to be done, notify workers individually.
 *
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers(lf_scheduler_t* scheduler) {
    size_t workers_to_awaken =
        LF_MIN(scheduler->number_of_idle_workers,
            pqueue_size(
                (pqueue_t*)scheduler->executing_reactions));
    LF_PRINT_DEBUG("Notifying %zu workers.", workers_to_awaken);
    lf_atomic_fetch_add(&scheduler->number_of_idle_workers,
                        -1 * workers_to_awaken);
    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this
        // function.
        lf_semaphore_release(scheduler->semaphore,
                             (workers_to_awaken - 1));
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
bool _lf_scheduler_try_advance_tag_and_distribute(lf_scheduler_t* scheduler) {
    bool return_value = false;

    // Executing queue must be empty when this is called.
    assert(pqueue_size(
               (pqueue_t*)scheduler->executing_reactions) ==
           0);

    lf_mutex_lock(&scheduler->env->mutex);
    while (pqueue_size(
               (pqueue_t*)scheduler->triggered_reactions) ==
           0) {
        // Nothing more happening at this tag.
        LF_PRINT_DEBUG("Scheduler: Advancing tag.");
        // This worker thread will take charge of advancing tag.
        if (_lf_sched_advance_tag_locked(scheduler)) {
            LF_PRINT_DEBUG("Scheduler: Reached stop tag.");
            return_value = true;
            break;
        }
    }

    int reactions_distributed = _lf_sched_distribute_ready_reactions_locked(scheduler);
    lf_mutex_unlock(&scheduler->env->mutex);

    if (reactions_distributed) {
        _lf_sched_notify_workers(scheduler);
    }

    // pqueue_dump(scheduler->executing_reactions,
    // print_reaction);
    return return_value;
}

/**
 * @brief Signal all worker threads that it is time to stop.
 *
 */
void _lf_sched_signal_stop(lf_scheduler_t* scheduler) {
    scheduler->should_stop = true;
    lf_semaphore_release(scheduler->semaphore,
                         (scheduler->number_of_workers - 1));
}

/**
 * @brief Transfer the contents of the 'output_reactions' queue to the
 * 'scheduler->triggered_reactions'.
 *
 * This function assumes that the 'mutex' is not locked.
 *
 * @param worker_number The worker number of the calling worker thread.
 */
void _lf_sched_update_triggered_reactions(lf_scheduler_t* scheduler, size_t worker_number) {
    lf_mutex_lock(&scheduler->env->mutex);
    LF_PRINT_DEBUG("Scheduler: Emptying the output reaction queue of Worker %zu.",
                worker_number);
    pqueue_empty_into(
        (pqueue_t**)&scheduler->triggered_reactions,
        &scheduler->custom_data->output_reactions[worker_number]);
    lf_mutex_unlock(&scheduler->env->mutex);
}

/**
 * @brief Wait until the scheduler assigns work.
 *
 * If the calling worker thread is the last to become idle, it will call on the
 * scheduler to distribute work. Otherwise, it will wait on
 * 'scheduler->semaphore'.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
void _lf_sched_wait_for_work(lf_scheduler_t* scheduler, size_t worker_number) {
    // First, empty the 'output_reactions' for this worker thread into the
    // 'scheduler->triggered_reactions'.
    _lf_sched_update_triggered_reactions(scheduler, worker_number);

    // Second, increment the number of idle workers by 1 and
    // check if this is the last worker thread to become idle.
    if (lf_atomic_add_fetch(&scheduler->number_of_idle_workers,
                            1) ==
        scheduler->number_of_workers) {
        // Last thread to go idle
        LF_PRINT_DEBUG("Scheduler: Worker %zu is the last idle thread.",
                    worker_number);
        // Call on the scheduler to distribute work or advance tag.
        if (_lf_scheduler_try_advance_tag_and_distribute(scheduler)) {
            // It's time to stop
            _lf_sched_signal_stop(scheduler);
        }
    } else {
        // Not the last thread to become idle.
        // Wait for work to be released.
        lf_semaphore_acquire(scheduler->semaphore);
    }
}

///////////////////// Scheduler Init and Destroy API /////////////////////////
/**
 * @brief Initialize the scheduler.
 *
 * This has to be called before other functions of the scheduler can be used.
 * If the scheduler is already initialized, this will be a no-op.
 *
 * @param env Environment within which we are executing.
 * @param number_of_workers Indicate how many workers this scheduler will be
 *  managing.
 * @param option Pointer to a `sched_params_t` struct containing additional
 *  scheduler parameters.
 */
void lf_sched_init(
    environment_t* env,
    size_t number_of_workers,
    sched_params_t* params
) {
    assert(env != GLOBAL_ENVIRONMENT);

    LF_PRINT_DEBUG("Scheduler: Initializing with %zu workers", number_of_workers);
    if(!init_sched_instance(env, &env->scheduler, number_of_workers, params)) {
        // Already initialized
        return;
    }

    size_t queue_size = INITIAL_REACT_QUEUE_SIZE;
    if (params != NULL) {
        if (params->num_reactions_per_level != NULL) {
            // Recalculate the queue size
            queue_size = 0;
            for (size_t i = 0; i <= env->scheduler->max_reaction_level; i++) {
                queue_size += params->num_reactions_per_level[i];
            }
        }
    }
    LF_PRINT_DEBUG("Scheduler: Adopting a queue size of %zu.", queue_size);

    env->scheduler->array_of_mutexes =
        (lf_mutex_t*)calloc(1, sizeof(lf_mutex_t));
    lf_mutex_init(&env->scheduler->array_of_mutexes[0]);

    // Reaction queue ordered first by deadline, then by level.
    // The index of the reaction holds the deadline in the 48 most significant
    // bits, the level in the 16 least significant bits.
    env->scheduler->triggered_reactions = pqueue_init(
        queue_size, in_reverse_order, get_reaction_index, get_reaction_position,
        set_reaction_position, reaction_matches, print_reaction);
    env->scheduler->transfer_reactions = pqueue_init(
        queue_size, in_reverse_order, get_reaction_index, get_reaction_position,
        set_reaction_position, reaction_matches, print_reaction);
    // Create a queue on which to put reactions that are currently executing.
    env->scheduler->executing_reactions = pqueue_init(
        queue_size, in_reverse_order, get_reaction_index, get_reaction_position,
        set_reaction_position, reaction_matches, print_reaction);

    // Create the custom scheduler data struct. It contains a pqueue for each worker thread
    env->scheduler->custom_data = (custom_scheduler_data_t*)calloc(1, sizeof(custom_scheduler_data_t));
    env->scheduler->custom_data->output_reactions_size = number_of_workers;
    env->scheduler->custom_data->output_reactions = (pqueue_t **) calloc(number_of_workers, sizeof(pqueue_t*));
    for (int i = 0; i < env->scheduler->number_of_workers; i++) {
        env->scheduler->custom_data->output_reactions[i] = pqueue_init(
            queue_size, in_reverse_order, get_reaction_index,
            get_reaction_position, set_reaction_position, reaction_matches,
            print_reaction);
    }
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free(lf_scheduler_t* scheduler) {
    for (int i = 0; i < scheduler->number_of_workers; i++) {
        pqueue_free(scheduler->custom_data->output_reactions[i]);
    }
    free(scheduler->custom_data->output_reactions);
    free(scheduler->custom_data);
    // pqueue_free(scheduler->triggered_reactions); FIXME: This
    // might be causing weird memory errors
    pqueue_free((pqueue_t*)scheduler->transfer_reactions);
    pqueue_free((pqueue_t*)scheduler->executing_reactions);
    lf_semaphore_destroy(scheduler->semaphore);
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
reaction_t* lf_sched_get_ready_reaction(lf_scheduler_t* scheduler, int worker_number) {
    // Iterate until the stop_tag is reached or reaction queue is empty
    while (!scheduler->should_stop) {
        lf_mutex_lock(&scheduler->array_of_mutexes[0]);
        reaction_t* reaction_to_return = (reaction_t*)pqueue_pop(
            (pqueue_t*)scheduler->executing_reactions);
        lf_mutex_unlock(&scheduler->array_of_mutexes[0]);

        if (reaction_to_return != NULL) {
            // Got a reaction
            return reaction_to_return;
        }

        LF_PRINT_DEBUG("Worker %d is out of ready reactions.", worker_number);

        // Ask the scheduler for more work and wait
        tracepoint_worker_wait_starts(scheduler->env->trace, worker_number);
        _lf_sched_wait_for_work(scheduler, worker_number);
        tracepoint_worker_wait_ends(scheduler->env->trace, worker_number);
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
void lf_sched_done_with_reaction(size_t worker_number,
                                 reaction_t* done_reaction) {
    if (!lf_bool_compare_and_swap(&done_reaction->status, running, inactive)) {
        lf_print_error_and_exit("Unexpected reaction status: %d. Expected %d.",
                             done_reaction->status, running);
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
 * @param worker_number The ID of the worker that is making this call. 0 should
 * be used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 */
void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number) {
    if (reaction == NULL || !lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        return;
    }
    LF_PRINT_DEBUG("Scheduler: Enqueing reaction %s, which has level %lld.",
            reaction->name, LF_LEVEL(reaction->index));
    if (worker_number == -1) {
        lf_mutex_lock(&scheduler->env->mutex);
        // Immediately put 'reaction' on the reaction queue.
        pqueue_insert(
            (pqueue_t*)scheduler->triggered_reactions,
            (void*)reaction);
        lf_mutex_unlock(&scheduler->env->mutex);
    } else {
        reaction->worker_affinity = worker_number;
        // Note: The scheduler has already checked that we are not enqueueing
        // this reaction twice.
        pqueue_insert(scheduler->custom_data->output_reactions[worker_number],
                      (void*)reaction);
    }
}
#endif
