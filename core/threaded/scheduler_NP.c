/**
 * @file
 * @author Soroush Bateni
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Non-preemptive scheduler for the threaded runtime of the C target of Lingua Franca.
 */
#include "lf_types.h"

#if SCHEDULER == SCHED_NP || !defined(SCHEDULER)

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>

#include "low_level_platform.h"
#include "environment.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "lf_semaphore.h"
#include "tracepoint.h"
#include "util.h"
#include "reactor_threaded.h"

#ifdef FEDERATED
#include "federate.h"
#endif

// Data specific to the NP scheduler.
typedef struct custom_scheduler_data_t {
  reaction_t** executing_reactions;
  lf_mutex_t* array_of_mutexes;
  reaction_t*** triggered_reactions;
  volatile size_t next_reaction_level;
  lf_semaphore_t* semaphore; // Signal the maximum number of worker threads that should
                             // be executing work at the same time.  Initially 0.
                             // For example, if the scheduler releases the semaphore with a count of 4,
                             // no more than 4 worker threads should wake up to process reactions.
} custom_scheduler_data_t;

/////////////////// Scheduler Private API /////////////////////////

/**
 * @brief Insert 'reaction' into scheduler->triggered_reactions at the appropriate level.
 *
 * @param reaction The reaction to insert.
 */
static inline void _lf_sched_insert_reaction(lf_scheduler_t* scheduler, reaction_t* reaction) {
  size_t reaction_level = LF_LEVEL(reaction->index);
#ifdef FEDERATED
  // Lock the mutex if federated because a federate can insert reactions with
  // a level equal to the current level.
  size_t current_level = scheduler->custom_data->next_reaction_level - 1;
  // There is a race condition here where
  // `scheduler->custom_data->next_reaction_level` can change after it is
  // cached here. In that case, if the cached value is equal to
  // `reaction_level`, the cost will be an additional unnecessary mutex lock,
  // but no logic error. If the cached value is not equal to `reaction_level`,
  // it can never become `reaction_level` because the scheduler will only
  // change the `scheduler->custom_data->next_reaction_level` if it can
  // ensure that all worker threads are idle, and thus, none are triggering
  // reactions (and therefore calling this function).
  if (reaction_level == current_level) {
    LF_PRINT_DEBUG("Scheduler: Trying to lock the mutex for level %zu.", reaction_level);
    LF_MUTEX_LOCK(&scheduler->custom_data->array_of_mutexes[reaction_level]);
    LF_PRINT_DEBUG("Scheduler: Locked the mutex for level %zu.", reaction_level);
  }
  // The level index for the current level can sometimes become negative. Set
  // it back to zero before adding a reaction (otherwise worker threads will
  // not be able to see the added reaction).
  if (scheduler->indexes[reaction_level] < 0) {
    scheduler->indexes[reaction_level] = 0;
  }
#endif
  int reaction_q_level_index = lf_atomic_fetch_add32((int32_t*)&scheduler->indexes[reaction_level], 1);
  assert(reaction_q_level_index >= 0);
  LF_PRINT_DEBUG("Scheduler: Accessing triggered reactions at the level %zu with index %d.", reaction_level,
                 reaction_q_level_index);
  ((reaction_t***)scheduler->custom_data->triggered_reactions)[reaction_level][reaction_q_level_index] = reaction;
  LF_PRINT_DEBUG("Scheduler: Index for level %zu is at %d.", reaction_level, reaction_q_level_index);
#ifdef FEDERATED
  if (reaction_level == current_level) {
    LF_MUTEX_UNLOCK(&scheduler->custom_data->array_of_mutexes[reaction_level]);
  }
#endif
}

/**
 * @brief Distribute any reaction that is ready to execute to idle worker
 * thread(s).
 *
 * @return 1 if any reaction is ready. 0 otherwise.
 */
