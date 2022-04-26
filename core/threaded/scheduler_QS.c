/* Quasi-static scheduler for the threaded runtime of the C target of Lingua
Franca. */

/*************
Copyright (c) 2022, The University of Texas at Dallas. Copyright (c) 2022, The
University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/**
 * Quasi-static scheduler for the threaded runtime of the C target of Lingua
 * Franca.
 *
 * @author{Shaokai Lin <shaokai@berkeley.edu>}
 */

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include <assert.h>

#include "../platform.h"
#include "../utils/semaphore.h"
#include "scheduler.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.c"

#include "static_schedule.h" // Generated

/////////////////// External Variables /////////////////////////
extern lf_mutex_t mutex;
extern const int reaction_count;
extern const inst_t** static_schedules[];
extern const uint32_t* schedule_lengths[];

/////////////////// Scheduler Variables and Structs /////////////////////////
_lf_sched_instance_t* _lf_sched_instance;

///////////////////// Scheduler Init and Destroy API /////////////////////////
/**
 * @brief Initialize the scheduler.
 *
 * This has to be called before other functions of the scheduler can be used.
 * If the scheduler is already initialized, this will be a no-op.
 *
 * @param number_of_workers Indicate how many workers this scheduler will be
 *  managing.
 * @param option Pointer to a `sched_params_t` struct containing additional
 *  scheduler parameters.
 */
void lf_sched_init(
    size_t number_of_workers, 
    sched_params_t* params
) {
    DEBUG_PRINT("Scheduler: Initializing with %d workers", number_of_workers);

    // This scheduler is unique in that it requires `num_reactions_per_level` to
    // work correctly.
    if (init_sched_instance(&_lf_sched_instance, number_of_workers, params)) {
        // Scheduler has not been initialized before.
        if (params == NULL) {
            error_print_and_exit(
                "Scheduler: Internal error. The NP scheduler "
                "requires params to be set.");
        }
    } else {
        // Already initialized
        return;
    }

    // Initialize the QS-specific fields.
    _lf_sched_instance->static_schedules = static_schedules;
    _lf_sched_instance->current_schedule = NULL;
    _lf_sched_instance->schedule_lengths = schedule_lengths;
    _lf_sched_instance->pc = calloc(number_of_workers, sizeof(size_t));
    // TODO: The entries will be filled in when reactions instantiate in.
    _lf_sched_instance->reaction_instances = calloc(reaction_count, sizeof(reaction_t*));

    _lf_sched_instance->_lf_sched_array_of_mutexes = calloc(reaction_count, sizeof(lf_mutex_t));
    _lf_sched_instance->reaction_return_values = calloc(number_of_workers, sizeof(int));
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    free(_lf_sched_instance->pc);
    free(_lf_sched_instance->reaction_instances);
}

///////////////////// Scheduler Worker API (public) /////////////////////////
/**
 * @brief Ask the scheduler for one more reaction.
 *
 * This function blocks until it can return a ready reaction for worker thread
 * 'worker_number' or it is time for the worker thread to stop and exit (where a
 * NULL value would be returned).
 *
 * @param worker_number
 * @return reaction_t* A reaction for the worker to execute. NULL if the calling
 * worker thread should exit.
 */
reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    // Execute the instructions
    int pc = _lf_sched_instance->pc[worker_number];
    int ret_value = _lf_sched_instance->reaction_return_values[worker_number];
    inst_t* sch_base = _lf_sched_instance->static_schedules[_lf_sched_instance->current_schedule_index];
    reaction_t* react_base = _lf_sched_instance->reaction_instances[_lf_sched_instance->current_schedule_index];
    semaphore_t** sema_base = _lf_sched_instance->reaction_semaphore;
    
    // If the instruction is Execute, return the reaction pointer and advance pc.
    // If the instruction is Wait, block until the reaction is finished (by checking
    // the semaphore) and process the next instruction until we process an Execute.
    // If the instruction is Stop, return NULL.
    do {
        pc += 1;
        switch (sch_base[pc].id) {
        case 'e': // Execute
            _lf_sched_instance->pc[worker_number] = pc;
            reaction_t* react = &react_base[sch_base[pc].op1];
            if (react->status == queue) return react;
            break;
        case 'b': // Branch
            pc = ret_value == sch_base[pc].op1 ? sch_base[pc].op2 : pc + 1;
            break;
        case 'w': // Wait
            lf_semaphore_wait(&sema_base[sch_base[pc].op1][sch_base[pc].op2]);
            break;
        case 's': // Stop
            return NULL;
        }
    } while (pc < _lf_sched_instance->schedule_lengths[worker_number]);
    return NULL;
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 * 
 * Check if the reaction that we just executed produces any outputs
 * (similar to how the schedule_output_reactions() function does it).
 * If there are outputs, append them to an event queue (a linked list
 * of linked list, with each outer linked list representing a list of
 * signals present at some time step).
 *
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number,
                                 reaction_t* done_reaction) {
    semaphore_t** sema_base = _lf_sched_instance->reaction_semaphore;
    _lf_sched_instance->reaction_return_values[worker_number] = done_reaction->output_produced[i];
    lf_semaphore_release(&sema_base[_lf_sched_instance->current_schedule_index[worker_number]]
        [_lf_sched_instance->pc[worker_number]]);
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 *
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should
 *  be used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 *
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    // Shaokai: We might not need to do anything here.
}