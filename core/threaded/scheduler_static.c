#if defined(LF_THREADED)
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
 * A static scheduler for the threaded runtime of the C target of Lingua Franca.
 *
 * @author{Shaokai Lin <shaokai@berkeley.edu>}
 */
#include "lf_types.h"
#if SCHEDULER == STATIC || (!defined(SCHEDULER) && defined(LF_THREADED))
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include <assert.h>

#include "platform.h"
#include "reactor_common.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "semaphore.h"
#include "trace.h"
#include "util.h"

/////////////////// External Variables /////////////////////////
// Global variable defined in tag.c:
extern instant_t start_time;

// Global variables defined in schedule.c:
extern const inst_t* static_schedules[];
extern const long long int hyperperiod;
extern volatile uint32_t hyperperiod_iterations[];
extern volatile uint32_t counters[];
extern const size_t num_counters;

/////////////////// Scheduler Private API /////////////////////////
/**
 * @brief If there is work to be done, notify workers individually.
 *
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers(lf_scheduler_t* scheduler) {
    // Note: All threads are idle. Therefore, there is no need to lock the mutex
    // while accessing the executing queue (which is pointing to one of the
    // reaction queues).
    size_t workers_to_awaken = 
        scheduler->number_of_idle_workers;
    LF_PRINT_DEBUG("Scheduler: Notifying %zu workers.", workers_to_awaken);
    scheduler->number_of_idle_workers -= workers_to_awaken;
    LF_PRINT_DEBUG("Scheduler: New number of idle workers: %zu.",
                scheduler->number_of_idle_workers);
    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this
        // function.
        lf_semaphore_release(scheduler->semaphore,
                             (workers_to_awaken - 1));
    }
}

/**
 * @brief Wait until the scheduler assigns work.
 *
 * If the calling worker thread is the last to become idle, it will call on the
 * scheduler to distribute work. Otherwise, it will wait on
 * 'scheduler->semaphore'.
 * This implementation of _lf_sched_wait_for_work also takes on the role of
 * advancing time for all reactors at the end of the hyperperiod.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 * @param next_timestamp The next timestamp all reactors advance to.
 */
void _lf_sched_wait_for_work(
    lf_scheduler_t* scheduler,
    size_t worker_number,
    instant_t next_timestamp
) {
    // Increment the number of idle workers by 1 and
    // check if this is the last worker thread to become idle.
    if (lf_atomic_add_fetch(&scheduler->number_of_idle_workers,
                            1) ==
        scheduler->number_of_workers) {
        
        // Last thread to go idle
        LF_PRINT_DEBUG("Scheduler: Worker %zu is the last idle thread.",
                    worker_number);
        
        // The last worker advances all reactors to the next tag.
        for (int j = 0; j < scheduler->num_reactor_self_instances; j++) {
            scheduler->reactor_self_instances[j]->tag.time = next_timestamp;
            scheduler->reactor_self_instances[j]->tag.microstep = 0;
        }
        
        // The last worker clears all the counters.
        for (int i = 0; i < num_counters; i++) {
            counters[i] = 0;
        }
        
        // The last worker calls on the scheduler to distribute work or advance tag.
        _lf_sched_notify_workers(scheduler);

    } else {
        // Not the last thread to become idle.
        // Wait for work to be released.
        lf_semaphore_acquire(scheduler->semaphore);
    }
}

/**
 * @brief BIT: Branch If Timeout
 * Check if timeout is reached. If not, don't do anything.
 * If so, jump to a specified location (rs1).
 * 
 * FIXME: Should the timeout value be an operand?
 * FIXME: Use a global variable num_active_reactors instead of iterating over
 * a for loop.
 */
