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
    LF_CRITICAL_SECTION_ENTER(env);
    if (*instance != NULL) {
        // Already initialized
        LF_CRITICAL_SECTION_EXIT(env);
        return false;
    } else {
        *instance =
            (lf_scheduler_t*)calloc(1, sizeof(lf_scheduler_t));
    }
    LF_MUTEX_UNLOCK(&env->mutex);

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