static int _lf_sched_distribute_ready_reactions(lf_scheduler_t* scheduler) {
  // Note: All the threads are idle, which means that they are done inserting
  // reactions. Therefore, the reaction vectors can be accessed without
  // locking a mutex.
  while (scheduler->custom_data->next_reaction_level <= scheduler->max_reaction_level) {
#ifdef FEDERATED
    lf_stall_advance_level_federation(scheduler->env, scheduler->custom_data->next_reaction_level);
#endif
    scheduler->custom_data->executing_reactions =
        scheduler->custom_data->triggered_reactions[scheduler->custom_data->next_reaction_level];
    LF_PRINT_DEBUG("Start of rxn queue at %zu is %p", scheduler->custom_data->next_reaction_level,
                   (void*)((reaction_t**)scheduler->custom_data->executing_reactions)[0]);

    scheduler->custom_data->next_reaction_level++;

    if (scheduler->custom_data->executing_reactions[0] != NULL) {
      // There is at least one reaction to execute
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
static void _lf_sched_notify_workers(lf_scheduler_t* scheduler) {
  // Calculate the number of workers that we need to wake up, which is the
  // number of reactions enabled at this level.
  // Note: All threads are idle. Therefore, there is no need to lock the mutex while accessing the index for the
  // current level.
  size_t workers_to_awaken = LF_MIN(scheduler->number_of_idle_workers,
                                    (size_t)(scheduler->indexes[scheduler->custom_data->next_reaction_level - 1]));
  LF_PRINT_DEBUG("Scheduler: Notifying %zu workers.", workers_to_awaken);

  scheduler->number_of_idle_workers -= workers_to_awaken;
  LF_PRINT_DEBUG("Scheduler: New number of idle workers: %zu.", scheduler->number_of_idle_workers);

  if (workers_to_awaken > 1) {
    // Notify all the workers except the worker thread that has called this
    // function.
    lf_semaphore_release(scheduler->custom_data->semaphore, (workers_to_awaken - 1));
  }
}

/**
 * @brief Signal all worker threads that it is time to stop.
 *
 */
static void _lf_sched_signal_stop(lf_scheduler_t* scheduler) {
  scheduler->should_stop = true;
  lf_semaphore_release(scheduler->custom_data->semaphore, (scheduler->number_of_workers - 1));
}

/**
 * @brief Advance tag or distribute reactions to worker threads.
 *
 * Advance tag if there are no reactions in the array of reaction vectors. If
 * there are such reactions, distribute them to worker threads.
 *
 * This function assumes the caller does not hold the 'mutex' lock.
 */
static void _lf_scheduler_try_advance_tag_and_distribute(lf_scheduler_t* scheduler) {
  // Reset the index
  environment_t* env = scheduler->env;
  scheduler->indexes[scheduler->custom_data->next_reaction_level - 1] = 0;

  // Loop until it's time to stop or work has been distributed
  while (true) {
    if (scheduler->custom_data->next_reaction_level == (scheduler->max_reaction_level + 1)) {
      scheduler->custom_data->next_reaction_level = 0;
      LF_MUTEX_LOCK(&env->mutex);
      // Nothing more happening at this tag.
      LF_PRINT_DEBUG("Scheduler: Advancing tag.");
      // This worker thread will take charge of advancing tag.
      if (_lf_sched_advance_tag_locked(scheduler)) {
        LF_PRINT_DEBUG("Scheduler: Reached stop tag.");
        _lf_sched_signal_stop(scheduler);
        LF_MUTEX_UNLOCK(&env->mutex);
        break;
      }
      LF_MUTEX_UNLOCK(&env->mutex);
    }

    if (_lf_sched_distribute_ready_reactions(scheduler) > 0) {
      _lf_sched_notify_workers(scheduler);
      break;
    }
  }
}

/**
 * @brief Wait until the scheduler assigns work.
 *
 * If the calling worker thread is the last to become idle, it will call on the
 * scheduler to distribute work. Otherwise, it will wait on
 * 'scheduler->custom_data->semaphore'.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
static void _lf_sched_wait_for_work(lf_scheduler_t* scheduler, size_t worker_number) {
  // Increment the number of idle workers by 1 and check if this is the last
  // worker thread to become idle.
  if (lf_atomic_add_fetch32((int32_t*)&scheduler->number_of_idle_workers, 1) == (int)scheduler->number_of_workers) {
    // Last thread to go idle
    LF_PRINT_DEBUG("Scheduler: Worker %zu is the last idle thread.", worker_number);
    // Call on the scheduler to distribute work or advance tag.
    _lf_scheduler_try_advance_tag_and_distribute(scheduler);
  } else {
    // Not the last thread to become idle. Wait for work to be released.
    LF_PRINT_DEBUG("Scheduler: Worker %zu is trying to acquire the scheduling semaphore.", worker_number);
    lf_semaphore_acquire(scheduler->custom_data->semaphore);
    LF_PRINT_DEBUG("Scheduler: Worker %zu acquired the scheduling semaphore.", worker_number);
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
void lf_sched_init(environment_t* env, size_t number_of_workers, sched_params_t* params) {
  assert(env != GLOBAL_ENVIRONMENT);

  LF_PRINT_DEBUG("Scheduler: Initializing with %zu workers", number_of_workers);

  // This scheduler is unique in that it requires `num_reactions_per_level` to
  // work correctly.
  if (init_sched_instance(env, &env->scheduler, number_of_workers, params)) {
    // Scheduler has not been initialized before.
    if (params == NULL || params->num_reactions_per_level == NULL) {
      lf_print_warning("Scheduler initialized with no reactions");
      return;
    }
  } else {
    // Already initialized
    return;
  }

  LF_PRINT_DEBUG("Scheduler: Max reaction level: %zu", env->scheduler->max_reaction_level);

  env->scheduler->custom_data = (custom_scheduler_data_t*)calloc(1, sizeof(custom_scheduler_data_t));

  env->scheduler->custom_data->triggered_reactions =
      (reaction_t***)calloc((env->scheduler->max_reaction_level + 1), sizeof(reaction_t**));

  env->scheduler->custom_data->array_of_mutexes =
      (lf_mutex_t*)calloc((env->scheduler->max_reaction_level + 1), sizeof(lf_mutex_t));

  env->scheduler->custom_data->semaphore = lf_semaphore_new(0);

  env->scheduler->custom_data->next_reaction_level = 1;

  env->scheduler->indexes = (volatile int*)calloc((env->scheduler->max_reaction_level + 1), sizeof(volatile int));

  size_t queue_size = INITIAL_REACT_QUEUE_SIZE;
  for (size_t i = 0; i <= env->scheduler->max_reaction_level; i++) {
    if (params != NULL) {
      if (params->num_reactions_per_level != NULL) {
        queue_size = params->num_reactions_per_level[i];
      }
    }
    // Initialize the reaction vectors
    env->scheduler->custom_data->triggered_reactions[i] = (reaction_t**)calloc(queue_size, sizeof(reaction_t*));

    LF_PRINT_DEBUG("Scheduler: Initialized vector of reactions for level %zu with size %zu", i, queue_size);

    // Initialize the mutexes for the reaction vectors
    LF_MUTEX_INIT(&env->scheduler->custom_data->array_of_mutexes[i]);
  }
  env->scheduler->custom_data->executing_reactions = env->scheduler->custom_data->triggered_reactions[0];
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free(lf_scheduler_t* scheduler) {
  if (scheduler->custom_data->triggered_reactions) {
    for (size_t j = 0; j <= scheduler->max_reaction_level; j++) {
      free(scheduler->custom_data->triggered_reactions[j]);
    }
    free(scheduler->custom_data->triggered_reactions);
  }
  free(scheduler->custom_data->array_of_mutexes);
  lf_semaphore_destroy(scheduler->custom_data->semaphore);
  free(scheduler->custom_data);
}

///////////////////// Scheduler Worker API (public) /////////////////////////

void lf_sched_configure_worker() {}

reaction_t* lf_sched_get_ready_reaction(lf_scheduler_t* scheduler, int worker_number) {
  // Iterate until the stop tag is reached or reaction vectors are empty
  while (!scheduler->should_stop) {
    // Calculate the current level of reactions to execute
    size_t current_level = scheduler->custom_data->next_reaction_level - 1;
    reaction_t* reaction_to_return = NULL;
#ifdef FEDERATED
    // Need to lock the mutex because federate.c could trigger reactions at
    // the current level (if there is a causality loop)
    LF_MUTEX_LOCK(&scheduler->custom_data->array_of_mutexes[current_level]);
#endif
    int current_level_q_index = lf_atomic_add_fetch32((int32_t*)&scheduler->indexes[current_level], -1);
    if (current_level_q_index >= 0) {
      LF_PRINT_DEBUG("Scheduler: Worker %d popping reaction with level %zu, index "
                     "for level: %d.",
                     worker_number, current_level, current_level_q_index);
      reaction_to_return = scheduler->custom_data->executing_reactions[current_level_q_index];
      scheduler->custom_data->executing_reactions[current_level_q_index] = NULL;
    }
#ifdef FEDERATED
    lf_mutex_unlock(&scheduler->custom_data->array_of_mutexes[current_level]);
#endif

    if (reaction_to_return != NULL) {
      // Got a reaction
      return reaction_to_return;
    }

    LF_PRINT_DEBUG("Worker %d is out of ready reactions.", worker_number);

    // Ask the scheduler for more work and wait
    tracepoint_worker_wait_starts(scheduler->env, worker_number);
    _lf_sched_wait_for_work(scheduler, worker_number);
    tracepoint_worker_wait_ends(scheduler->env, worker_number);
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
 * This scheduler ignores the worker number.
 *
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 *
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should
 *  be used if there is only one worker (e.g., when the program is using the
 *  single-threaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 *
 */
void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number) {
  (void)worker_number;

  if (reaction == NULL || !lf_atomic_bool_compare_and_swap32((int32_t*)&reaction->status, inactive, queued)) {
    return;
  }
  LF_PRINT_DEBUG("Scheduler: Enqueueing reaction %s, which has level %lld.", reaction->name, LF_LEVEL(reaction->index));
  _lf_sched_insert_reaction(scheduler, reaction);
}
#endif // SCHEDULER == SCHED_NP || !defined(SCHEDULER)