void execute_inst_BIT(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_BIT_starts(scheduler->env->trace, worker_number, (int) *pc);
    bool stop = true;
    for (int i = 0; i < scheduler->num_reactor_self_instances; i++) {
        if (!scheduler->reactor_reached_stop_tag[i]) {
            stop = false;
            break;
        }
    }
    if (stop) *pc = rs1;    // Jump to a specified location.
    else *pc += 1;          // Increment pc.
    tracepoint_static_scheduler_BIT_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief EIT: "Execute-If-Triggered"
 * Check if the reaction status is "queued."
 * If so, return the reaction pointer and advance pc.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_EIT(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_EIT_starts(scheduler->env->trace, worker_number, (int) *pc);
    reaction_t* reaction = scheduler->reaction_instances[rs1];
    if (reaction->status == queued) {
        *returned_reaction = reaction;
        *exit_loop = true;
    } else
        LF_PRINT_DEBUG("*** Worker %zu skip execution", worker_number);
    *pc += 1; // Increment pc.
    tracepoint_static_scheduler_EIT_ends(scheduler->env->trace, worker_number, (int) *pc);

}

/**
 * @brief EXE: Execute a reaction
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_EXE(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_EXE_starts(scheduler->env->trace, worker_number, (int) *pc);
    reaction_t* reaction = scheduler->reaction_instances[rs1];
    *returned_reaction = reaction;
    *exit_loop = true;
    *pc += 1; // Increment pc.
    tracepoint_static_scheduler_EXE_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief DU: Delay Until a physical time offset (rs1) wrt the current hyperperiod is reached.
 * 
 * @param worker_number 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_DU(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_DU_starts(scheduler->env->trace, worker_number, (int) *pc);
    // FIXME: There seems to be an overflow problem.
    // When wakeup_time overflows but lf_time_physical() doesn't,
    // _lf_interruptable_sleep_until_locked() terminates immediately. 
    instant_t wakeup_time = start_time + hyperperiod * (*iteration) + rs1;
    LF_PRINT_DEBUG("start_time: %lld, wakeup_time: %lld, rs1: %lld, iteration: %d, current_physical_time: %lld, hyperperiod: %lld\n", start_time, wakeup_time, rs1, (*iteration), lf_time_physical(), hyperperiod);
    LF_PRINT_DEBUG("*** Worker %zu delaying", worker_number);
    _lf_interruptable_sleep_until_locked(scheduler->env, wakeup_time);
    LF_PRINT_DEBUG("*** Worker %zu done delaying", worker_number);
    *pc += 1; // Increment pc.
    tracepoint_static_scheduler_DU_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief WU: Wait until a counting variable reaches a specified value.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_WU(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_WU_starts(scheduler->env->trace, worker_number, (int) *pc);
    LF_PRINT_DEBUG("*** Worker %zu waiting", worker_number);
    while(scheduler->counters[rs1] < rs2);
    LF_PRINT_DEBUG("*** Worker %zu done waiting", worker_number);
    *pc += 1; // Increment pc.
    tracepoint_static_scheduler_WU_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief ADV: Advance time for a reactor up to a tag (relative to the current hyperperiod).
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_ADV(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_ADV_starts(scheduler->env->trace, worker_number, (int) *pc);

    // This mutex is quite expensive.
    lf_mutex_lock(&(scheduler->env->mutex));

    self_base_t* reactor =
        scheduler->reactor_self_instances[rs1];
    reactor->tag.time = hyperperiod * (*iteration) + rs2;
    reactor->tag.microstep = 0;

    // Reset all "is_present" fields of the output ports of the reactor
    // Doing this here has the major implicatio  that ADV has to execute AFTER 
    // all downstream reactions have finished. Since it is modifying state that is
    // visible to thos reactions.
    for (int i = 0; i<reactor->num_output_ports; i++) {
        reactor->output_ports[i]->is_present = false;
    }

    if (_lf_is_tag_after_stop_tag(scheduler->env, reactor->tag)) {
        scheduler->reactor_reached_stop_tag[rs1] = true;
    }
   
    lf_mutex_unlock(&(scheduler->env->mutex));

    *pc += 1; // Increment pc.

    tracepoint_static_scheduler_ADV_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief ADV: Advance time for a reactor up to a tag (relative to the current hyperperiod).
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_ADV2(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_ADV2_starts(scheduler->env->trace, worker_number, (int) *pc);

    self_base_t* reactor =
        scheduler->reactor_self_instances[rs1];
    reactor->tag.time = hyperperiod * (*iteration) + rs2;
    reactor->tag.microstep = 0;
    
    // Reset all "is_present" fields of the output ports of the reactor
    // Doing this here has the major implicatio  that ADV has to execute AFTER 
    // all downstream reactions have finished. Since it is modifying state that is
    // visible to thos reactions.
    for (int i = 0; i<reactor->num_output_ports; i++) {
        reactor->output_ports[i]->is_present = false;
    }

    if (_lf_is_tag_after_stop_tag(scheduler->env, reactor->tag)) {
        scheduler->reactor_reached_stop_tag[rs1] = true;
    }
   
    *pc += 1; // Increment pc.

    tracepoint_static_scheduler_ADV2_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief JMP: Jump to a particular line in the schedule.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_JMP(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_JMP_starts(scheduler->env->trace, worker_number, (int) *pc);
    if (rs2 != -1) *iteration += 1;
    *pc = rs1;
    tracepoint_static_scheduler_JMP_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief SAC: (Sync-Advance-Clear) synchronize all workers until all execute SAC
 * and let the last idle worker reset all counters to 0.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_SAC(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_SAC_starts(scheduler->env->trace, worker_number, (int) *pc);

    // Compute the next tag for all reactors.
    instant_t next_timestamp = hyperperiod * (*iteration) + rs1;
    
    tracepoint_worker_wait_starts(scheduler->env->trace, worker_number);
    _lf_sched_wait_for_work(scheduler, worker_number, next_timestamp);
    tracepoint_worker_wait_ends(scheduler->env->trace, worker_number);
    *pc += 1; // Increment pc.

    tracepoint_static_scheduler_SAC_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief INC: INCrement a counter (rs1) by an amount (rs2).
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_INC(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_INC_starts(scheduler->env->trace, worker_number, (int) *pc);
    lf_mutex_lock(&(scheduler->env->mutex));
    scheduler->counters[rs1] += rs2;
    lf_mutex_unlock(&(scheduler->env->mutex));
    *pc += 1; // Increment pc.
    tracepoint_static_scheduler_INC_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief INC2: [Lock-free] INCrement a counter (rs1) by an amount (rs2).
 * The compiler needs to guarantee a single writer.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_INC2(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_INC2_starts(scheduler->env->trace, worker_number, (int) *pc);
    scheduler->counters[rs1] += rs2;
    *pc += 1; // Increment pc.
    tracepoint_static_scheduler_INC2_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief STP: SToP the execution.
 * 
 */
void execute_inst_STP(lf_scheduler_t* scheduler, size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    tracepoint_static_scheduler_STP_starts(scheduler->env->trace, worker_number, (int) *pc);
    *exit_loop = true;
    tracepoint_static_scheduler_STP_ends(scheduler->env->trace, worker_number, (int) *pc);
}

/**
 * @brief Execute an instruction
 * 
 * @param op the opcode
 * @param rs1 the first operand
 * @param rs2 the second operand
 * @param pc a pointer to the program counter
 * @param returned_reaction a pointer to a reaction to be executed 
 * 
 * FIXME: This feels like a bad design in the abstraction.
 * @param exit_loop a pointer to a boolean indicating whether
 *                  the outer while loop should be exited
 */
void execute_inst(lf_scheduler_t* scheduler, size_t worker_number, opcode_t op, long long int rs1, long long int rs2,
    size_t* pc, reaction_t** returned_reaction, bool* exit_loop, volatile uint32_t* iteration) {
    char* op_str = NULL;
    switch (op) {
        case ADV:
            op_str = "ADV";
            execute_inst_ADV(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case ADV2:
            op_str = "ADV2";
            execute_inst_ADV2(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case BIT:
            op_str = "BIT";
            execute_inst_BIT(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
         case DU:  
            op_str = "DU";
            execute_inst_DU(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case EIT:
            op_str = "EIT";
            execute_inst_EIT(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case EXE:
            op_str = "EXE";
            execute_inst_EXE(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case INC:
            op_str = "INC";
            execute_inst_INC(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case INC2:
            op_str = "INC2";
            execute_inst_INC2(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case JMP:
            op_str = "JMP";
            execute_inst_JMP(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case SAC:
            op_str = "SAC";
            execute_inst_SAC(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case STP:
            op_str = "STP";
            execute_inst_STP(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case WU:
            op_str = "WU";
            execute_inst_WU(scheduler, worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        default:
            lf_print_error_and_exit("Invalid instruction: %d", op);
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
    environment_t *env,
    size_t number_of_workers,
    sched_params_t* params
) {
    LF_PRINT_DEBUG("Scheduler: Initializing with %zu workers", number_of_workers);
        
    // Scheduler already initialized
    if (!init_sched_instance(env, &env->scheduler, number_of_workers, params)) {
        // FIXME: This is not the best practice and seems to take advantage of a
        //        bug in the runtime.
        //        lf_sched_init() is for some reason called twice.
        //        Once in lf_reactor_c_main() in reactor_threaded.c.
        //        Another in initialize() -> _lf_initialize_trigger_objects()
        //        -> lf_sched_init(), also in reactor_threaded.c.
        //        This implementation takes advantage of the fact that when
        //        lf_sched_init() is called the second time, start_time is set
        //        to a meaningful value. When the first time lf_sched_init() is
        //        called, start_time has not been set.

        // Initialize the local tags for the STATIC scheduler.
        for (int i = 0; i < env->scheduler->num_reactor_self_instances; i++) {
            env->scheduler->reactor_self_instances[i]->tag.time = start_time;
            env->scheduler->reactor_self_instances[i]->tag.microstep = 0;
        }

        // Already initialized
        return;
    }

    env->scheduler->pc = calloc(number_of_workers, sizeof(size_t));
    env->scheduler->static_schedules = &static_schedules[0];
    env->scheduler->reaction_instances = params->reaction_instances;
    env->scheduler->reactor_self_instances = params->reactor_self_instances;
    env->scheduler->num_reactor_self_instances = params->num_reactor_self_instances;
    env->scheduler->reactor_reached_stop_tag = params->reactor_reached_stop_tag;
    env->scheduler->counters = counters;
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free(lf_scheduler_t* scheduler) {
    LF_PRINT_DEBUG("Freeing the pointers in the scheduler struct.");
    free(scheduler->pc);
    free(scheduler->reactor_self_instances);
    free(scheduler->reaction_instances);
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
reaction_t* lf_sched_get_ready_reaction(lf_scheduler_t* scheduler, int worker_number) {
    LF_PRINT_DEBUG("Worker %d inside lf_sched_get_ready_reaction", worker_number);
    
    const inst_t*   current_schedule    = scheduler->static_schedules[worker_number];
    reaction_t*     returned_reaction   = NULL;
    bool            exit_loop           = false;
    size_t*         pc                  = &scheduler->pc[worker_number];
    opcode_t        op;
    long long int   rs1;
    long long int   rs2;
    volatile uint32_t* iteration        = &hyperperiod_iterations[worker_number];

    while (!exit_loop) {
        op  = current_schedule[*pc].op;
        rs1 = current_schedule[*pc].rs1;
        rs2 = current_schedule[*pc].rs2;

        // Execute the current instruction
        execute_inst(scheduler, worker_number, op, rs1, rs2, pc,
                    &returned_reaction, &exit_loop, iteration);

        LF_PRINT_DEBUG("Worker %d: returned_reaction = %p, exit_loop = %d",
                        worker_number, returned_reaction, exit_loop);
    }

    LF_PRINT_DEBUG("Worker %d leaves lf_sched_get_ready_reaction", worker_number);
    return returned_reaction;
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 *
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number,
                                 reaction_t* done_reaction) {
    LF_PRINT_DEBUG("*** Worker %zu inside lf_sched_done_with_reaction, done with %s", worker_number, done_reaction->name);
    // If the reaction status is queued, change it back to inactive.
    // We do not check for error here because the EXE instruction
    // can execute a reaction with an "inactive" status.
    // The reason is that since runtime does not advance
    // global time, the next timer events will not be
    // scheduled and put onto the event queue. The next
    // timer events are encoded directly into the schedule
    // using the EXE instructions.
    lf_bool_compare_and_swap(&done_reaction->status, queued, inactive);

    LF_PRINT_DEBUG("*** Worker %zu reports updated status for %s: %u", worker_number, done_reaction->name, done_reaction->status);
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 *
 * If a worker number is not available (e.g., this function is not called by a
 * worker thread), -1 should be passed as the 'worker_number'.
 *
 * This scheduler ignores the worker number.
 *
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 *
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should
 *  be used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 *
 */
void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number) {
    LF_PRINT_DEBUG("*** Worker %d triggering reaction %s", worker_number, reaction->name);
    // Mark a reaction as queued, so that it will be executed when workers do work.
    if (!lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        // FIXME: Uncommenting the code below yields weird exception.
        // lf_print_error_and_exit("Worker %d reports unexpected reaction status for reaction %s: %d. Expected %d.",
        //                         worker_number, reaction->name,
        //                         reaction->status, inactive);
    }
}
#endif
#endif
