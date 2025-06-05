/**
 * @file
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @author Soroush Bateni
 * @author Mehrdad Niknami
 * @author Alexander Schulz-Rosengarten
 * @author Erling Rennemo Jellum
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Runtime infrastructure common to the threaded and single-threaded versions of the C runtime.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "low_level_platform.h"
#include "api/schedule.h"
#ifdef MODAL_REACTORS
#include "modes.h"
#endif
#ifdef FEDERATED
#include "federate.h"
#endif
#include "port.h"
#include "pqueue.h"
#include "reactor.h"
#include "tracepoint.h"
#include "util.h"
#include "vector.h"
#include "lf_core_version.h"
#include "hashset/hashset.h"
#include "hashset/hashset_itr.h"
#include "environment.h"
#include "reactor_common.h"

#if !defined(LF_SINGLE_THREADED)
#include "watchdog.h"
#endif

// Global variable defined in tag.c:
extern instant_t start_time;

#if !defined NDEBUG
// Global variable defined in lf_token.c:
extern int _lf_count_payload_allocations;
#endif

#ifdef FEDERATED_DECENTRALIZED

/**
 * @brief Global STA (safe to advance) offset uniformly applied to advancement of each
 * time step in federated execution.
 *
 * This can be retrieved in user code by calling lf_get_sta() and adjusted by
 * calling lf_set_sta(interval_t offset).
 */
interval_t lf_fed_STA_offset = 0LL;

#endif // FEDERATED_DECENTRALIZED

/**
 * Indicator of whether to wait for physical time to match logical time.
 * By default, execution will wait. The command-line argument -fast will
 * eliminate the wait and allow logical time to exceed physical time.
 */
bool fast = false;

/**
 * The number of worker threads for threaded execution.
 * By default, execution is not threaded and this variable will have value 0.
 *
 * If the execution is threaded, a value of 0 indicates that the runtime should
 * decide on the number of workers (which will be decided based on the number of
 * available cores on the host machine).
 */
unsigned int _lf_number_of_workers = 0u;

/**
 * The logical time to elapse during execution, or -1 if no timeout time has
 * been given. When the logical equal to start_time + duration has been
 * reached, execution will terminate.
 */
instant_t duration = -1LL;

/** Indicator of whether the keepalive command-line option was given. */
bool keepalive_specified = false;

void* lf_allocate(size_t count, size_t size, struct allocation_record_t** head) {
  void* mem = calloc(count, size);
  if (mem == NULL)
    lf_print_error_and_exit("Out of memory!");
  if (head != NULL) {
    struct allocation_record_t* record = (allocation_record_t*)calloc(1, sizeof(allocation_record_t));
    if (record == NULL)
      lf_print_error_and_exit("Out of memory!");
    record->allocated = mem;
    allocation_record_t* tmp = *head; // Previous head of the list or NULL.
    *head = record;                   // New head of the list.
    record->next = tmp;
  }
  return mem;
}

/**
 * Head of a list of pointers to dynamically generated reactor
 * self structs to be freed in terminate().
 */
struct allocation_record_t* _lf_reactors_to_free = NULL;

self_base_t* lf_new_reactor(size_t size) { return (self_base_t*)lf_allocate(1, size, &_lf_reactors_to_free); }

void lf_free(struct allocation_record_t** head) {
  if (head == NULL)
    return;
  struct allocation_record_t* record = *head;
  while (record != NULL) {
    LF_PRINT_DEBUG("Freeing memory at %p", record->allocated);
    free(record->allocated);
    struct allocation_record_t* tmp = record->next;
    LF_PRINT_DEBUG("Freeing allocation record at %p", (void*)record);
    free(record);
    record = tmp;
  }
  *head = NULL;
}

void lf_free_reactor(self_base_t* self) {
  lf_free(&self->allocations);
  free(self);
}

void lf_free_all_reactors(void) {
  struct allocation_record_t* head = _lf_reactors_to_free;
  while (head != NULL) {
    lf_free_reactor((self_base_t*)head->allocated);
    struct allocation_record_t* tmp = head->next;
    free(head);
    head = tmp;
  }
  _lf_reactors_to_free = NULL;
}

void lf_set_stop_tag(environment_t* env, tag_t tag) {
  assert(env != GLOBAL_ENVIRONMENT);
  if (lf_tag_compare(tag, env->stop_tag) < 0) {
    env->stop_tag = tag;
  }
}

const char* lf_reactor_name(self_base_t* self) { return self->name; }

const char* lf_reactor_full_name(self_base_t* self) {
  if (self->full_name != NULL) {
    return self->full_name;
  }
  // First find the length of the full name.
  size_t name_len = strlen(self->name);
  size_t len = name_len;
  self_base_t* parent = self->parent;
  while (parent != NULL) {
    len++; // For the dot.
    len += strlen(parent->name);
    parent = parent->parent;
  }
  self->full_name = (char*)lf_allocate(len + 1, sizeof(char), &self->allocations);
  self->full_name[len] = '\0'; // Null terminate the string.

  size_t location = len - name_len;
  memcpy(&self->full_name[location], self->name, name_len);
  parent = self->parent;
  while (parent != NULL) {
    location--;
    self->full_name[location] = '.';
    size_t parent_len = strlen(parent->name);
    location -= parent_len;
    memcpy(&self->full_name[location], parent->name, parent_len);
    parent = parent->parent;
  }
  return self->full_name;
}

#ifdef FEDERATED_DECENTRALIZED

interval_t lf_get_stp_offset() { return lf_fed_STA_offset; }

interval_t lf_get_sta() { return lf_fed_STA_offset; }

void lf_set_stp_offset(interval_t offset) { lf_set_sta(offset); }

void lf_set_sta(interval_t offset) { lf_fed_STA_offset = offset; }

#endif // FEDERATED_DECENTRALIZED

