/**
 * @file
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Global Earliest Deadline First (GEDF) non-preemptive scheduler for the
 * threaded runtime of the C target of Lingua Franca.
 */
#include "lf_types.h"

#if SCHEDULER == SCHED_GEDF_NP

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>

#include "low_level_platform.h"
#include "environment.h"
#include "pqueue.h"
#include "reactor_threaded.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "tracepoint.h"
#include "util.h"

// Data specific to the GEDF scheduler.
typedef struct custom_scheduler_data_t {
  pqueue_t* reaction_q;
  lf_cond_t reaction_q_changed;
  size_t current_level;
} custom_scheduler_data_t;

/////////////////// Scheduler Private API /////////////////////////


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
void lf_sched_init(environment_t* env, size_t number_of_workers, sched_params_t* params) {
  assert(env != GLOBAL_ENVIRONMENT);

  LF_PRINT_DEBUG("Scheduler: Initializing with %zu workers", number_of_workers);
  if (!init_sched_instance(env, &env->scheduler, number_of_workers, params)) {
    // Already initialized
    return;
  }
  lf_scheduler_t* scheduler = env->scheduler;

  // Just one reaction queue and mutex for each environment.
  scheduler->triggered_reactions = calloc(1, sizeof(pqueue_t*));
  scheduler->array_of_mutexes = (lf_mutex_t*)calloc(1, sizeof(lf_mutex_t));

  scheduler->custom_data = (custom_scheduler_data_t*)calloc(1, sizeof(custom_scheduler_data_t));

  // Initialize the reaction queue.
  size_t queue_size = INITIAL_REACT_QUEUE_SIZE;
  scheduler->custom_data->reaction_q =
      pqueue_init(queue_size, in_reverse_order, get_reaction_index, get_reaction_position, set_reaction_position,
                  reaction_matches, print_reaction);

  LF_COND_INIT(&scheduler->custom_data->reaction_q_changed, &env->mutex);

  scheduler->custom_data->current_level = 0;
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free(lf_scheduler_t* scheduler) {
  // for (size_t j = 0; j <= scheduler->max_reaction_level; j++) {
  //     pqueue_free(scheduler->triggered_reactions[j]);
  //     FIXME: This is causing weird memory errors.
  // }
  pqueue_free((pqueue_t*)scheduler->custom_data->reaction_q);
  free(scheduler->custom_data);
}

///////////////////// Scheduler Worker API (public) /////////////////////////
/**
 * @brief Ask the scheduler for one more reaction.
 *
 * This function blocks until it can return a ready reaction for worker thread
 * 'worker_number' or it is time for the worker thread to stop and exit (where a
 * NULL value would be returned).
 * 
 * This function assumes that the environment mutex is not locked.
 * @param scheduler The scheduler instance.
 * @param worker_number The worker number.
 * @return A reaction for the worker to execute. NULL if the calling worker thread should exit.
 */
reaction_t* lf_sched_get_ready_reaction(lf_scheduler_t* scheduler, int worker_number) {
  
  // Need to lock the environment mutex.
  LF_PRINT_DEBUG("Scheduler: Worker %d locking environment mutex.", worker_number);
  LF_MUTEX_LOCK(&scheduler->env->mutex);
  LF_PRINT_DEBUG("Scheduler: Worker %d locked environment mutex.", worker_number);

  // Iterate until the stop_tag is reached or the event queue is empty.
  while (!scheduler->should_stop) {
    reaction_t* reaction_to_return = (reaction_t*)pqueue_peek(scheduler->custom_data->reaction_q);
    if (reaction_to_return != NULL) {
      // Found a reaction.  Check the level.  Notice that because of deadlines, the current level
      // may advance to the maximum and then back down to 0. 
      if (LF_LEVEL(reaction_to_return->index) == scheduler->custom_data->current_level) {
        // Found a reaction at the current level.
        LF_PRINT_DEBUG("Scheduler: Worker %d found a reaction at level %zu.",
            worker_number, scheduler->custom_data->current_level);
        // Remove the reaction from the queue.
        pqueue_pop(scheduler->custom_data->reaction_q);
        LF_MUTEX_UNLOCK(&scheduler->env->mutex);
        return reaction_to_return;
      } else {
        // Found a reaction at a level other than the current level.
        LF_PRINT_DEBUG("Scheduler: Worker %d found a reaction at level %lld. Current level is %zu",
            worker_number, LF_LEVEL(reaction_to_return->index), scheduler->custom_data->current_level);
        // We need to wait to advance to the next level or get a new reaction at the current level.
        if (scheduler->number_of_idle_workers == scheduler->number_of_workers - 1) {
          // All other workers are idle.  Advance to the next level.
          try_advance_level(scheduler->env, &scheduler->custom_data->current_level);
          if (scheduler->custom_data->current_level > scheduler->max_reaction_level) {
            // Since the reaction queue is not empty, we must be cycling back to level 0 due to deadlines
            // having been given precedence over levels.  Reset the next level to 1.
            scheduler->custom_data->current_level = 0;
          }
          LF_PRINT_DEBUG("Scheduler: Advancing to next reaction level %zu.",
              scheduler->custom_data->current_level);
          // Notify other workers that we are at the next level.
          LF_COND_BROADCAST(&scheduler->custom_data->reaction_q_changed);
        } else {
          // Some workers are still working on reactions on the current level.
          // Wait for them to finish.
          scheduler->number_of_idle_workers++;
          tracepoint_worker_wait_starts(scheduler->env, worker_number);
          LF_COND_WAIT(&scheduler->custom_data->reaction_q_changed);
          tracepoint_worker_wait_ends(scheduler->env, worker_number);
          scheduler->number_of_idle_workers--;
        }
      }
    } else {
      // The reaction queue is empty.
      LF_PRINT_DEBUG("Worker %d finds nothing on the reaction queue.", worker_number);

      // If all other workers are idle, then we are done with this tag.
      if (scheduler->number_of_idle_workers == scheduler->number_of_workers - 1) {
        // Last thread to go idle
        LF_PRINT_DEBUG("Scheduler: Worker %d is advancing the tag.", worker_number);
        // Advance the tag.
        scheduler->custom_data->current_level = 0;
        if (_lf_sched_advance_tag_locked(scheduler)) {
          LF_PRINT_DEBUG("Scheduler: Reached stop tag.");
          scheduler->should_stop = true;
          LF_COND_BROADCAST(&scheduler->custom_data->reaction_q_changed);
          break;
        }
        try_advance_level(scheduler->env, &scheduler->custom_data->current_level);
        LF_COND_BROADCAST(&scheduler->custom_data->reaction_q_changed);
      } else {
        // Some other workers are still working on reactions on the current level.
        // Wait for them to finish.
        scheduler->number_of_idle_workers++;
        tracepoint_worker_wait_starts(scheduler->env, worker_number);
        LF_COND_WAIT(&scheduler->custom_data->reaction_q_changed);
        tracepoint_worker_wait_ends(scheduler->env, worker_number);
        scheduler->number_of_idle_workers--;
      }
    }
  }

  // It's time for the worker thread to stop and exit.
  LF_MUTEX_UNLOCK(&scheduler->env->mutex);
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
  (void)worker_number;
  if (!lf_atomic_bool_compare_and_swap32((int32_t*)&done_reaction->status, queued, inactive)) {
    lf_print_error_and_exit("Unexpected reaction status: %d. Expected %d.", done_reaction->status, queued);
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
 *  single-threaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 */
void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number) {
  (void)worker_number;  // Suppress unused parameter warning.
  if (reaction == NULL || !lf_atomic_bool_compare_and_swap32((int32_t*)&reaction->status, inactive, queued)) {
    return;
  }
  LF_PRINT_DEBUG("Scheduler: Enqueueing reaction %s, which has level %lld.", reaction->name, LF_LEVEL(reaction->index));

  // FIXME: Mutex not needed when pulling from the event queue.
  LF_PRINT_DEBUG("Scheduler: Locking mutex for environment.");
  LF_MUTEX_LOCK(&scheduler->env->mutex);
  LF_PRINT_DEBUG("Scheduler: Locked mutex for environment.");

  pqueue_insert(scheduler->custom_data->reaction_q, (void*)reaction);
  // Notify any idle workers of the new reaction.
  LF_COND_BROADCAST(&scheduler->custom_data->reaction_q_changed);

  LF_MUTEX_UNLOCK(&scheduler->env->mutex);
}
#endif // SCHEDULER == SCHED_GEDF_NP
