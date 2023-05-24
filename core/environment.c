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
    int num_is_present_fields
) {
    env->id = id;
    env->stop_tag = FOREVER_TAG;

    env->_lf_timer_triggers_size=num_timers;
    env->_lf_timer_triggers = (trigger_t **) calloc(num_timers, sizeof(trigger_t));
    if (env->_lf_timer_triggers == NULL) {
        return 1;
    }

    env->_lf_startup_reactions_size=num_startup_reactions;
    env->_lf_startup_reactions = (reaction_t **) calloc(num_startup_reactions, sizeof(reaction_t));
    if (env->_lf_startup_reactions == NULL) {
        return 1;
    }

    env->_lf_shutdown_reactions_size=num_shutdown_reactions;
    env->_lf_shutdown_reactions = (reaction_t **) calloc(num_shutdown_reactions, sizeof(reaction_t));
    if (env->_lf_shutdown_reactions == NULL) {
        return 1;
    }

    env->_lf_reset_reactions_size=num_reset_reactions;
    env->_lf_reset_reactions = (reaction_t **) calloc(num_reset_reactions, sizeof(reaction_t));
    if (env->_lf_reset_reactions == NULL) {
        return 1;
    }

    env->_lf_is_present_fields_size = num_is_present_fields;
    env->_lf_is_present_fields_abbreviated_size = 0;

    env->_lf_is_present_fields = (bool**)calloc(num_is_present_fields, sizeof(bool*));
    if (env->_lf_is_present_fields == NULL) {
        return 1;
    }

    env->_lf_is_present_fields_abbreviated = (bool**)calloc(num_is_present_fields, sizeof(bool*));
    if (env->_lf_is_present_fields_abbreviated == NULL) {
        return 1;
    }

    env->_lf_handle=1; // FIXME: What is this?
    #ifdef LF_THREADED
    env->num_workers = num_workers;
    env->thread_ids = (lf_thread_t*)calloc(num_workers, sizeof(lf_thread_t));
    env->barrier.requestors = 0;
    env->barrier.horizon = FOREVER_TAG;
    #endif


    return 0;
}

void environment_free(environment_t* env) {
    free(env->_lf_timer_triggers);
    free(env->_lf_startup_reactions);
    free(env->_lf_shutdown_reactions);
    free(env->_lf_reset_reactions);
    free(env->_lf_is_present_fields);
    free(env->_lf_is_present_fields_abbreviated);
    #ifdef LF_THREADED
    free(env->thread_ids);
    lf_sched_free(env->scheduler);   
    #endif
}