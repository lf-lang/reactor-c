/**
 * @file
 * @author Soroush Bateni
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @author Francesco Paladino
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Global Earliest Deadline First (GEDF) non-preemptive scheduler for the
 * threaded runtime of the C target of Lingua Franca.
 *
 * At each tag, this scheduler prioritizes reactions with the smallest (inferred) deadline.
 * An inferred deadline for reaction _R_ is either an explicitly declared deadline or the declared deadline of
 * a reaction that depends on _R_. This scheduler is non-preemptive, meaning that once a worker thread starts
 * executing a reaction, it will execute that reaction to completion. The underlying thread scheduler, of
 * course, could preempt the execution in favor of some other worker thread.
 * This scheduler does not take into account execution times of reactions.
 * Moreover, it does not prioritize reactions across distinct tags.
 */
#include "lf_types.h"

#if SCHEDULER == SCHED_GEDF_NP

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include <stdint.h> // For uint32_t

#include "low_level_platform.h"
#include "environment.h"
#include "pqueue.h"
#include "reactor_threaded.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "tracepoint.h"
#include "util.h"

#ifdef FEDERATED
#include "federate.h"
#endif

// Data specific to the GEDF scheduler.
typedef struct custom_scheduler_data_t {
  pqueue_t* reaction_q;
  lf_cond_t reaction_q_changed;
  size_t current_level;
  bool solo_holds_mutex; // Indicates sole thread holds the mutex.
} custom_scheduler_data_t;

typedef struct edf_sched_node_t {
  instant_t abs_d;
  uint32_t pri;
  struct edf_sched_node_t* left;
  struct edf_sched_node_t* right;
  lf_thread_t thread_id;
} edf_sched_node_t;

// Linked list of worker threads sorted by priority.
static edf_sched_node_t* edf_elements = NULL;
static edf_sched_node_t* edf_ll_head = NULL;

/////////////////// Scheduler Private API /////////////////////////

/**
 * @brief Mark the calling thread idle and wait for notification of change to the reaction queue.
 * @param scheduler The scheduler.
 * @param worker_number The number of the worker thread.
 */
inline static void wait_for_reaction_queue_updates(lf_scheduler_t* scheduler, int worker_number) {
  scheduler->number_of_idle_workers++;

  // Set the priority to the maximum to be sure to be woken up when new reactions are available.
  // FIXME: Should only do this if the number of threads is greater than the number of cores.
  lf_thread_set_priority(lf_thread_self(), LF_SCHED_MAX_PRIORITY);

  tracepoint_worker_wait_starts(scheduler->env, worker_number);
  LF_COND_WAIT(&scheduler->custom_data->reaction_q_changed);
  tracepoint_worker_wait_ends(scheduler->env, worker_number);
  scheduler->number_of_idle_workers--;
}

/**
 * @brief Assuming this is the last worker to go idle, advance the tag.
 * @param scheduler The scheduler.
 * @return Non-zero if the stop tag has been reached.
 */
static int advance_tag(lf_scheduler_t* scheduler) {
  // Set a flag in the scheduler that the lock is held by the sole executing thread.
  // This prevents acquiring the mutex in lf_scheduler_trigger_reaction.
  scheduler->custom_data->solo_holds_mutex = true;
  if (_lf_sched_advance_tag_locked(scheduler)) {
    LF_PRINT_DEBUG("Scheduler: Reached stop tag.");
    scheduler->should_stop = true;
    scheduler->custom_data->solo_holds_mutex = false;
    // Notify all threads that the stop tag has been reached.
    LF_COND_BROADCAST(&scheduler->custom_data->reaction_q_changed);
    return 1;
  }
  scheduler->custom_data->solo_holds_mutex = false;
  // Reset the level to 0.
  scheduler->custom_data->current_level = 0;
#ifdef FEDERATED
  // In case there are blocking network input reactions at this level, stall.
  lf_stall_advance_level_federation_locked(scheduler->custom_data->current_level);
#endif
  return 0;
}

