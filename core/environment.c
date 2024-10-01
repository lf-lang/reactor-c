/**
 * @file
 * @author Erling R. Jellum
 * @copyright (c) 2023-2024, The Norwegian University of Science and Technology.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 *
 * This file defines functions intitializing and freeing memory for environments.
 * See environment.h for docs.
 */

#include "environment.h"
#include "util.h"
#include "lf_types.h"
#include <string.h>
#include "tracepoint.h"
#if !defined(LF_SINGLE_THREADED)
#include "scheduler.h"
#endif

//////////////////
// Local functions, not intended for use outside this file.

/**
 * @brief Callback function to determine whether two events have the same trigger.
 * This function is used by event queue and recycle.
 * Return 1 if the triggers are identical, 0 otherwise.
 * @param event1 A pointer to an event.
 * @param event2 A pointer to an event.
 */
static int event_matches(void* event1, void* event2) {
  return (((event_t*)event1)->trigger == ((event_t*)event2)->trigger);
}

/**
 * @brief Callback function to print information about an event.
 * This function is used by event queue and recycle.
 * @param element A pointer to an event.
 */
static void print_event(void* event) {
  event_t* e = (event_t*)event;
  LF_PRINT_DEBUG("tag: " PRINTF_TAG ", trigger: %p, token: %p", e->base.tag.time, e->base.tag.microstep,
                 (void*)e->trigger, (void*)e->token);
}

/**
 * @brief Initialize the threaded part of the environment struct.
 */
static void environment_init_threaded(environment_t* env, int num_workers) {
#if !defined(LF_SINGLE_THREADED)
  env->num_workers = num_workers;
  env->thread_ids = (lf_thread_t*)calloc(num_workers, sizeof(lf_thread_t));
  LF_ASSERT_NON_NULL(env->thread_ids);
  env->barrier.requestors = 0;
  env->barrier.horizon = FOREVER_TAG;

  // Initialize synchronization objects.
  LF_MUTEX_INIT(&env->mutex);
  LF_COND_INIT(&env->event_q_changed, &env->mutex);
  LF_COND_INIT(&env->global_tag_barrier_requestors_reached_zero, &env->mutex);
#else
  (void)env;
  (void)num_workers;
#endif
}

/**
 * @brief Initialize the single-threaded-specific parts of the environment struct.
 */
static void environment_init_single_threaded(environment_t* env) {
#ifdef LF_SINGLE_THREADED
  // Reaction queue ordered first by deadline, then by level.
  // The index of the reaction holds the deadline in the 48 most significant bits,
  // the level in the 16 least significant bits.
  env->reaction_q = pqueue_init(INITIAL_REACT_QUEUE_SIZE, in_reverse_order, get_reaction_index, get_reaction_position,
                                set_reaction_position, reaction_matches, print_reaction);

#else
  (void)env;
#endif
}

/**
 * @brief Initialize the modal-specific parts of the environment struct.
 */
static void environment_init_modes(environment_t* env, int num_modes, int num_state_resets) {
#ifdef MODAL_REACTORS
  if (num_modes > 0) {
    mode_environment_t* modes = (mode_environment_t*)calloc(1, sizeof(mode_environment_t));
    LF_ASSERT_NON_NULL(modes);
    modes->modal_reactor_states = (reactor_mode_state_t**)calloc(num_modes, sizeof(reactor_mode_state_t*));
    LF_ASSERT_NON_NULL(modes->modal_reactor_states);
    modes->modal_reactor_states_size = num_modes;
    modes->triggered_reactions_request = 0;

    modes->state_resets_size = num_state_resets;
    if (modes->state_resets_size > 0) {
      modes->state_resets =
          (mode_state_variable_reset_data_t*)calloc(modes->state_resets_size, sizeof(mode_state_variable_reset_data_t));
      LF_ASSERT_NON_NULL(modes->state_resets);
    } else {
      modes->state_resets = NULL;
    }

    env->modes = modes;

  } else {
    env->modes = NULL;
  }
#else
  (void)env;
  (void)num_modes;
  (void)num_state_resets;
#endif
}

/**
 * @brief Initialize the federation-specific parts of the environment struct.
 */
static void environment_init_federated(environment_t* env, int num_is_present_fields) {
#if defined(FEDERATED_CENTRALIZED)
  env->need_to_send_LTC = false;
  (void)num_is_present_fields;
#elif defined(FEDERATED_DECENTRALIZED)
  if (num_is_present_fields > 0) {
    env->_lf_intended_tag_fields = (tag_t**)calloc(num_is_present_fields, sizeof(tag_t*));
    LF_ASSERT_NON_NULL(env->_lf_intended_tag_fields);
    env->_lf_intended_tag_fields_size = num_is_present_fields;
  } else {
    env->_lf_intended_tag_fields = NULL;
    env->_lf_intended_tag_fields_size = 0;
  }
#else
  (void)env;
  (void)num_is_present_fields;
#endif
}

static void environment_free_threaded(environment_t* env) {
#if !defined(LF_SINGLE_THREADED)
  free(env->thread_ids);
  lf_sched_free(env->scheduler);
#else
  (void)env;
#endif
}

static void environment_free_single_threaded(environment_t* env) {
#ifdef LF_SINGLE_THREADED
  pqueue_free(env->reaction_q);
#else
  (void)env;
#endif
}

