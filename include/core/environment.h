#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "lf_types.h"

int environment_init(
    environment_t* env,
    int id,
    int num_workers,
    int num_timers, 
    int num_startup_reactions, 
    int num_shutdown_reactions, 
    int num_reset_reactions,
    int num_is_present_fields,
    int num_modes
);

void environment_free(environment_t* env);

#define ENVIRONMENT_INIT {\
    ._lf_handle = 1\
}



#endif