/**
 * @brief Shift the priority of elements in the linked list (LL) to assign
 * to current (freshly inserted in the LL) a priority value complying with the EDF rule.
 *
 * First, the algorithm tries to shift the nodes to the right of the position
 * where current has been inserted. If this is not possible (the tail
 * of the LL has priority 98 and there is no space to shift the previous
 * elements), the function tries to shift the nodes to the left. If
 * this again is not possible (the head of the LL has priority 2 and
 * there is no space to shift the following elements), the function
 * returns false (true otherwise).
 *
 * @param current: the node to which to assign a priority value after shifting
 *
 * @return true if the shift (either right or left) was possible and a priority
 * value was assigned to current; false otherwise.
 */
static bool shift_edf_priorities(edf_sched_node_t* current) {
  edf_sched_node_t* before = current->left;
  edf_sched_node_t* after = current->right;
  edf_sched_node_t* ptr = after;
  // count the number of times the while loop executes, so you can distribute
  // the priority space you find across the threads in the linked list
  int counter = 1;
  // set to true if we can find a shifting spot
  bool shifted = false;
  while (ptr) {
    counter++;
    if (!ptr->right && ptr->pri != LF_SCHED_MAX_PRIORITY - 1) {
      // if until the tail we cannot find a shifting spot, and the tail is not 98
      // this means the tail can be shifted to 98
      shifted = true;
      int diff = LF_SCHED_MAX_PRIORITY - 1 - ptr->left->pri;
      // the formula is explained in the else-if case
      int incr = (diff - 1) / (counter + 1) + 1;
      // change the tail's priority
      ptr->pri = LF_SCHED_MAX_PRIORITY - 1;
      lf_thread_set_priority(ptr->thread_id, ptr->pri);

      // change the priorities of node from tail to the current thread
      // to keep the relative priorities while shifting and system-calling
      ptr = ptr->left;
      do {
        ptr->pri = ptr->right->pri - incr;
        lf_thread_set_priority(ptr->thread_id, ptr->pri);
        ptr = ptr->left;
      } while (ptr != current);

      break;

    } else if (ptr->right) {
      int diff = ptr->right->pri - ptr->pri;
      if (diff > 1) {
        // eureka! we found shifting spot
        shifted = true;
        // calculate the increment interval:
        // # diff - 1 is the extra space to distribute equally
        // (-1 because we guarantee a space of at least 1 between ptr and ptr->right);
        // # counter + 1 is the number of spaces between "counter" elements in the LL;
        // # +1 replaces the ceiling (not exactly but still ok);
        int incr = (diff - 1) / (counter + 1) + 1;
        // shift every node's priority from this spot to the spot
        // we inserted the current thread, starting from the right
        do {
          ptr->pri = ptr->right->pri - incr;
          lf_thread_set_priority(ptr->thread_id, ptr->pri);
          ptr = ptr->left;
        } while (ptr != current);

        // since we found a spot and shifted priorities, we don't need to continue with the while loop
        break;
      }
    }
    ptr = ptr->right;
  }

  if (!shifted) {
    // if flag is still false, this mean we couldn't find any shifting spot on the right side
    // we need to check left (this is a copy of the above code with the needed edits... debeatable)
    ptr = before;
    counter = 1;
    while (ptr) {
      counter++;
      if (!ptr->left && ptr->pri != 2) {
        // if until the head we cannot find a shifting spot, and the head is not 2
        // (1 is reserved for reactions w/o deadline) this means the head can be shifted
        shifted = true;
        int diff = ptr->right->pri - 2;
        // usual formula to spread priorities
        int incr = (diff - 1) / (counter + 1) + 1;
        // change the head's priority
        ptr->pri = 2;
        lf_thread_set_priority(ptr->thread_id, ptr->pri);

        // change the priorities of node from head to the current thread
        ptr = ptr->right;
        do {
          ptr->pri = ptr->left->pri + incr;
          lf_thread_set_priority(ptr->thread_id, ptr->pri);
          ptr = ptr->right;
        } while (ptr != current);

        break;

      } else if (ptr->left) {
        int diff = ptr->pri - ptr->left->pri;
        if (diff > 1) {
          // eureka! we found shifting spot
          shifted = true;
          // usual formula to spread priorities
          int incr = (diff - 1) / (counter + 1) + 1;
          // shift every node's priority from this spot to the spot
          // we inserted the current thread, starting from the left
          do {
            ptr->pri = ptr->left->pri + incr;
            lf_thread_set_priority(ptr->thread_id, ptr->pri);
            ptr = ptr->right;
          } while (ptr != current);

          // since we found a spot and shifted priorities, we don't need to continue with the while loop
          break;
        }
      }
      ptr = ptr->left;
    }
  }
  return shifted;
}