void _lf_start_time_step(environment_t* env) {
  assert(env != GLOBAL_ENVIRONMENT);
  if (!env->execution_started) {
    // Execution hasn't started, so this is probably being invoked in termination
    // due to an error.
    return;
  }
  LF_PRINT_LOG("--------- Start time step at tag " PRINTF_TAG ".", env->current_tag.time - start_time,
               env->current_tag.microstep);
  // Handle dynamically created tokens for mutable inputs.
  _lf_free_token_copies();

  bool** is_present_fields = env->is_present_fields_abbreviated;
  int size = env->is_present_fields_abbreviated_size;
  if (env->is_present_fields_abbreviated_size > env->is_present_fields_size) {
    size = env->is_present_fields_size;
    is_present_fields = env->is_present_fields;
  }
  for (int i = 0; i < size; i++) {
    *is_present_fields[i] = false;
  }
  // Reset sparse IO record sizes to 0, if any.
  if (env->sparse_io_record_sizes.start != NULL) {
    for (size_t i = 0; i < vector_size(&env->sparse_io_record_sizes); i++) {
      // NOTE: vector_at does not return the element at
      // the index, but rather returns a pointer to that element, which is
      // itself a pointer.
      int** sizep = (int**)vector_at(&env->sparse_io_record_sizes, i);
      if (sizep != NULL && *sizep != NULL) {
        **sizep = 0;
      }
    }
  }
  env->is_present_fields_abbreviated_size = 0;

#ifdef FEDERATED
  // If the environment is the top-level one, we have some work to do.
  environment_t* envs;
  _lf_get_environments(&envs);
  if (env == envs) {
    // This is the top-level environment.

#ifdef FEDERATED_DECENTRALIZED
    for (int i = 0; i < env->is_present_fields_size; i++) {
      // An intended tag of NEVER_TAG indicates that it has never been set.
      *env->_lf_intended_tag_fields[i] = NEVER_TAG;
    }
#endif // FEDERATED_DECENTRALIZED

#ifdef FEDERATED_CENTRALIZED
    env->need_to_send_LTC = false;
#endif // FEDERATED_CENTRALIZED

    // Reset absent fields on network ports because
    // their status is unknown
    lf_reset_status_fields_on_input_port_triggers();
  }
#endif // FEDERATED
}

bool lf_is_tag_after_stop_tag(environment_t* env, tag_t tag) {
  assert(env != GLOBAL_ENVIRONMENT);
  return (lf_tag_compare(tag, env->stop_tag) > 0);
}

void _lf_pop_events(environment_t* env) {
  assert(env != GLOBAL_ENVIRONMENT);
#ifdef MODAL_REACTORS
  _lf_handle_mode_triggered_reactions(env);
#endif

  event_t* event = (event_t*)pqueue_tag_peek(env->event_q);
  while (event != NULL && lf_tag_compare(event->base.tag, env->current_tag) == 0) {
    event = (event_t*)pqueue_tag_pop(env->event_q);

    if (event->trigger == NULL) {
      LF_PRINT_DEBUG("Popped dummy event from the event queue.");
      lf_recycle_event(env, event);
      // Peek at the next event in the event queue.
      event = (event_t*)pqueue_tag_peek(env->event_q);
      continue;
    }

#ifdef MODAL_REACTORS
    // If this event is associated with an inactive mode it should haven been suspended and no longer on the event
    // queue. NOTE: This should not be possible
    if (!_lf_mode_is_active(event->trigger->mode)) {
      lf_print_warning(
          "Assumption violated. There is an event on the event queue that is associated to an inactive mode.");
    }
#endif

    lf_token_t* token = event->token;

    // Put the corresponding reactions onto the reaction queue.
    for (int i = 0; i < event->trigger->number_of_reactions; i++) {
      reaction_t* reaction = event->trigger->reactions[i];
      // Do not enqueue this reaction twice.
      if (reaction->status == inactive) {
#ifdef FEDERATED_DECENTRALIZED
        // In federated execution, an intended tag that is not (NEVER, 0)
        // indicates that this particular event is triggered by a network message.
        // The intended tag is set in handle_tagged_message in federate.c whenever
        // a tagged message arrives from another federate.
        if (event->intended_tag.time != NEVER) {
          // If the intended tag of the event is actually set,
          // transfer the intended tag to the trigger so that
          // the reaction can access the value.
          event->trigger->intended_tag = event->intended_tag;
          // And check if it is in the past compared to the current tag.
          if (lf_tag_compare(event->intended_tag, env->current_tag) < 0) {
            // Mark the triggered reaction with a STP violation
            reaction->is_STP_violated = true;
            LF_PRINT_LOG("Trigger %p has violated the reaction's STP offset. Intended tag: " PRINTF_TAG
                         ". Current tag: " PRINTF_TAG,
                         (void*)event->trigger, event->intended_tag.time - start_time, event->intended_tag.microstep,
                         env->current_tag.time - start_time, env->current_tag.microstep);
            // Need to update the last_known_status_tag of the port because otherwise,
            // the MLAA could get stuck, causing the program to lock up.
            // This should not call update_last_known_status_on_input_port because we
            // are starting a new tag step execution, so there are no reactions blocked on this input.
            if (lf_tag_compare(env->current_tag, event->trigger->last_known_status_tag) > 0) {
              event->trigger->last_known_status_tag = env->current_tag;
            }
          }
        }
#endif

#ifdef MODAL_REACTORS
        // Check if reaction is disabled by mode inactivity
        if (!_lf_mode_is_active(reaction->mode)) {
          LF_PRINT_DEBUG("Suppressing reaction %s due inactive mode.", reaction->name);
          continue; // Suppress reaction by preventing entering reaction queue
        }
#endif
        LF_PRINT_DEBUG("Triggering reaction %s.", reaction->name);
        _lf_trigger_reaction(env, reaction, -1);
      } else {
        LF_PRINT_DEBUG("Reaction is already triggered: %s", reaction->name);
      }
    }

    // Mark the trigger present
    event->trigger->status = present;

    // If the trigger is a periodic timer, create a new event for its next execution.
    if (event->trigger->is_timer && event->trigger->period > 0LL) {
      // Reschedule the trigger.
      lf_schedule_trigger(env, event->trigger, event->trigger->period, NULL);
    } else {
      // For actions, store a pointer to status field so it is reset later.
      int ipfas = lf_atomic_fetch_add(&env->is_present_fields_abbreviated_size, 1);
      if (ipfas < env->is_present_fields_size) {
        env->is_present_fields_abbreviated[ipfas] = (bool*)&event->trigger->status;
      }
    }

    // Copy the token pointer into the trigger struct so that the
    // reactions can access it. This overwrites the previous template token,
    // for which we decrement the reference count.
    _lf_replace_template_token((token_template_t*)event->trigger, token);

    // Decrement the reference count because the event queue no longer needs this token.
    // This has to be done after the above call to _lf_replace_template_token because
    // that call will increment the reference count and we need to not let the token be
    // freed prematurely.
    _lf_done_using(token);

    lf_recycle_event(env, event);

    // Peek at the next event in the event queue.
    event = (event_t*)pqueue_tag_peek(env->event_q);
  };
}