static void environment_free_modes(environment_t* env) {
#ifdef MODAL_REACTORS
  if (env->modes) {
    free(env->modes->modal_reactor_states);
    free(env->modes->state_resets);
    free(env->modes);
  }
#else
  (void)env;
#endif
}

static void environment_free_federated(environment_t* env) {
#ifdef FEDERATED_DECENTRALIZED
  free(env->_lf_intended_tag_fields);
#else
  (void)env;
#endif
}

//////////////////
// Functions defined in environment.h.

void environment_free(environment_t* env) {
  free(env->name);
  free(env->timer_triggers);
  free(env->startup_reactions);
  free(env->shutdown_reactions);
  free(env->reset_reactions);
  free(env->is_present_fields);
  free(env->is_present_fields_abbreviated);
  pqueue_tag_free(env->event_q);
  pqueue_tag_free(env->recycle_q);

  environment_free_threaded(env);
  environment_free_single_threaded(env);
  environment_free_modes(env);
  environment_free_federated(env);
}

void environment_init_tags(environment_t* env, instant_t start_time, interval_t duration) {
  env->current_tag = (tag_t){.time = start_time, .microstep = 0u};

  tag_t stop_tag = FOREVER_TAG_INITIALIZER;
  if (duration >= 0LL) {
    // A duration has been specified. Calculate the stop time.
    stop_tag.time = env->current_tag.time + duration;
    stop_tag.microstep = 0;
  }
  env->stop_tag = stop_tag;
}

int environment_init(environment_t* env, const char* name, int id, int num_workers, int num_timers,
                     int num_startup_reactions, int num_shutdown_reactions, int num_reset_reactions,
                     int num_is_present_fields, int num_modes, int num_state_resets, int num_watchdogs,
                     const char* trace_file_name) {
  (void)trace_file_name; // Will be used with future enclave support.

  // Space for the name string with the null terminator.
  if (name != NULL) {
    size_t name_size = strlen(name) + 1; // +1 for the null terminator
    env->name = (char*)malloc(name_size);
    LF_ASSERT_NON_NULL(env->name);
    // Use strncpy rather than strcpy to avoid compiler warnings.
    strncpy(env->name, name, name_size);
  } else {
    env->name = NULL;
  }
  env->id = id;
  env->stop_tag = FOREVER_TAG;

  env->timer_triggers_size = num_timers;
  if (env->timer_triggers_size > 0) {
    env->timer_triggers = (trigger_t**)calloc(num_timers, sizeof(trigger_t));
    LF_ASSERT_NON_NULL(env->timer_triggers);
  } else {
    env->timer_triggers = NULL;
  }

  env->startup_reactions_size = num_startup_reactions;
  if (env->startup_reactions_size > 0) {
    env->startup_reactions = (reaction_t**)calloc(num_startup_reactions, sizeof(reaction_t));
    LF_ASSERT_NON_NULL(env->startup_reactions);
  } else {
    env->startup_reactions = NULL;
  }

  env->shutdown_reactions_size = num_shutdown_reactions;
  if (env->shutdown_reactions_size > 0) {
    env->shutdown_reactions = (reaction_t**)calloc(num_shutdown_reactions, sizeof(reaction_t));
    LF_ASSERT_NON_NULL(env->shutdown_reactions);
  } else {
    env->shutdown_reactions = NULL;
  }

  env->reset_reactions_size = num_reset_reactions;
  if (env->reset_reactions_size > 0) {
    env->reset_reactions = (reaction_t**)calloc(num_reset_reactions, sizeof(reaction_t));
    LF_ASSERT_NON_NULL(env->reset_reactions);
  } else {
    env->reset_reactions = NULL;
  }

  env->is_present_fields_size = num_is_present_fields;
  env->is_present_fields_abbreviated_size = 0;

  if (env->is_present_fields_size > 0) {
    env->is_present_fields = (bool**)calloc(num_is_present_fields, sizeof(bool*));
    LF_ASSERT_NON_NULL(env->is_present_fields);
    env->is_present_fields_abbreviated = (bool**)calloc(num_is_present_fields, sizeof(bool*));
    LF_ASSERT_NON_NULL(env->is_present_fields_abbreviated);
  } else {
    env->is_present_fields = NULL;
    env->is_present_fields_abbreviated = NULL;
  }

  env->watchdogs_size = num_watchdogs;
  if (env->watchdogs_size > 0) {
    env->watchdogs = (watchdog_t**)calloc(env->watchdogs_size, sizeof(watchdog_t*));
    LF_ASSERT(env->watchdogs, "Out of memory");
  }

  env->_lf_handle = 1;

  // Initialize our priority queues.
  env->event_q = pqueue_tag_init_customize(INITIAL_EVENT_QUEUE_SIZE, pqueue_tag_compare, event_matches, print_event);
  env->recycle_q =
      pqueue_tag_init_customize(INITIAL_EVENT_QUEUE_SIZE, in_no_particular_order, event_matches, print_event);

  // Initialize functionality depending on target properties.
  environment_init_threaded(env, num_workers);
  environment_init_single_threaded(env);
  environment_init_modes(env, num_modes, num_state_resets);
  environment_init_federated(env, num_is_present_fields);

  env->initialized = true;
  return 0;
}

void environment_verify(environment_t* env) {
  for (int i = 0; i < env->is_present_fields_size; i++) {
    LF_ASSERT_NON_NULL(env->is_present_fields[i]);
  }
}