/**
 * @brief Assign a priority value between 1 and 98 to the current worker thread.
 *
 * The priority is determined by the current_reaction_to_execute using EDF.
 * Priority 1 is assigned to worker threads executing reactions without a deadline,
 * while priority 99 is reserved for worker threads waiting for work. The reason
 * for the highest priority is to be sure that the awakening of these threads is not
 * delayed by the other worker threads executing reactions (even in different enclaves).
 *
 * This function enters a critical section so the mutex lock should not be held on the
 * environment when this is called.
 *
 * @param env The environment within which we are executing.
 * @param current_reaction_to_execute The reaction the current worker thread is about to serve.
 */
static void assign_edf_priority(environment_t* env, reaction_t* current_reaction_to_execute) {
  LF_PRINT_LOG("Assigning priority to reaction %s (thread %d, %ld)", current_reaction_to_execute->name, lf_thread_id(), lf_thread_self());

  // Examine the reaction's (inferred) deadline to set this thread's priority.
  interval_t inferred_deadline = (interval_t)(current_reaction_to_execute->index >> 16);
  // If there is no deadline, set the priority to 1.
  if (inferred_deadline >= (FOREVER >> 16) << 16) {
    LF_PRINT_LOG("Reaction %s has no deadline, setting its priority to 1", current_reaction_to_execute->name);
    // All reactions without (inferred) deadlines result in a thread priority of 1.
    lf_thread_set_priority(lf_thread_self(), 1);
    // assuming the edf_element was not in the linked list anymore
    // (removed on the completion of the previous served reaction)
  } else {
    // Get the absolute deadline to implement EDF.
    instant_t absolute_deadline = (env->current_tag.time + inferred_deadline);
    LF_PRINT_LOG("Reaction %s has inferred deadline " PRINTF_TIME, current_reaction_to_execute->name,
                 absolute_deadline);
    // Need to know the priorities of running threads and the absolute deadline associated with it
    // and choose a priority that is in between those with earlier deadlines and those with larger deadlines.
    edf_sched_node_t* current = &edf_elements[lf_thread_id()];
    current->abs_d = absolute_deadline;

    lf_critical_section_enter(GLOBAL_ENVIRONMENT);
    LF_PRINT_LOG("In the CS for reaction %s", current_reaction_to_execute->name);
    // if there is no head -- then the current thread is the head
    if (!edf_ll_head) {
      edf_ll_head = current;
      // FIXME: make sure this is an appropriate value
      current->pri = 50;

      LF_PRINT_LOG("No worker threads running, reaction %s is the head of the list and "
                   "gets priority %d",
                   current_reaction_to_execute->name, current->pri);
    } else {
      // there is a head in the LL

      // assuming the edf_element was not in the linked list anymore
      // (removed at the completion of the previously executed reaction)

      edf_sched_node_t* ptr = edf_ll_head;
      // find the spot that the current thread needs to insert itself
      while (ptr) {
        // the LL is from lowest priority to the highest priority
        if (ptr->abs_d < absolute_deadline) {
          LF_PRINT_LOG("Found a reaction having shorter deadline: " PRINTF_TIME, ptr->abs_d);
          // change the pointers to insert the current thread
          edf_sched_node_t* temp = ptr->left;
          ptr->left = current;
          current->right = ptr;
          current->left = temp;
          // if the insertion is not at the beginning of the list
          if (temp) {
            LF_PRINT_LOG("Insertion not at the beginning of the list");
            temp->right = current;
            // if there is enough space to assign a priority value between ptr and temp
            if (ptr->pri - temp->pri >= 2) {
              // distancing the priority by 3 from the lowest if there is enough space
              // (because it's likely that as time passes, newly coming deadlines will be bigger)
              int incr = (ptr->pri - temp->pri >= 4) ? 3 : (ptr->pri - temp->pri - 1);
              current->pri = current->left->pri + incr;
              LF_PRINT_LOG("Assigned priority %d to reaction %s", current->pri, current_reaction_to_execute->name);
            } else {
              // shift elements to find a proper priority value
              LF_PRINT_LOG("Shifting");
              if (!shift_edf_priorities(current)) {
                lf_print_error_and_exit("More threads than priority values. Aborting.");
              }
            }
          } else { // if the insertion is at the beginning of the list
            LF_PRINT_LOG("Insertion at the beginning of the list");
            edf_ll_head = current;
            // distancing the priority by 5 from ptr (if there is enough space)
            current->pri = (ptr->pri - 5 >= 2) ? (ptr->pri - 5) : 2;
          }

          break;
        } else if (ptr->right == NULL) {
          // this reaction has the earliest deadline in the list =>
          // it needs to be added as the tail of the LL
          LF_PRINT_LOG("No reactions having shorter deadline, adding %s as the tail of the LL",
                       current_reaction_to_execute->name);

          // ptr is the current tail of the LL (cannot be null)
          current->right = NULL;
          current->left = ptr;
          ptr->right = current;

          if (LF_SCHED_MAX_PRIORITY - ptr->pri >= 2) {
            // distancing the priority by 3 from the lowest if there is enough space
            // (because it's likely that as time passes, newly coming deadlines will be bigger)
            // the maximum priority is LF_SCHED_MAX_PRIORITY - 1 (on Linux it's 98)
            int incr = (LF_SCHED_MAX_PRIORITY - 1 - ptr->pri >= 4) ? 3 : (LF_SCHED_MAX_PRIORITY - 1 - ptr->pri - 1);
            current->pri = current->left->pri + incr;
            LF_PRINT_LOG("Assigned priority %d to reaction %s", current->pri, current_reaction_to_execute->name);
          } else {
            // shift elements to find a proper priority value
            LF_PRINT_LOG("Shifting (only left)");
            if (!shift_edf_priorities(current)) {
              lf_print_error_and_exit("More threads than priority values. Aborting.");
            }
          }
          break;
        }
        ptr = ptr->right;
      }
    }
    lf_thread_set_priority(lf_thread_self(), edf_elements[lf_thread_id()].pri);
    lf_critical_section_exit(GLOBAL_ENVIRONMENT);
  }
}