event_t* lf_get_new_event(environment_t* env) {
  assert(env != GLOBAL_ENVIRONMENT);
  // Recycle event_t structs, if possible.
  event_t* e = (event_t*)pqueue_tag_pop(env->recycle_q);
  if (e == NULL) {
    e = (event_t*)calloc(1, sizeof(struct event_t));
    if (e == NULL)
      lf_print_error_and_exit("Out of memory!");
#ifdef FEDERATED_DECENTRALIZED
    e->intended_tag = (tag_t){.time = NEVER, .microstep = 0u};
#endif
  }
  return e;
}

bool _lf_initialize_timer(environment_t* env, trigger_t* timer) {
  bool result = false;
  assert(env != GLOBAL_ENVIRONMENT);
  interval_t delay = 0;

#ifdef MODAL_REACTORS
  // Suspend all timer events that start in inactive mode
  if (!_lf_mode_is_active(timer->mode)) {
    // FIXME: The following check might not be working as
    // intended
    // && (timer->offset != 0 || timer->period != 0)) {
    event_t* e = lf_get_new_event(env);
    e->trigger = timer;
    e->base.tag = (tag_t){.time = lf_time_logical(env) + timer->offset, .microstep = 0};
    _lf_add_suspended_event(e);
    return result;
  }
#endif
  if (timer->offset == 0) {
    for (int i = 0; i < timer->number_of_reactions; i++) {
      _lf_trigger_reaction(env, timer->reactions[i], -1);
      result = true;
      tracepoint_schedule(env, timer, 0LL); // Trace even though schedule is not called.
    }
    if (timer->period == 0) {
      return result;
    } else {
      // Schedule at t + period.
      delay = timer->period;
    }
  } else {
    // Schedule at t + offset.
    delay = timer->offset;
  }

  // Get an event_t struct to put on the event queue.
  // Recycle event_t structs, if possible.
  tag_t next_tag = (tag_t){.time = lf_time_logical(env) + delay, .microstep = 0};
  // Do not schedule the next event if it is after the timeout.
  if (!lf_is_tag_after_stop_tag(env, next_tag)) {
    event_t* e = lf_get_new_event(env);
    e->trigger = timer;
    e->base.tag = next_tag;
    // NOTE: No lock is being held. Assuming this only happens at startup.
    pqueue_tag_insert(env->event_q, (pqueue_tag_element_t*)e);
    tracepoint_schedule(env, timer, delay); // Trace even though schedule is not called.
  }
  return result;
}

bool _lf_initialize_timers(environment_t* env) {
  bool result = false;
  assert(env != GLOBAL_ENVIRONMENT);
  for (int i = 0; i < env->timer_triggers_size; i++) {
    if (env->timer_triggers[i] != NULL) {
      result = _lf_initialize_timer(env, env->timer_triggers[i]) || result;
    }
  }

  // To avoid runtime memory allocations for timer-driven programs
  // the recycle queue is initialized with a single event.
  if (env->timer_triggers_size > 0) {
    event_t* e = lf_get_new_event(env);
    lf_recycle_event(env, e);
  }

  return result;
}

void _lf_trigger_startup_reactions(environment_t* env) {
  assert(env != GLOBAL_ENVIRONMENT);
  for (int i = 0; i < env->startup_reactions_size; i++) {
    if (env->startup_reactions[i] != NULL) {
      if (env->startup_reactions[i]->mode != NULL) {
        // Skip reactions in modes
        continue;
      }
      _lf_trigger_reaction(env, env->startup_reactions[i], -1);
    }
  }
#ifdef MODAL_REACTORS
  if (env->modes) {
    _lf_handle_mode_startup_reset_reactions(env, env->startup_reactions, env->startup_reactions_size, NULL, 0,
                                            env->modes->modal_reactor_states, env->modes->modal_reactor_states_size);
  }
#endif
}

void _lf_trigger_shutdown_reactions(environment_t* env) {
  assert(env != GLOBAL_ENVIRONMENT);
  for (int i = 0; i < env->shutdown_reactions_size; i++) {
    if (env->shutdown_reactions[i] != NULL) {
      if (env->shutdown_reactions[i]->mode != NULL) {
        // Skip reactions in modes
        continue;
      }
      _lf_trigger_reaction(env, env->shutdown_reactions[i], -1);
    }
  }
#ifdef MODAL_REACTORS
  if (env->modes) {
    _lf_handle_mode_shutdown_reactions(env, env->shutdown_reactions, env->shutdown_reactions_size);
  }
#endif
}

void lf_recycle_event(environment_t* env, event_t* e) {
  assert(env != GLOBAL_ENVIRONMENT);
  e->base.tag = (tag_t){.time = 0LL, .microstep = 0};
  e->trigger = NULL;
  e->token = NULL;
#ifdef FEDERATED_DECENTRALIZED
  e->intended_tag = (tag_t){.time = NEVER, .microstep = 0u};
#endif
  pqueue_tag_insert(env->recycle_q, (pqueue_tag_element_t*)e);
}

event_t* _lf_create_dummy_events(environment_t* env, tag_t tag) {
  event_t* dummy = lf_get_new_event(env);
  dummy->base.tag = tag;

  dummy->trigger = NULL;
  return dummy;
}

void lf_replace_token(event_t* event, lf_token_t* token) {
  if (event->token != token) {
    // Free the existing token, if any.
    _lf_done_using(event->token);
  }
  // Replace the token with ours.
  event->token = token;
}

