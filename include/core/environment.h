#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "lf_types.h"

/**
 * @brief Initialize an environment struct with parameters given in the arguments.
 */
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
    int num_state_resets,
    const char * trace_file_name
);

/**
 * @brief Free the dynamically allocated memory on the environment struct
 * @param env The environment in which we are execution
 */
void environment_free(environment_t* env);

/**
 * @brief Initialize the start and stop tags on the environment struct
 */
void environment_init_tags(
    environment_t *env, instant_t start_time, interval_t duration
);

#endif