/**
 * @brief Remove the edf_sched_node_t element indexed by my_id from the LL and reset deadline and priority.
 * @param my_id LF thread ID of the element to remove from the LL
 */
static void remove_from_edf_ll(lf_thread_t my_id) {
  edf_sched_node_t* ptr = edf_ll_head;
  // if the thread is already on the LL -- remove the old one
  while (ptr) {
    if (ptr->thread_id == my_id) {
      if (ptr->left) {
        ptr->left->right = ptr->right;
      }
      if (ptr->right) {
        ptr->right->left = ptr->left;
      }

      // my node was the head of the LL
      if (ptr == edf_ll_head) {
        edf_ll_head = ptr->right;
      }

      ptr->abs_d = FOREVER;
      ptr->pri = 1;
      ptr->left = ptr->right = NULL;

      break;
    } else {
      ptr = ptr->right;
    }
  }
}

/**
 * @brief Assuming all other workers are idle, advance to the next level.
 * @param scheduler The scheduler.
 */
static void advance_level(lf_scheduler_t* scheduler) {
  if (++scheduler->custom_data->current_level > scheduler->max_reaction_level) {
    // Since the reaction queue is not empty, we must be cycling back to level 0 due to deadlines
    // having been given precedence over levels.  Reset the current level to 1.
    scheduler->custom_data->current_level = 0;
  }
  LF_PRINT_DEBUG("Scheduler: Advancing to next reaction level %zu.", scheduler->custom_data->current_level);
#ifdef FEDERATED
  // In case there are blocking network input reactions at this level, stall.
  lf_stall_advance_level_federation_locked(scheduler->custom_data->current_level);
#endif
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
  if (!init_sched_instance(env, &env->scheduler, number_of_workers, params)) {
    // Already initialized
    return;
  }
  
  // Environment 0 (top level) is responsible for allocating the array that stores the
  // information about worker thread priorities and deadlines.
  environment_t* top_level_env;
  int num_envs = _lf_get_environments(&top_level_env);
  if (top_level_env == env) {
    edf_elements = (edf_sched_node_t*)calloc(num_envs * top_level_env->num_workers, sizeof(edf_sched_node_t));
  }
  
  lf_scheduler_t* scheduler = env->scheduler;

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
  pqueue_free((pqueue_t*)scheduler->custom_data->reaction_q);
  // Environment 0 (top level) is responsible for freeing the array that stores the
  // information about worker thread priorities and deadlines.
  environment_t* top_level_env;
  _lf_get_environments(&top_level_env);
  if (top_level_env == scheduler->env) {
    free(edf_elements);
  }
  free(scheduler->custom_data);
}

