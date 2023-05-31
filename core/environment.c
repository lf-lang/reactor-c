#include "environment.h"
#include "util.h"
#include "lf_types.h"
#ifdef LF_THREADED
#include "scheduler.h"
#endif

static void environment_init_threaded(environment_t* env, int num_workers);
static void environment_init_unthreaded(environment_t* env);
static void environment_init_modes(environment_t* env, int num_modes, int num_state_resets);
static void environment_init_federated(environment_t* env, int num_is_present_fields);

static void environment_free_threaded(environment_t* env);
static void environment_free_unthreaded(environment_t* env);
static void environment_free_modes(environment_t* env);
static void environment_free_federated(environment_t* env);

int environment_init(
    environment_t* env,
    int id,
    int num_workers,
    int num_timers, 
    int num_startup_reactions, 
    int num_shutdown_reactions, 
    int num_reset_reactions,
    int num_is_present_fields,
    int num_modes,
    int num_state_resets
) {
    env->id = id;
    env->stop_tag = FOREVER_TAG;

    env->timer_triggers_size=num_timers;
    env->timer_triggers = (trigger_t **) calloc(num_timers, sizeof(trigger_t));
    lf_assert(env->timer_triggers != NULL, "Out of memory");

    env->startup_reactions_size=num_startup_reactions;
    env->startup_reactions = (reaction_t **) calloc(num_startup_reactions, sizeof(reaction_t));
    lf_assert(env->startup_reactions != NULL, "Out of memory");

    env->shutdown_reactions_size=num_shutdown_reactions;
    env->shutdown_reactions = (reaction_t **) calloc(num_shutdown_reactions, sizeof(reaction_t));
    lf_assert(env->shutdown_reactions != NULL, "Out of memory");

    env->reset_reactions_size=num_reset_reactions;
    env->reset_reactions = (reaction_t **) calloc(num_reset_reactions, sizeof(reaction_t));
    lf_assert(env->reset_reactions != NULL, "Out of memory");

    env->is_present_fields_size = num_is_present_fields;
    env->is_present_fields_abbreviated_size = 0;

    env->is_present_fields = (bool**)calloc(num_is_present_fields, sizeof(bool*));
    lf_assert(env->is_present_fields != NULL, "Out of memory");

    env->is_present_fields_abbreviated = (bool**)calloc(num_is_present_fields, sizeof(bool*));
    lf_assert(env->is_present_fields_abbreviated != NULL, "Out of memory");

    env->_lf_handle=1; // FIXME: What is this?
    
    // Initialize our priority queues.
    env->event_q = pqueue_init(INITIAL_EVENT_QUEUE_SIZE, in_reverse_order, get_event_time,
            get_event_position, set_event_position, event_matches, print_event);
    env->recycle_q = pqueue_init(INITIAL_EVENT_QUEUE_SIZE, in_no_particular_order, get_event_time,
            get_event_position, set_event_position, event_matches, print_event);
    env->next_q = pqueue_init(INITIAL_EVENT_QUEUE_SIZE, in_no_particular_order, get_event_time,
            get_event_position, set_event_position, event_matches, print_event);

    // Initialize functionality depending on target properties.
    environment_init_threaded(env, num_workers);
    environment_init_unthreaded(env);
    environment_init_modes(env, num_modes, num_state_resets);
    environment_init_federated(env, num_is_present_fields);

    return 0;
}