trigger_handle_t _lf_schedule_at_tag(environment_t* env, trigger_t* trigger, tag_t tag, lf_token_t* token) {
  assert(env != GLOBAL_ENVIRONMENT);
  tag_t current_logical_tag = env->current_tag;

  LF_PRINT_DEBUG("_lf_schedule_at_tag() called with tag " PRINTF_TAG " at tag " PRINTF_TAG ".", tag.time - start_time,
                 tag.microstep, current_logical_tag.time - start_time, current_logical_tag.microstep);
  if (lf_tag_compare(tag, current_logical_tag) <= 0 && env->execution_started) {
    lf_print_warning("_lf_schedule_at_tag(): requested to schedule an event at the current or past tag.");
    return -1;
  }

  // Increment the reference count of the token.
  if (token != NULL) {
    token->ref_count++;
    LF_PRINT_DEBUG("_lf_schedule_at_tag: Incremented ref_count of %p to %zu.", (void*)token, token->ref_count);
  }

  // Do not schedule events if the tag is after the stop tag
  if (lf_is_tag_after_stop_tag(env, tag)) {
    lf_print_warning("_lf_schedule_at_tag: event time is past the timeout. Discarding event.");
    _lf_done_using(token);
    return -1;
  }

  event_t* e = lf_get_new_event(env);
  // Set the event time
  e->base.tag = tag;

  tracepoint_schedule(env, trigger, tag.time - current_logical_tag.time);

  // Make sure the event points to this trigger so when it is
  // dequeued, it will trigger this trigger.
  e->trigger = trigger;

  // Set the payload.
  e->token = token;

#ifdef FEDERATED_DECENTRALIZED
  // Set the intended tag
  e->intended_tag = trigger->intended_tag;
#endif

  event_t* found = (event_t*)pqueue_tag_find_equal_same_tag(env->event_q, (pqueue_tag_element_t*)e);
  if (found != NULL) {
    switch (trigger->policy) {
    case drop:
      if (found->token != token) {
        _lf_done_using(token);
      }
      lf_recycle_event(env, e);
      return (0);
      break;
    case replace:
      // Replace the payload of the event at the head with our
      // current payload.
      lf_replace_token(found, token);
      lf_recycle_event(env, e);
      return 0;
      break;
    default:
      // Adding a microstep to the original
      // intended tag.
      tag.microstep++;
      e->base.tag = tag;
      if (lf_is_tag_after_stop_tag(env, (tag_t){.time = tag.time, .microstep = tag.microstep})) {
        // Scheduling e will incur a microstep after the stop tag,
        // which is illegal.
        lf_recycle_event(env, e);
        return 0;
      }
    }
  }
  pqueue_tag_insert(env->event_q, (pqueue_tag_element_t*)e);
  trigger_handle_t return_value = env->_lf_handle++;
  if (env->_lf_handle < 0) {
    env->_lf_handle = 1;
  }
  return return_value;
}

trigger_handle_t _lf_insert_reactions_for_trigger(environment_t* env, trigger_t* trigger, lf_token_t* token) {
  assert(env != GLOBAL_ENVIRONMENT);
  // The trigger argument could be null, meaning that nothing is triggered.
  // Doing this after incrementing the reference count ensures that the
  // payload will be freed, if there is one.
  if (trigger == NULL) {
    lf_print_warning("_lf_schedule_init_reactions() called with a NULL trigger");
    _lf_done_using(token);
    return 0;
  }

  // Check to see if the trigger is not a timer
  // and not a physical action
  if (trigger->is_timer || trigger->is_physical) {
    lf_print_warning("_lf_schedule_init_reactions() called on a timer or physical action.");
    return 0;
  }

#ifdef MODAL_REACTORS
  // If this trigger is associated with an inactive mode, it should not trigger any reaction.
  if (!_lf_mode_is_active(trigger->mode)) {
    LF_PRINT_DEBUG("Suppressing reactions of trigger due inactivity of mode %s.", trigger->mode->name);
    return 1;
  }
#endif

  // Check if the trigger has violated the STP offset
  bool is_STP_violated = false;
#ifdef FEDERATED
  if (lf_tag_compare(trigger->intended_tag, env->current_tag) < 0) {
    is_STP_violated = true;
  }
#ifdef FEDERATED_CENTRALIZED
  // Check for STP violation in the centralized coordination, which is a
  // critical error.
  if (is_STP_violated) {
    lf_print_error_and_exit(
        "Attempted to insert reactions for a trigger that had an intended tag that was in the past. "
        "This should not happen under centralized coordination. Intended tag: " PRINTF_TAG ". Current tag: " PRINTF_TAG
        ").",
        trigger->intended_tag.time - lf_time_start(), trigger->intended_tag.microstep, lf_time_logical_elapsed(env),
        env->current_tag.microstep);
  }
#endif
#endif

  // Copy the token pointer into the trigger struct so that the
  // reactions can access it. This overwrites the previous template token,
  // for which we decrement the reference count.
  _lf_replace_template_token((token_template_t*)trigger, token);

  // Mark the trigger present and store a pointer to it for marking it as absent later.
  trigger->status = present;
  int ipfas = lf_atomic_fetch_add(&env->is_present_fields_abbreviated_size, 1);
  if (ipfas < env->is_present_fields_size) {
    env->is_present_fields_abbreviated[ipfas] = (bool*)&trigger->status;
  }

  // Push the corresponding reactions for this trigger
  // onto the reaction queue.
  for (int i = 0; i < trigger->number_of_reactions; i++) {
    reaction_t* reaction = trigger->reactions[i];
#ifdef MODAL_REACTORS
    // Check if reaction is disabled by mode inactivity
    if (!_lf_mode_is_active(reaction->mode)) {
      LF_PRINT_DEBUG("Suppressing reaction %s due inactivity of mode %s.", reaction->name, reaction->mode->name);
      continue; // Suppress reaction by preventing entering reaction queue
    }
#endif
    // Do not enqueue this reaction twice.
    if (reaction->status == inactive) {
      reaction->is_STP_violated = is_STP_violated;
      _lf_trigger_reaction(env, reaction, -1);
      LF_PRINT_LOG("Enqueued reaction %s at time " PRINTF_TIME ".", reaction->name, lf_time_logical(env));
    }
  }

  return 1;
}