///////////////////// Scheduler Worker API (public) /////////////////////////

void lf_sched_configure_worker() {
// Set default worker thread properties.
edf_elements[lf_thread_id()].abs_d = FOREVER;
edf_elements[lf_thread_id()].pri = 1;
edf_elements[lf_thread_id()].thread_id = lf_thread_self();
edf_elements[lf_thread_id()].left = NULL;
edf_elements[lf_thread_id()].right = NULL;

// Use the target property to set the policy.
lf_scheduling_policy_t policy = {.priority = 80, // FIXME: determine good priority
                                  .policy = LF_THREAD_POLICY};
LF_PRINT_LOG("Setting thread policy to %d to thread %d", LF_THREAD_POLICY, lf_thread_id());
int ret = lf_thread_set_scheduling_policy(lf_thread_self(), &policy);
if (ret != 0) {
  lf_print_warning("Couldn't set the scheduling policy. Try running the program with sudo rights.");
}

#if LF_NUMBER_OF_CORES > 0
  // Pin the thread to cores starting from the highest numbered core using
  // the assigned thread id: small thread id => high core number. Still,
  // respecting the constraint on the specified number of cores the program can use.
  int core_number = lf_available_cores() - 1 - (lf_thread_id() % LF_NUMBER_OF_CORES);

  ret = lf_thread_set_cpu(lf_thread_self(), core_number);
  if (ret != 0) {
    lf_print_error_and_exit("Couldn't bind thread-%u to core %d.", lf_thread_id(), core_number);
  }
  LF_PRINT_LOG("Thread %d using core_number %d", lf_thread_id(), core_number);
#endif // LF_NUMBER_OF_CORES > 0
}

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
        LF_PRINT_DEBUG("Scheduler: Worker %d found a reaction at level %zu.", worker_number,
                       scheduler->custom_data->current_level);
        // Remove the reaction from the queue.
        pqueue_pop(scheduler->custom_data->reaction_q);

        // If there is another reaction at the current level and an idle thread, then
        // notify an idle thread.
        reaction_t* next_reaction = (reaction_t*)pqueue_peek(scheduler->custom_data->reaction_q);
        if (next_reaction != NULL && LF_LEVEL(next_reaction->index) == scheduler->custom_data->current_level &&
            scheduler->number_of_idle_workers > 0) {
          // Notify an idle thread. Note that we could do a broadcast here, but it's probably not
          // a good idea because all workers awakened need to acquire the same mutex to examine the
          // reaction queue. Only one of them will acquire the mutex, and that worker can check whether
          // there are further reactions on the same level that warrant waking another worker thread.
          // So we opt to wake one other worker here rather than broadcasting.
          LF_COND_SIGNAL(&scheduler->custom_data->reaction_q_changed);
        }
        LF_MUTEX_UNLOCK(&scheduler->env->mutex);
        if (LF_THREAD_POLICY > LF_SCHED_FAIR) {
          assign_edf_priority(scheduler->env, reaction_to_return);
        }
        return reaction_to_return;
      } else {
        // Found a reaction at a level other than the current level.
        LF_PRINT_DEBUG("Scheduler: Worker %d found a reaction at level %lld. Current level is %zu", worker_number,
                       LF_LEVEL(reaction_to_return->index), scheduler->custom_data->current_level);
        // We need to wait to advance to the next level or get a new reaction at the current level.
        if (scheduler->number_of_idle_workers == scheduler->number_of_workers - 1) {
          // All other workers are idle.  Advance to the next level.
          advance_level(scheduler);
        } else {
          // Some workers are still working on reactions on the current level.
          // Wait for them to finish.
          wait_for_reaction_queue_updates(scheduler, worker_number);
        }
      }
    } else {
      // The reaction queue is empty.
      LF_PRINT_DEBUG("Worker %d finds nothing on the reaction queue.", worker_number);

      // If all other workers are idle, then we are done with this tag.
      if (scheduler->number_of_idle_workers == scheduler->number_of_workers - 1) {
        // Last thread to go idle
        LF_PRINT_DEBUG("Scheduler: Worker %d is advancing the tag.", worker_number);
        if (advance_tag(scheduler)) {
          // Stop tag has been reached.
          break;
        }
      } else {
        // Some other workers are still working on reactions on the current level.
        // Wait for them to finish.
        wait_for_reaction_queue_updates(scheduler, worker_number);
      }
    }
  }

  // It's time for the worker thread to stop and exit.
  LF_MUTEX_UNLOCK(&scheduler->env->mutex);
  return NULL;
}

