#include "environment.h"
#include "util.h"
#include "lf_types.h"
#ifdef LF_THREADED
#include "scheduler.h"
#endif

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
    #ifdef LF_THREADED
    env->num_workers = num_workers;
    env->thread_ids = (lf_thread_t*)calloc(num_workers, sizeof(lf_thread_t));
    lf_assert(env->thread_ids != NULL, "Out of memory");
    env->barrier.requestors = 0;
    env->barrier.horizon = FOREVER_TAG;
    #endif

    #ifdef MODAL_REACTORS
    if (num_modes > 0) {
        mode_environment_t* modes = (mode_environment_t *) malloc(sizeof(mode_environment_t));
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
    
    
    #ifdef FEDERATED_DECENTRALIZED
    env->_lf_intended_tag_fields = (bool**)calloc(num_is_present_fields, sizeof(tag_t*));
    lf_assert(env->_lf_intended_tag_fields != NULL, "Out of memory");
    env->_lf_intended_tag_fields_size = num_is_present_fields;
    #endif
    return 0;
}

void environment_free(environment_t* env) {
    free(env->timer_triggers);
    free(env->startup_reactions);
    free(env->shutdown_reactions);
    free(env->reset_reactions);
    free(env->is_present_fields);
    free(env->is_present_fields_abbreviated);
    #ifdef LF_THREADED
    free(env->thread_ids);
    lf_sched_free(env->scheduler);   
    #endif
}