void _lf_advance_tag(environment_t* env, tag_t next_tag) {
  assert(env != GLOBAL_ENVIRONMENT);

// FIXME: The following checks that _lf_advance_tag()
// is being called correctly. Namely, check if logical time
// is being pushed past the head of the event queue. This should
// never happen if _lf_advance_tag() is called correctly.
// This is commented out because it will add considerable overhead
// to the ordinary execution of LF programs. Instead, there might
// be a need for a target property that enables these kinds of logic
// assertions for development purposes only.
#ifndef NDEBUG
  event_t* next_event = (event_t*)pqueue_tag_peek(env->event_q);
  if (next_event != NULL) {
    if (lf_tag_compare(next_tag, next_event->base.tag) > 0) {
      lf_print_error_and_exit("_lf_advance_tag(): Attempted to move tag to " PRINTF_TAG ", which is "
                              "past the head of the event queue, " PRINTF_TAG ".",
                              next_tag.time - start_time, next_tag.microstep, next_event->base.tag.time - start_time,
                              next_event->base.tag.microstep);
    }
  }
#endif
  if (lf_tag_compare(env->current_tag, next_tag) < 0) {
    env->current_tag = next_tag;
  } else {
    lf_print_error_and_exit("_lf_advance_tag(): Attempted to move (elapsed) tag to " PRINTF_TAG ", which is "
                            "earlier than or equal to the (elapsed) current tag, " PRINTF_TAG ".",
                            next_tag.time - start_time, next_tag.microstep, env->current_tag.time - start_time,
                            env->current_tag.microstep);
  }
  LF_PRINT_LOG("Advanced (elapsed) tag to " PRINTF_TAG " at physical time " PRINTF_TIME, next_tag.time - start_time,
               env->current_tag.microstep, lf_time_physical_elapsed());
}

/**

 * Invoke the given reaction
 *
 * @param env Environment in which we are executing.
 * @param reaction The reaction that has just executed.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution (for tracing).
 */
void _lf_invoke_reaction(environment_t* env, reaction_t* reaction, int worker) {
  assert(env != GLOBAL_ENVIRONMENT);

#if !defined(LF_SINGLE_THREADED)
  if (((self_base_t*)reaction->self)->reactor_mutex != NULL) {
    LF_MUTEX_LOCK((lf_mutex_t*)((self_base_t*)reaction->self)->reactor_mutex);
  }
#endif

  tracepoint_reaction_starts(env, reaction, worker);
  ((self_base_t*)reaction->self)->executing_reaction = reaction;
  reaction->function(reaction->self);
  ((self_base_t*)reaction->self)->executing_reaction = NULL;
  tracepoint_reaction_ends(env, reaction, worker);

#if !defined(LF_SINGLE_THREADED)
  if (((self_base_t*)reaction->self)->reactor_mutex != NULL) {
    LF_MUTEX_UNLOCK((lf_mutex_t*)((self_base_t*)reaction->self)->reactor_mutex);
  }
#endif
}

/**
 * For the specified reaction, if it has produced outputs, insert the
 * resulting triggered reactions into the reaction queue.
 * This procedure assumes the mutex lock is NOT held and grabs
 * the lock only when it actually inserts something onto the reaction queue.
 * @param env Environment in which we are executing.
 * @param reaction The reaction that has just executed.
 * @param worker The thread number of the worker thread or 0 for single-threaded execution (for tracing).
 */
