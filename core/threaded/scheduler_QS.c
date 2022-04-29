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
extern const int num_semaphores;
extern const inst_t** static_schedules[];
extern const uint32_t* schedule_lengths[];

/////////////////// Scheduler Variables and Structs /////////////////////////
_lf_sched_instance_t* _lf_sched_instance;

/////////////////// Scheduler Private API /////////////////////////
/**
 * @brief If there is work to be done, notify workers individually.
 *
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers() {    
    // Calculate the number of workers that we need to wake up, which is the
    // Note: All threads are idle. Therefore, there is no need to lock the mutex
    // while accessing the index for the current level.
    size_t workers_to_awaken = _lf_sched_instance->_lf_sched_number_of_workers;
    DEBUG_PRINT("Scheduler: Notifying %d workers.", workers_to_awaken);

    _lf_sched_instance->_lf_sched_number_of_idle_workers -= workers_to_awaken;
    DEBUG_PRINT("Scheduler: New number of idle workers: %u.",
                _lf_sched_instance->_lf_sched_number_of_idle_workers);
    
    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this
        // function.
        lf_mutex_lock(&_lf_sched_instance->_lf_sched_semaphore->mutex);
        lf_cond_broadcast(&_lf_sched_instance->_lf_sched_semaphore->cond);
        lf_mutex_unlock(&_lf_sched_instance->_lf_sched_semaphore->mutex);
    }
    
}

/**
 * @brief Signal all worker threads that it is time to stop.
 *
 */
void _lf_sched_signal_stop() {
    _lf_sched_instance->_lf_sched_should_stop = true;
}

/**
 * @brief Wait until the scheduler assigns work.
 *
 * If the calling worker thread is the last to become idle, it will call on the
 * scheduler to distribute work. Otherwise, it will wait on
 * '_lf_sched_instance->_lf_sched_semaphore'.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
void _lf_sched_wait_for_work(size_t worker_number) {
    // Increment the number of idle workers by 1 and check if this is the last
    // worker thread to become idle.
    if (lf_atomic_add_fetch(&_lf_sched_instance->_lf_sched_number_of_idle_workers,
                            1) ==
        _lf_sched_instance->_lf_sched_number_of_workers) {
        // Last thread to go idle
        DEBUG_PRINT("Scheduler: Worker %d is the last idle thread.",
                    worker_number);

        lf_mutex_lock(&mutex);
        // Nothing more happening at this tag.
        DEBUG_PRINT("Scheduler: Advancing tag.");
        // This worker thread will take charge of advancing tag.
        if (_lf_sched_advance_tag_locked()) {
            DEBUG_PRINT("Scheduler: Reached stop tag.");
            _lf_sched_signal_stop();
        }
        lf_mutex_unlock(&mutex);

        // Reset all the PCs to 0.
        for (int w = 0; w < _lf_sched_instance->_lf_sched_number_of_workers; w++) {
            _lf_sched_instance->pc[w] = 0;
        }

        _lf_sched_notify_workers();
    } else {
        // Not the last thread to become idle. Wait for work to be released.
        DEBUG_PRINT(
            "Scheduler: Worker %d is trying to acquire the scheduling "
            "semaphore.",
            worker_number);
        // Wait for the last thread to signal the condition variable.
        lf_mutex_lock(&_lf_sched_instance->_lf_sched_semaphore->mutex);
        lf_cond_wait(&_lf_sched_instance->_lf_sched_semaphore->cond, &_lf_sched_instance->_lf_sched_semaphore->mutex);
        lf_mutex_unlock(&_lf_sched_instance->_lf_sched_semaphore->mutex);
        
        DEBUG_PRINT("Scheduler: Worker %d acquired the scheduling semaphore.",
                    worker_number);
    }
}

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
    _lf_sched_instance->static_schedules = &static_schedules[0];
    _lf_sched_instance->current_schedule_index = 0;
    _lf_sched_instance->schedule_lengths = &schedule_lengths[0];
    _lf_sched_instance->pc = calloc(number_of_workers, sizeof(size_t));
    _lf_sched_instance->reaction_instances = params->reaction_instances;
    // Populate semaphores.
    _lf_sched_instance->semaphores = calloc(num_semaphores, sizeof(semaphore_t));
    for (int i = 0; i < num_semaphores; i++) {
        _lf_sched_instance->semaphores[i] = lf_semaphore_new(0);
    }
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    DEBUG_PRINT("Freeing the pointers in the scheduler struct.");
    free(_lf_sched_instance->pc);
    for (int i = 0; i < num_semaphores; i++) {
        lf_semaphore_destroy(_lf_sched_instance->semaphores[i]);
    }
    free(_lf_sched_instance->semaphores);
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
    DEBUG_PRINT("Worker %d inside lf_sched_get_ready_reaction", worker_number);
    // Execute the instructions
    size_t* pc = &_lf_sched_instance->pc[worker_number];
    int schedule_index = _lf_sched_instance->current_schedule_index;
    const inst_t* current_schedule = _lf_sched_instance->static_schedules[schedule_index][worker_number];
    reaction_t** reaction_instances = _lf_sched_instance->reaction_instances;
    semaphore_t** semaphores = _lf_sched_instance->semaphores;
    
    // If the instruction is Execute, return the reaction pointer and advance pc.
    // If the instruction is Wait, block until the reaction is finished (by checking
    // the semaphore) and process the next instruction until we process an Execute.
    // If the instruction is Stop, return NULL.
    reaction_t* returned_reaction = NULL;
    bool loop_done = false;
    while (*pc < _lf_sched_instance->schedule_lengths[schedule_index][worker_number] && !loop_done) {
        DEBUG_PRINT("Current instruction for worker %d: %c %zu", worker_number, current_schedule[*pc].inst, current_schedule[*pc].op);
        switch (current_schedule[*pc].inst) {
        case 'e': // Execute 
        {
            reaction_t* react = reaction_instances[current_schedule[*pc].op];
            if (react->status == queued) {
                returned_reaction = react;
                loop_done = true;
            } else
                DEBUG_PRINT("Worker %d skip execution", worker_number);
            *pc += 1;
            break;
        }
        case 'w': // Wait
            lf_semaphore_wait(semaphores[current_schedule[*pc].op]);
            *pc += 1;
            break;
        case 'n': // Notify
            lf_semaphore_release(semaphores[current_schedule[*pc].op], 1);
            *pc += 1;
            break;
        case 's': // Stop
            DEBUG_PRINT("Worker %d reaches a stop instruction", worker_number);
            // Check if the worker is the last worker to reach stop.
            // If so, this worker thread will take charge of advancing tag.
            // Ask the scheduler for more work and wait
            tracepoint_worker_wait_starts(worker_number);
            _lf_sched_wait_for_work(worker_number);
            tracepoint_worker_wait_ends(worker_number);
            loop_done = _lf_sched_instance->_lf_sched_should_stop;
            break;
        }
    };
    DEBUG_PRINT("Worker %d leaves lf_sched_get_ready_reaction", worker_number);
    return returned_reaction;
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
    if (!lf_bool_compare_and_swap(&done_reaction->status, queued, inactive)) {
        error_print_and_exit("Unexpected reaction status: %d. Expected %d.",
                             done_reaction->status, queued);
    }
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 * 
 * This function will be directly called from the top-level c file once
 * to handle the startup trigger. This function marks a reaction as queued,
 * so that it can be returned as returned_reaction.
 *
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should
 *  be used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 *
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    // Mark a reaction as queued, so that it will be executed when workers do work.
    reaction->status = queued; 
}