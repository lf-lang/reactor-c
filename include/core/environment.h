/**
 * @file
 * @author Erling R. Jellum (erling.r.jellum@ntnu.no)
 *
 * @section LICENSE
 * Copyright (c) 2023, The Norwegian University of Science and Technology.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION API for creating and destroying environments. An environment is the
 * "context" within which the reactors are executed. The environment contains data structures
 * which are shared among the reactors such as priority queues, the current logical tag, 
 * the worker scheduler, and a lot of meta data. Each reactor stores a pointer to its
 * environment on its self-struct. If a LF program has multiple scheduling enclaves,
 * then each enclave will have its own environment.
 * 
 */
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "lf_types.h"
#include "platform.h"
#include "trace.h"

// Forward declarations so that a pointers can appear in the environment struct.
typedef struct lf_scheduler_t lf_scheduler_t;
typedef struct mode_environment_t mode_environment_t;
typedef struct enclave_info_t enclave_info_t;

/**
 * @brief The global environment.
 * Some operations are not specific to a particular scheduling enclave and therefore
 * have no associated environment. When invoking a function such as lf_critical_section_enter,
 * which requires an environment argument, it may be possible to pass this GLOBAL_ENVIRONMENT.
 * For lf_critical_section_enter, for example, this may acquire a global mutex instead of
 * a mutex specific to a particular scheduling enclave. Most functions that take environment
 * arguments, however, cannot accept the GLOBAL_ENVIRONMENT argument, and passing it will
 * result in an assertion violation.
 */
#define GLOBAL_ENVIRONMENT NULL

/**
 * @brief Execution environment.
 * This struct contains information about the execution environment.
 * An execution environment maintains a notion of a "current tag"
 * and has its own event queue and scheduler.
 * Normally, there is only one execution environment, but if you use
 * scheduling enclaves, then there will be one for each enclave.
 */
typedef struct environment_t {
    bool initialized;
    bool execution_started;  // Events at the start tag have been pulled from the event queue.
    char *name;
    int id;
    tag_t current_tag;
    tag_t stop_tag;
    pqueue_t *event_q;
    pqueue_t *recycle_q;
    pqueue_t *next_q;
    bool** is_present_fields;
    int is_present_fields_size;
    bool** is_present_fields_abbreviated;
    int is_present_fields_abbreviated_size;
    vector_t sparse_io_record_sizes;
    trigger_handle_t _lf_handle;
    trigger_t** timer_triggers;
    int timer_triggers_size;
    reaction_t** startup_reactions;
    int startup_reactions_size;
    reaction_t** shutdown_reactions;
    int shutdown_reactions_size;
    reaction_t** reset_reactions;
    int reset_reactions_size;
    mode_environment_t* modes;
    trace_t* trace;
    int worker_thread_count;
#if defined(LF_SINGLE_THREADED)
    pqueue_t *reaction_q;
#else
    int num_workers;
    lf_thread_t* thread_ids;
    lf_mutex_t mutex;
    lf_cond_t event_q_changed;
    lf_scheduler_t* scheduler;
    _lf_tag_advancement_barrier barrier;
    lf_cond_t global_tag_barrier_requestors_reached_zero;
#endif // LF_SINGLE_THREADED
#if defined(FEDERATED)
    tag_t** _lf_intended_tag_fields;
    int _lf_intended_tag_fields_size;
#endif // FEDERATED
#ifdef LF_ENCLAVES // TODO: Consider dropping #ifdef
    enclave_info_t *enclave_info;
#endif
} environment_t;

#if defined(MODAL_REACTORS)
struct mode_environment_t {
    uint8_t triggered_reactions_request;
    reactor_mode_state_t** modal_reactor_states;
    int modal_reactor_states_size;
    mode_state_variable_reset_data_t* state_resets;
    int state_resets_size;
};
#endif

/**
 * @brief Initialize an environment struct with parameters given in the arguments.
 */
int environment_init(
    environment_t* env,
    const char * name,
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
 * @brief Free the dynamically allocated memory on the environment struct.
 * @param env The environment in which we are executing.
 */
void environment_free(environment_t* env);

/**
 * @brief Initialize the start and stop tags on the environment struct.
 */
void environment_init_tags(
    environment_t *env, instant_t start_time, interval_t duration
);

/**
 * @brief Will update the argument to point to the beginning of the array of environments in this program
 * @note Is code-generated by the compiler
 * @param envs A double pointer which will be dereferenced and modified
 * @return int The number of environments in the array
 */
int _lf_get_environments(environment_t **envs);

#endif