void schedule_output_reactions(environment_t* env, reaction_t* reaction, int worker) {
  assert(env != GLOBAL_ENVIRONMENT);

  // If the reaction produced outputs, put the resulting triggered
  // reactions into the reaction queue. As an optimization, if exactly one
  // downstream reaction is enabled by this reaction, then it may be
  // executed immediately in this same thread
  // without going through the reaction queue.
  reaction_t* downstream_to_execute_now = NULL;
  int num_downstream_reactions = 0;
#ifdef FEDERATED_DECENTRALIZED // Only pass down STP violation for federated programs that use decentralized
                               // coordination.
  // Extract the inherited STP violation
  bool inherited_STP_violation = reaction->is_STP_violated;
  LF_PRINT_DEBUG("Reaction %s has STP violation status: %d.", reaction->name, reaction->is_STP_violated);
#endif
  LF_PRINT_DEBUG("There are %zu outputs from reaction %s.", reaction->num_outputs, reaction->name);
  for (size_t i = 0; i < reaction->num_outputs; i++) {
    if (reaction->output_produced[i] != NULL && *(reaction->output_produced[i])) {
      LF_PRINT_DEBUG("Output %zu has been produced.", i);
      trigger_t** triggerArray = (reaction->triggers)[i];
      LF_PRINT_DEBUG("There are %d trigger arrays associated with output %zu.", reaction->triggered_sizes[i], i);
      for (int j = 0; j < reaction->triggered_sizes[i]; j++) {
        trigger_t* trigger = triggerArray[j];
        if (trigger != NULL) {
          LF_PRINT_DEBUG("Trigger %p lists %d reactions.", (void*)trigger, trigger->number_of_reactions);
          for (int k = 0; k < trigger->number_of_reactions; k++) {
            reaction_t* downstream_reaction = trigger->reactions[k];
#ifdef FEDERATED_DECENTRALIZED // Only pass down tardiness for federated LF programs
            // Set the is_STP_violated for the downstream reaction
            if (downstream_reaction != NULL) {
              downstream_reaction->is_STP_violated = inherited_STP_violation;
              LF_PRINT_DEBUG("Passing is_STP_violated of %d to the downstream reaction: %s",
                             downstream_reaction->is_STP_violated, downstream_reaction->name);
            }
#endif
            if (downstream_reaction != NULL && downstream_reaction != downstream_to_execute_now) {
              num_downstream_reactions++;
              // If there is exactly one downstream reaction that is enabled by this
              // reaction, then we can execute that reaction immediately without
              // going through the reaction queue. In multithreaded execution, this
              // avoids acquiring a mutex lock.
              // FIXME: Check the earliest deadline on the reaction queue.
              // This optimization could violate EDF scheduling otherwise.
              if (num_downstream_reactions == 1 && downstream_reaction->last_enabling_reaction == reaction) {
                // So far, this downstream reaction is a candidate to execute now.
                downstream_to_execute_now = downstream_reaction;
              } else {
                // If there is a previous candidate reaction to execute now,
                // it is no longer a candidate.
                if (downstream_to_execute_now != NULL) {
                  // More than one downstream reaction is enabled.
                  // In this case, if we were to execute the downstream reaction
                  // immediately without changing any queues, then the second
                  // downstream reaction would be blocked because this reaction
                  // remains on the executing queue. Hence, the optimization
                  // is not valid. Put the candidate reaction on the queue.
                  _lf_trigger_reaction(env, downstream_to_execute_now, worker);
                  downstream_to_execute_now = NULL;
                }
                // Queue the reaction.
                _lf_trigger_reaction(env, downstream_reaction, worker);
              }
            }
          }
        }
      }
    }
  }
  if (downstream_to_execute_now != NULL) {
    LF_PRINT_LOG("Worker %d: Optimizing and executing downstream reaction now: %s", worker,
                 downstream_to_execute_now->name);
    bool violation = false;
#ifdef FEDERATED_DECENTRALIZED // Only use the STP handler for federated programs that use decentralized coordination
    // If the is_STP_violated for the reaction is true,
    // an input trigger to this reaction has been triggered at a later
    // logical time than originally anticipated. In this case, a special
    // STP handler will be invoked.
    // FIXME: Note that the STP handler will be invoked
    // at most once per logical time value. If the STP handler triggers the
    // same reaction at the current time value, even if at a future superdense time,
    // then the reaction will be invoked and the STP handler will not be invoked again.
    // However, input ports to a federate reactor are network port types so this possibly should
    // be disallowed.
    // @note The STP handler and the deadline handler are not mutually exclusive.
    //  In other words, both can be invoked for a reaction if it is triggered late
    //  in logical time (STP offset is violated) and also misses the constraint on
    //  physical time (deadline).
    // @note In absence of a STP handler, the is_STP_violated will be passed down the reaction
    //  chain until it is dealt with in a downstream STP handler.
    if (downstream_to_execute_now->is_STP_violated == true) {
      // Tardiness has occurred
      LF_PRINT_LOG("Event has STP violation.");
      reaction_function_t handler = downstream_to_execute_now->STP_handler;
      // Invoke the STP handler if there is one.
      if (handler != NULL) {
        // There is a violation and it is being handled here
        // If there is no STP handler, pass the is_STP_violated
        // to downstream reactions.
        violation = true;
        LF_PRINT_LOG("Invoke tardiness handler.");
        (*handler)(downstream_to_execute_now->self);

        // If the reaction produced outputs, put the resulting
        // triggered reactions into the queue or execute them directly if possible.
        schedule_output_reactions(env, downstream_to_execute_now, worker);

        // Reset the tardiness because it has been dealt with in the
        // STP handler
        downstream_to_execute_now->is_STP_violated = false;
        LF_PRINT_DEBUG("Reset reaction's is_STP_violated field to false: %s", downstream_to_execute_now->name);
      }
    }
#endif
    if (downstream_to_execute_now->deadline >= 0LL) {
      // Get the current physical time.
      instant_t physical_time = lf_time_physical();
      // Check for deadline violation.
      if (downstream_to_execute_now->deadline == 0 ||
          physical_time > env->current_tag.time + downstream_to_execute_now->deadline) {
        // Deadline violation has occurred.
        tracepoint_reaction_deadline_missed(env, downstream_to_execute_now, worker);
        violation = true;
        // Invoke the local handler, if there is one.
        reaction_function_t handler = downstream_to_execute_now->deadline_violation_handler;
        tracepoint_reaction_starts(env, downstream_to_execute_now, worker);
        if (handler != NULL) {
          // Assume the mutex is still not held.
          (*handler)(downstream_to_execute_now->self);

          // If the reaction produced outputs, put the resulting
          // triggered reactions into the queue or execute them directly if possible.
          schedule_output_reactions(env, downstream_to_execute_now, worker);
        }
        tracepoint_reaction_ends(env, downstream_to_execute_now, worker);
      }
    }
    if (!violation) {
      // Invoke the downstream_reaction function.
      _lf_invoke_reaction(env, downstream_to_execute_now, worker);

      // If the downstream_reaction produced outputs, put the resulting triggered
      // reactions into the queue (or execute them directly, if possible).
      schedule_output_reactions(env, downstream_to_execute_now, worker);
    }

    // Reset the is_STP_violated because it has been passed
    // down the chain
    downstream_to_execute_now->is_STP_violated = false;
    LF_PRINT_DEBUG("Finally, reset reaction's is_STP_violated field to false: %s", downstream_to_execute_now->name);
  }
}

/**
 * Print a usage message.
 * TODO: This is not necessary for NO_CLI
 */
void usage(int argc, const char* argv[]) {
  printf("\nCommand-line arguments: \n\n");
  printf("  -f, --fast [true | false]\n");
  printf("   Whether to wait for physical time to match logical time.\n\n");
  printf("  -o, --timeout <duration> <units>\n");
  printf("   Stop after the specified amount of logical time, where units are one of\n");
  printf("   nsec, usec, msec, sec, minute, hour, day, week, or the plurals of those.\n\n");
  printf("  -k, --keepalive\n");
  printf("   Whether continue execution even when there are no events to process.\n\n");
  printf("  -w, --workers <n>\n");
  printf("   Executed in <n> threads if possible (optional feature).\n\n");
  printf("  -i, --id <n>\n");
  printf("   The ID of the federation that this reactor will join.\n\n");
#ifdef FEDERATED
  printf("  -r, --rti <n>\n");
  printf("   The address of the RTI, which can be in the form of user@host:port or ip:port.\n\n");
  printf("  -l\n");
  printf("   Send stdout to individual log files for each federate.\n\n");
#endif

  printf("Command given:\n");
  for (int i = 0; i < argc; i++) {
    printf("%s ", argv[i]);
  }
  printf("\n\n");
}

// Some options given in the target directive are provided here as
// default command-line options.
int default_argc = 0;
const char** default_argv = NULL;