void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
  (void)worker_number; // Suppress unused parameter warning.
  if (!lf_atomic_bool_compare_and_swap32((int32_t*)&done_reaction->status, queued, inactive)) {
    lf_print_error_and_exit("Unexpected reaction status: %d. Expected %d.", done_reaction->status, queued);
  }
  lf_critical_section_enter(GLOBAL_ENVIRONMENT);
  lf_thread_t my_id = lf_thread_self();
  remove_from_edf_ll(my_id);
  lf_critical_section_exit(GLOBAL_ENVIRONMENT);
}

void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number) {
  (void)worker_number; // Suppress unused parameter warning.
  if (reaction == NULL || !lf_atomic_bool_compare_and_swap32((int32_t*)&reaction->status, inactive, queued)) {
    return;
  }
  LF_PRINT_DEBUG("Scheduler: Enqueueing reaction %s, which has level %lld.", reaction->name, LF_LEVEL(reaction->index));

  // Mutex not needed when pulling from the event queue.
  if (!scheduler->custom_data->solo_holds_mutex) {
    LF_PRINT_DEBUG("Scheduler: Locking mutex for environment.");
    LF_MUTEX_LOCK(&scheduler->env->mutex);
    LF_PRINT_DEBUG("Scheduler: Locked mutex for environment.");
  }
  pqueue_insert(scheduler->custom_data->reaction_q, (void*)reaction);
  if (!scheduler->custom_data->solo_holds_mutex) {
    // If this is called from a reaction execution, then the triggered reaction
    // has one level higher than the current level. No need to notify idle threads.
    // But in federated execution, it could be called because of message arrival.
    // Also, in modal models, reset and startup reactions may be triggered.
#if defined(FEDERATED) || (defined(MODAL) && !defined(LF_SINGLE_THREADED))
    reaction_t* triggered_reaction = (reaction_t*)pqueue_peek(scheduler->custom_data->reaction_q);
    if (LF_LEVEL(triggered_reaction->index) == scheduler->custom_data->current_level) {
      LF_COND_SIGNAL(&scheduler->custom_data->reaction_q_changed);
    }
#endif // FEDERATED || MODAL

    LF_MUTEX_UNLOCK(&scheduler->env->mutex);
  }
}
#endif // SCHEDULER == SCHED_GEDF_NP
