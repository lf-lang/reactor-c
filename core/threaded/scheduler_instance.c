#include <assert.h>
#include "scheduler_instance.h"
#include "reactor_common.h"
#include "lf_types.h"
#include "util.h"


bool init_sched_instance(
    environment_t * env,
    lf_scheduler_t** instance,
    size_t number_of_workers,
    sched_params_t* params
) {
    
    assert(env != GLOBAL_ENVIRONMENT);
    LF_ASSERT(env, "`init_sched_instance` called without env pointer being set");

    // Check if the instance is already initialized
    lf_critical_section_enter(env);
    if (*instance != NULL) {
        // Already initialized
        lf_critical_section_exit(env);
        return false;
    } else {
        *instance =
            (lf_scheduler_t*)calloc(1, sizeof(lf_scheduler_t));
    }
    lf_mutex_unlock(&env->mutex);

    if (params == NULL || params->num_reactions_per_level_size == 0) {
        (*instance)->max_reaction_level = DEFAULT_MAX_REACTION_LEVEL;
    }

    if (params != NULL) {
        if (params->num_reactions_per_level != NULL) {
            (*instance)->max_reaction_level =
                params->num_reactions_per_level_size - 1;
        }
    }

    (*instance)->semaphore = lf_semaphore_new(0);
    (*instance)->number_of_workers = number_of_workers;
    (*instance)->next_reaction_level = 1;

    (*instance)->should_stop = false;
    (*instance)->env = env;

    return true;
}
