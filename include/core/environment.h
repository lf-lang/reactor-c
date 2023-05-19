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
    void (*initialize_trigger_ojects_func)(environment_t* env)
);

void environment_free(environment_t* env);

#define ENVIRONMENT_INIT {\
    ._lf_handle = 1\
}



#endif