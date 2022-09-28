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
#include "../utils/vector.h"

#include "../../schedule.h" // Generated

/////////////////// External Variables /////////////////////////
extern lf_mutex_t mutex;
extern const int num_semaphores;
extern const inst_t** static_schedules[];
extern const uint32_t* schedule_lengths[];

extern char* static_schedule_path;
FILE* static_schedule_fp;

/////////////////// Scheduler Variables and Structs /////////////////////////
_lf_sched_instance_t* _lf_sched_instance;

/////////////////// Scheduler Private API /////////////////////////
/**
 * @brief If there is work to be done, notify workers individually.
 *
 * This currently assumes that the caller IS holding any thread mutexes.
 * FIXME: Does the caller need to hold the mutex here?
 */
void _lf_sched_notify_workers() {    
    // Note: All threads are idle. Therefore, there is no need to lock the mutex
    // while accessing the index for the current level.
    size_t workers_to_awaken = _lf_sched_instance->_lf_sched_number_of_workers;
    LF_PRINT_DEBUG("Scheduler: Notifying %d workers.", workers_to_awaken);

    _lf_sched_instance->_lf_sched_number_of_idle_workers -= workers_to_awaken;
    LF_PRINT_DEBUG("Scheduler: New number of idle workers: %u.",
                _lf_sched_instance->_lf_sched_number_of_idle_workers);
    
    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this
        // function.
        // FIXME: Use lf_semaphore_release here.
        lf_cond_broadcast(&_lf_sched_instance->_lf_sched_array_of_conds[0]);
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
    LF_PRINT_DEBUG(
        "Scheduler: Worker %d is trying to acquire the scheduling "
        "semaphore.",
        worker_number);

    // FIXME: There might be a better place to lock the mutex.
    lf_mutex_lock(&mutex);
    
    if (++_lf_sched_instance->_lf_sched_number_of_idle_workers ==
        _lf_sched_instance->_lf_sched_number_of_workers) {
        
        // FIXME: Check if we can lock the mutex here.
        // If not, find out why this could create deadlocks.
        
        // Last thread to go idle
        LF_PRINT_DEBUG("Scheduler: Worker %d is the last idle thread.",
                    worker_number);

        // Nothing more happening at this tag.
        LF_PRINT_DEBUG("Scheduler: Advancing tag.");
        
        // This worker thread will take charge of advancing tag.
        if (_lf_sched_advance_tag_locked()) {
            LF_PRINT_DEBUG("Scheduler: Reached stop tag.");
            _lf_sched_signal_stop();
        }

        // FIXME: Check if we can unlock the mutex here.

        // Reset all the PCs to 0.
        for (int w = 0; w < _lf_sched_instance->_lf_sched_number_of_workers; w++) {
            _lf_sched_instance->pc[w] = 0;
        }

        // Reset all the semaphores.
        for (int i = 0; i < _lf_sched_instance->num_semaphores; i++) {
            _lf_sched_instance->semaphores[i]->count = 0 ;
        }
        // Wake up all the idle workers to do work at the new time tag.
        _lf_sched_notify_workers();

    } else {
        // Not the last thread to become idle. Wait for work to be released.
        // Wait for the last thread to signal the condition variable.
        // FIXME: Should the index of _lf_sched_array_of_conds be the worker number?
        lf_cond_wait(&_lf_sched_instance->_lf_sched_array_of_conds[0], &mutex);
        
        LF_PRINT_DEBUG("Scheduler: Worker %d acquired the scheduling semaphore.",
                    worker_number);
    }

    // FIXME: There might be a better place to unlock the mutex.
    lf_mutex_unlock(&mutex);
    // FIXME: Can we move _lf_sched_notify_workers() here?
}

///////////////////// Scheduler Init and Destroy API /////////////////////////
/**
 * @brief Set the static schedule from file object.
 * 
 * @param fp the file pointer of the static schedule file
 */