static void environment_init_threaded(environment_t* env, int num_workers) {
#ifdef LF_THREADED
    env->num_workers = num_workers;
    env->thread_ids = (lf_thread_t*)calloc(num_workers, sizeof(lf_thread_t));
    lf_assert(env->thread_ids != NULL, "Out of memory");
    env->barrier.requestors = 0;
    env->barrier.horizon = FOREVER_TAG;
    
    // Initialize synchronization objects
    if (lf_mutex_init(&env->mutex) != 0) {
        lf_print_error_and_exit("Could not initialize environment mutex");
    }
    if (lf_cond_init(&env->event_q_changed, &env->mutex) != 0) {
        lf_print_error_and_exit("Could not initialize environment event queue condition variable");
    }
    if (lf_cond_init(&env->global_tag_barrier_requestors_reached_zero, &env->mutex)) {
        lf_print_error_and_exit("Could not initialize environment tag barrier condition variable");
    }


#endif
}
static void environment_init_unthreaded(environment_t* env) {
#ifdef LF_UNTHREADED
    // Reaction queue ordered first by deadline, then by level.
    // The index of the reaction holds the deadline in the 48 most significant bits,
    // the level in the 16 least significant bits.
    env->reaction_q = pqueue_init(INITIAL_REACT_QUEUE_SIZE, in_reverse_order, get_reaction_index,
                get_reaction_position, set_reaction_position, reaction_matches, print_reaction);

#endif
}

static void environment_init_modes(environment_t* env, int num_modes, int num_state_resets) {
#ifdef MODAL_REACTORS
    if (num_modes > 0) {
        mode_environment_t* modes = (mode_environment_t *) calloc(1, sizeof(mode_environment_t));
        lf_assert(modes != NULL, "Out of memory");
        modes->modal_reactor_states = (reactor_mode_state_t**) calloc(num_modes, sizeof(reactor_mode_state_t*));
        lf_assert(modes->modal_reactor_states != NULL, "Out of memory");
        modes->modal_reactor_states_size = num_modes;
        modes->triggered_reactions_request = 0;

        modes->state_resets = (mode_state_variable_reset_data_t *) calloc(num_state_resets, sizeof(mode_state_variable_reset_data_t));
        lf_assert(modes->state_resets != NULL, "Out of memory");
        modes->state_resets_size = num_state_resets;

        env->modes = modes;

    } else {
        env->modes = NULL;
    }

#endif
}

static void environment_init_federated(environment_t* env, int num_is_present_fields) {
#ifdef FEDERATED_DECENTRALIZED
    env->_lf_intended_tag_fields = (bool**)calloc(num_is_present_fields, sizeof(tag_t*));
    lf_assert(env->_lf_intended_tag_fields != NULL, "Out of memory");
    env->_lf_intended_tag_fields_size = num_is_present_fields;
#endif
}



void environment_init_tags( environment_t *env, instant_t start_time, interval_t duration) {
    env->current_tag = (tag_t){.time = start_time, .microstep = 0u};
    
    tag_t stop_tag = FOREVER_TAG_INITIALIZER;
    if (duration >= 0LL) {
        // A duration has been specified. Calculate the stop time.
        stop_tag.time = env->current_tag.time + duration;
        stop_tag.microstep = 0;
    }
    env->stop_tag = stop_tag;
}

void environment_free(environment_t* env) {
    free(env->timer_triggers);
    free(env->startup_reactions);
    free(env->shutdown_reactions);
    free(env->reset_reactions);
    free(env->is_present_fields);
    free(env->is_present_fields_abbreviated);
    pqueue_free(env->event_q);
    pqueue_free(env->recycle_q);
    pqueue_free(env->next_q);

    environment_free_threaded(env);
    environment_free_unthreaded(env);
    environment_free_modes(env);
    environment_free_federated(env);
}

static void environment_free_threaded(environment_t* env) {
#ifdef LF_THREADED
    free(env->thread_ids);
    lf_sched_free(env->scheduler);   
#endif
}

static void environment_free_unthreaded(environment_t* env) {
#ifdef LF_UNTHREADED
    pqueue_free(env->reaction_q);
#endif
}

static void environment_free_modes(environment_t* env) {
#ifdef MODAL_REACTORS
    if (env->modes) {
        free(env->modes->modal_reactor_states);
        free(env->modes->state_resets);
        free(env->modes);
    }
#endif
}

static void environment_free_federated(environment_t* env) {
#ifdef FEDERATED_DECENTRALIZED
    free(env->_lf_intendend_tag_fields);
#endif
}