/**
 * Process the command-line arguments. If the command line arguments are not
 * understood, then print a usage message and return 0. Otherwise, return 1.
 * @return 1 if the arguments processed successfully, 0 otherwise.
 * TODO: Not necessary for NO_CLI
 */
int process_args(int argc, const char* argv[]) {
  int i = 1;
  while (i < argc) {
    const char* arg = argv[i++];
    if (strcmp(arg, "-f") == 0 || strcmp(arg, "--fast") == 0) {
      if (argc < i + 1) {
        lf_print_error("--fast needs a boolean.");
        usage(argc, argv);
        return 0;
      }
      const char* fast_spec = argv[i++];
      if (strcmp(fast_spec, "true") == 0) {
        fast = true;
      } else if (strcmp(fast_spec, "false") == 0) {
        fast = false;
      } else {
        lf_print_error("Invalid value for --fast: %s", fast_spec);
      }
    } else if (strcmp(arg, "-o") == 0 || strcmp(arg, "--timeout") == 0 || strcmp(arg, "-timeout") == 0) {
      // Tolerate -timeout for legacy uses.
      if (argc < i + 2) {
        lf_print_error("--timeout needs time and units.");
        usage(argc, argv);
        return 0;
      }
      const char* time_spec = argv[i++];
      const char* units = argv[i++];

#if defined(PLATFORM_ARDUINO)
      duration = atol(time_spec);
#else
      duration = atoll(time_spec);
#endif

      // A parse error returns 0LL, so check to see whether that is what is meant.
      if (duration == 0LL && strncmp(time_spec, "0", 1) != 0) {
        // Parse error.
        lf_print_error("Invalid time value: %s", time_spec);
        usage(argc, argv);
        return 0;
      }
      if (strncmp(units, "sec", 3) == 0) {
        duration = SEC(duration);
      } else if (strncmp(units, "msec", 4) == 0) {
        duration = MSEC(duration);
      } else if (strncmp(units, "usec", 4) == 0) {
        duration = USEC(duration);
      } else if (strncmp(units, "nsec", 4) == 0) {
        duration = NSEC(duration);
      } else if (strncmp(units, "min", 3) == 0) {
        duration = MINUTE(duration);
      } else if (strncmp(units, "hour", 4) == 0) {
        duration = HOUR(duration);
      } else if (strncmp(units, "day", 3) == 0) {
        duration = DAY(duration);
      } else if (strncmp(units, "week", 4) == 0) {
        duration = WEEK(duration);
      } else {
        // Invalid units.
        lf_print_error("Invalid time units: %s", units);
        usage(argc, argv);
        return 0;
      }
    } else if (strcmp(arg, "-k") == 0 || strcmp(arg, "--keepalive") == 0) {
      if (argc < i + 1) {
        lf_print_error("--keepalive needs a boolean.");
        usage(argc, argv);
        return 0;
      }
      const char* keep_spec = argv[i++];
      if (strcmp(keep_spec, "true") == 0) {
        keepalive_specified = true;
      } else if (strcmp(keep_spec, "false") == 0) {
        keepalive_specified = false;
      } else {
        lf_print_error("Invalid value for --keepalive: %s", keep_spec);
      }
    } else if (strcmp(arg, "-w") == 0 || strcmp(arg, "--workers") == 0) {
      if (argc < i + 1) {
        lf_print_error("--workers needs an integer argument.s");
        usage(argc, argv);
        return 0;
      }
      const char* threads_spec = argv[i++];
      int num_workers = atoi(threads_spec);
      if (num_workers <= 0) {
        lf_print_error("Invalid value for --workers: %s. Using 1.", threads_spec);
        num_workers = 1;
      }
      _lf_number_of_workers = (unsigned int)num_workers;
    }
#ifdef FEDERATED
    else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--id") == 0) {
      if (argc < i + 1) {
        lf_print_error("--id needs a string argument.");
        usage(argc, argv);
        return 0;
      }
      const char* fid = argv[i++];
      lf_set_federation_id(fid);
      lf_print("Federation ID for executable %s: %s", argv[0], fid);
    } else if (strcmp(arg, "-r") == 0 || strcmp(arg, "--rti") == 0) {
      if (argc < i + 1) {
        lf_print_error("--rti needs a string argument in the form of [user]@[host]:[port].");
        usage(argc, argv);
        return 0;
      }
      parse_rti_code_t code = lf_parse_rti_addr(argv[i++]);
      if (code != SUCCESS) {
        switch (code) {
        case INVALID_HOST:
          lf_print_error("--rti needs a valid host");
          break;
        case INVALID_PORT:
          lf_print_error("--rti needs a valid port");
          break;
        case INVALID_USER:
          lf_print_error("--rti needs a valid user");
          break;
        case FAILED_TO_PARSE:
          lf_print_error("Failed to parse address of RTI");
          break;
        default:
          break;
        }
        usage(argc, argv);
        return 0;
      }
    }
#endif
    else if (strcmp(arg, "--ros-args") == 0) {
      // FIXME: Ignore ROS arguments for now
    } else {
      lf_print_error("Unrecognized command-line argument: %s", arg);
      usage(argc, argv);
      return 0;
    }
  }
  return 1;
}

/**
 * @brief Check that the provided version information is consistent with the
 * core runtime.
 */
#ifdef LF_TRACE
static void check_version(const version_t* version) {
#ifdef LF_SINGLE_THREADED
  LF_ASSERT(version->build_config.single_threaded == TRIBOOL_TRUE ||
                version->build_config.single_threaded == TRIBOOL_DOES_NOT_MATTER,
            "expected single-threaded version");
#else
  LF_ASSERT(version->build_config.single_threaded == TRIBOOL_FALSE ||
                version->build_config.single_threaded == TRIBOOL_DOES_NOT_MATTER,
            "expected multi-threaded version");
#endif
#ifdef NDEBUG
  LF_ASSERT(version->build_config.build_type_is_debug == TRIBOOL_FALSE ||
                version->build_config.build_type_is_debug == TRIBOOL_DOES_NOT_MATTER,
            "expected release version");
#else
  LF_ASSERT(version->build_config.build_type_is_debug == TRIBOOL_TRUE ||
                version->build_config.build_type_is_debug == TRIBOOL_DOES_NOT_MATTER,
            "expected debug version");
#endif
  LF_ASSERT(version->build_config.log_level == LOG_LEVEL || version->build_config.log_level == INT_MAX,
            "expected log level %d", LOG_LEVEL);
  // assert(!version.core_version_name || strcmp(version.core_version_name, CORE_SHA) == 0); // TODO: provide CORE_SHA
}
#endif // LF_TRACE