void set_static_schedule_from_file(
    FILE* static_schedule_fp,
    size_t number_of_workers,
    sched_params_t* params) {

    // Initialize fields.
    _lf_sched_instance->current_schedule_index = 0;
    _lf_sched_instance->pc = calloc(number_of_workers, sizeof(size_t));
    _lf_sched_instance->reaction_instances = params->reaction_instances;
    _lf_sched_instance->_lf_sched_array_of_conds = (lf_cond_t*) calloc(1, sizeof(lf_cond_t));
    lf_cond_init(&_lf_sched_instance->_lf_sched_array_of_conds[0]);
    
    // Read static_schedules, infer schedule length and number of semaphores.
    char*   line = NULL;
    size_t  len  = 0;
    ssize_t read;

    // FIXME: Need to populate each each entry.
    _lf_sched_instance->schedule_lengths = (uint32_t**) malloc(sizeof(uint32_t*));
    _lf_sched_instance->static_schedules = (inst_t***) malloc(sizeof(inst_t**));
    _lf_sched_instance->schedule_lengths[0] = (uint32_t*) malloc(sizeof(uint32_t) * number_of_workers);
    _lf_sched_instance->static_schedules[0] = (inst_t**) malloc(sizeof(inst_t*) * number_of_workers);
    int current_worker = -1;
    int inst_index = 0;
    int max_semaphore_id = 0;

    while ((read = getline(&line, &len, static_schedule_fp)) != -1) {
        LF_PRINT_DEBUG("Line: %s", line);
        
        // Continue if the line is whitespace
        if (is_empty(line)) continue;

        // Split the line by space.
        char* first = strtok(line, " ");
        char* second = strtok(NULL, " ");
        if (strtok(NULL, " ") != NULL) {
            lf_print_error_and_exit(
                "More than two tokens detected: %s", line);
        }

        if (first != NULL && second == NULL) {
            current_worker++;
            inst_index = 0;
            int length = atoi(first);
            _lf_sched_instance->schedule_lengths[0][current_worker] = length;
            _lf_sched_instance->static_schedules[0][current_worker] =
                (inst_t*) malloc(sizeof(inst_t) * length);
            LF_PRINT_DEBUG("Schedule length for worker %d: %d", current_worker, length);
        } else if (first != NULL && second != NULL) {
            char operator = first[0];
            int operand = atoi(second);
            _lf_sched_instance->static_schedules[0][current_worker][inst_index] =
                (inst_t) { .inst = operator, .op = operand };
            LF_PRINT_DEBUG("Instruction parsed: %s %d", first, operand);
            if ((operator == 'w' || operator == 'n') && operand > max_semaphore_id) {
                max_semaphore_id = operand;
            }
            inst_index++;
            // FIXME: Need to check whether these instructions are valid.
        }
    }

    // Populate semaphores.
    LF_PRINT_DEBUG("Number of semaphores needed: %d", max_semaphore_id);
    _lf_sched_instance->num_semaphores = max_semaphore_id + 1; // Semaphore id is 0-indexed.
    _lf_sched_instance->semaphores = calloc(_lf_sched_instance->num_semaphores, sizeof(semaphore_t));
    for (int i = 0; i < _lf_sched_instance->num_semaphores; i++) {
        _lf_sched_instance->semaphores[i] = lf_semaphore_new(0);
    }

    if (line) free(line);
}

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
    LF_PRINT_DEBUG("Scheduler: Initializing with %d workers", number_of_workers);

    // This scheduler is unique in that it requires `num_reactions_per_level` to
    // work correctly.
    if (init_sched_instance(&_lf_sched_instance, number_of_workers, params)) {
        // Scheduler has not been initialized before.
        if (params == NULL) {
            lf_print_error_and_exit(
                "Scheduler: Internal error. The NP scheduler "
                "requires params to be set.");
        }
    } else {
        // Already initialized
        return;
    }

    // Initialize the QS-specific fields.
    if (static_schedule_path == NULL) {
        _lf_sched_instance->static_schedules = &static_schedules[0];
        _lf_sched_instance->current_schedule_index = 0;
        _lf_sched_instance->schedule_lengths = &schedule_lengths[0];
        _lf_sched_instance->pc = calloc(number_of_workers, sizeof(size_t));
        _lf_sched_instance->reaction_instances = params->reaction_instances;
        _lf_sched_instance->_lf_sched_array_of_conds = (lf_cond_t*) calloc(1, sizeof(lf_cond_t));
        lf_cond_init(&_lf_sched_instance->_lf_sched_array_of_conds[0]);
        // Populate semaphores.
        _lf_sched_instance->num_semaphores = num_semaphores;
        _lf_sched_instance->semaphores = calloc(num_semaphores, sizeof(semaphore_t));
        for (int i = 0; i < num_semaphores; i++) {
            _lf_sched_instance->semaphores[i] = lf_semaphore_new(0);
        }
    } else {
        static_schedule_fp = fopen(static_schedule_path, "r");
        if (static_schedule_fp == NULL) {
            lf_print_error_and_exit(
                "Failed to open the schedule file: %s", static_schedule_path);
        }
        set_static_schedule_from_file(static_schedule_fp, number_of_workers, params);
    }
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    LF_PRINT_DEBUG("Freeing the pointers in the scheduler struct.");
    free(_lf_sched_instance->pc);
    for (int i = 0; i < _lf_sched_instance->num_semaphores; i++) {
        lf_semaphore_destroy(_lf_sched_instance->semaphores[i]);
    }
    free(_lf_sched_instance->semaphores);
    free(_lf_sched_instance->reaction_instances);
    free(_lf_sched_instance->_lf_sched_array_of_conds);
    if (static_schedule_path != NULL)
        fclose(static_schedule_fp);
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
    LF_PRINT_DEBUG("Worker %d inside lf_sched_get_ready_reaction", worker_number);
    size_t* pc = &_lf_sched_instance->pc[worker_number];
    int schedule_index = _lf_sched_instance->current_schedule_index;
    const inst_t* current_schedule = _lf_sched_instance->static_schedules[schedule_index][worker_number];
    reaction_t** reaction_instances = _lf_sched_instance->reaction_instances;
    semaphore_t** semaphores = _lf_sched_instance->semaphores;
    reaction_t* returned_reaction = NULL;
    bool exit_loop = false;

    while (*pc < _lf_sched_instance->schedule_lengths[schedule_index][worker_number] && !exit_loop) {
        LF_PRINT_DEBUG("Current instruction for worker %d: %c %zu", worker_number, current_schedule[*pc].inst, current_schedule[*pc].op);
        switch (current_schedule[*pc].inst) {
        // If the instruction is Execute, return the reaction pointer and advance pc.
        case 'e':
        {
            reaction_t* react = reaction_instances[current_schedule[*pc].op];
            if (react->status == queued) {
                returned_reaction = react;
                exit_loop = true;
            } else
                LF_PRINT_DEBUG("Worker %d skip execution", worker_number);
            *pc += 1;
            break;
        }
        // If the instruction is Wait, block until the reaction is finished (by checking
        // the semaphore) and process the next instruction until we process an Execute.
        case 'w': // Wait
            lf_semaphore_wait(semaphores[current_schedule[*pc].op]);
            *pc += 1;
            break;
        case 'n': // Notify
            LF_PRINT_DEBUG("Releasing semaphore %d: %p",
                current_schedule[*pc].op,
                _lf_sched_instance->semaphores[current_schedule[*pc].op]);
            lf_semaphore_release(semaphores[current_schedule[*pc].op], 1);
            *pc += 1;
            break;
        // If the instruction is Stop, return NULL.
        case 's': // Stop
            LF_PRINT_DEBUG("Worker %d reaches a stop instruction", worker_number);
            // Check if the worker is the last worker to reach stop.
            // If so, this worker thread will take charge of advancing tag.
            // Ask the scheduler for more work and wait
            tracepoint_worker_wait_starts(worker_number);
            _lf_sched_wait_for_work(worker_number);
            tracepoint_worker_wait_ends(worker_number);
            exit_loop = _lf_sched_instance->_lf_sched_should_stop;
            break;
        }
    };
    LF_PRINT_DEBUG("Worker %d leaves lf_sched_get_ready_reaction", worker_number);
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
        lf_print_error_and_exit("Unexpected reaction status: %d. Expected %d.",
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