void initialize_global(void) {
#ifdef LF_TRACE
  check_version(lf_version_tracing());
#endif
#if !defined NDEBUG
  _lf_count_payload_allocations = 0;
  _lf_count_token_allocations = 0;
#endif

#if defined(LF_SINGLE_THREADED)
  int max_threads_tracing = 1;
#else
  environment_t* envs;
  int num_envs = _lf_get_environments(&envs);
  int max_threads_tracing = envs[0].num_workers * num_envs + 1; // add 1 for the main thread
#endif

#if defined(FEDERATED)
  // NUMBER_OF_FEDERATES is an upper bound on the number of upstream federates
  // -- threads are spawned to listen to upstream federates. Add 1 for the
  // clock sync thread and add 1 for the staa thread
  max_threads_tracing += NUMBER_OF_FEDERATES + 2;
  lf_tracing_global_init(envs[0].name, _LF_FEDERATE_NAMES_COMMA_SEPARATED, FEDERATE_ID, max_threads_tracing);
#else
  lf_tracing_global_init("main", NULL, 0, max_threads_tracing);
#endif
  // Call the code-generated function to initialize all actions, timers, and ports
  // This is done for all environments/enclaves at the same time.
  _lf_initialize_trigger_objects();

#if !defined(LF_SINGLE_THREADED) && !defined(NDEBUG)
  // If we are testing, verify that environment with pointers is correctly set up.
  for (int i = 0; i < num_envs; i++) {
    environment_verify(&envs[i]);
  }
#endif
}

/**
 * Flag to prevent termination function from executing twice and to signal to background
 * threads to terminate.
 */
bool _lf_termination_executed = false;

/** Flag used to disable cleanup operations on abnormal termination. */
bool _lf_normal_termination = false;

/**
 * Report elapsed logical and physical times and report if any
 * memory allocated for tokens has not been freed.
 */
void termination(void) {
  if (_lf_termination_executed)
    return;
  _lf_termination_executed = true;

  environment_t* env;
  int num_envs = _lf_get_environments(&env);
  // Invoke the code generated termination function. It terminates the federated related services.
  // It should only be called for the top-level environment, which, by convention, is the first environment.
  lf_terminate_execution(env);

  // In order to free tokens, we perform the same actions we would have for a new time step.
  for (int i = 0; i < num_envs; i++) {
    if (!env[i].initialized) {
      lf_print_warning("---- Environment %u was never initialized", env[i].id);
      continue;
    }
    LF_PRINT_LOG("---- Terminating environment %u, normal termination: %d", env[i].id, _lf_normal_termination);

#if !defined(LF_SINGLE_THREADED)
    // Make sure all watchdog threads have stopped
    _lf_watchdog_terminate_all(&env[i]);
#endif

    // Skip most cleanup on abnormal termination.
    if (_lf_normal_termination) {
      _lf_start_time_step(&env[i]);

#ifdef MODAL_REACTORS
      // Free events and tokens suspended by modal reactors.
      _lf_terminate_modal_reactors(&env[i]);
#endif
      // If the event queue still has events on it, report that.
      if (env[i].event_q != NULL && pqueue_tag_size(env[i].event_q) > 0) {
        lf_print_warning("---- There are %zu unprocessed future events on the event queue.",
                         pqueue_tag_size(env[i].event_q));
        event_t* event = (event_t*)pqueue_tag_peek(env[i].event_q);
        lf_print_warning("---- The first future event has timestamp " PRINTF_TAG " after start tag.",
                         event->base.tag.time - start_time, event->base.tag.microstep);
      }
      // Print elapsed times.
      // If these are negative, then the program failed to start up.
      interval_t elapsed_time = lf_time_logical_elapsed(&env[i]);
      if (elapsed_time >= 0LL) {
        char time_buffer[29]; // 28 bytes is enough for the largest 64 bit number: 9,223,372,036,854,775,807
        lf_comma_separated_time(time_buffer, elapsed_time);
        printf("---- Elapsed logical time (in nsec): %s\n", time_buffer);

        // If start_time is 0, then execution didn't get far enough along
        // to initialize this.
        if (start_time > 0LL) {
          lf_comma_separated_time(time_buffer, lf_time_physical_elapsed());
          printf("---- Elapsed physical time (in nsec): %s\n", time_buffer);
        }
      }
    }
  }
  lf_tracing_global_shutdown();
  // Skip most cleanup on abnormal termination.
  if (_lf_normal_termination) {
    _lf_free_all_tokens(); // Must be done before freeing reactors.
#if !defined NDEBUG
    // Issue a warning if a memory leak has been detected.
    if (_lf_count_payload_allocations > 0) {
      lf_print_warning("Memory allocated for messages has not been freed.");
      lf_print_warning("Number of unfreed messages: %d.", _lf_count_payload_allocations);
    }
    if (_lf_count_token_allocations > 0) {
      lf_print_warning("Memory allocated for tokens has not been freed!");
      lf_print_warning("Number of unfreed tokens: %d.", _lf_count_token_allocations);
    }
#endif
#if !defined(LF_SINGLE_THREADED)
    for (int i = 0; i < env->watchdogs_size; i++) {
      if (env->watchdogs[i]->base->reactor_mutex != NULL) {
        free(env->watchdogs[i]->base->reactor_mutex);
      }
    }
#endif
    lf_free_all_reactors();

    // Free up memory associated with environment.
    // Do this last so that printed warnings don't access freed memory.
    for (int i = 0; i < num_envs; i++) {
      environment_free(&env[i]);
    }
#if defined LF_ENCLAVES
    free_local_rti();
#endif
  }
}

index_t lf_combine_deadline_and_level(interval_t deadline, int level) {
  if (deadline > (interval_t)(ULLONG_MAX >> 16))
    return ((ULLONG_MAX >> 16) << 16) | level;
  else
    return (deadline << 16) | level;
}
