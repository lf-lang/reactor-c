#if !defined(LF_SINGLE_THREADED)
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
#include <inttypes.h>
#include "lf_types.h"
#if defined SCHEDULER && SCHEDULER == SCHED_STATIC
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include <assert.h>

#include "platform.h"
#include "reactor_common.h"
#include "scheduler_instance.h"
#include "scheduler_static_functions.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "tag.h"
#include "tracepoint.h"
#include "util.h"

#ifndef TRACE_ALL_INSTRUCTIONS
#define TRACE_ALL_INSTRUCTIONS false
#endif
#define SPIN_WAIT_THRESHOLD SEC(1)

/////////////////// External Variables /////////////////////////
// Global variable defined in tag.c:
extern instant_t start_time;

// Global variables defined in schedule.c:
extern const inst_t* static_schedules[];
extern reg_t timeout;
extern const size_t num_counters;
extern reg_t time_offset;
extern reg_t offset_inc;
extern const uint64_t zero;
extern volatile uint32_t counters[];
extern volatile reg_t return_addr[];
extern volatile reg_t binary_sema[];

/////////////////// Scheduler Private API /////////////////////////

/**
 * @brief The implementation of the ADD instruction
 */
void execute_inst_ADD(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "ADD";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_ADD_starts(worker_number, pc_orig);
#endif
    reg_t *dst = op1.reg;
    reg_t *src = op2.reg;
    reg_t *src2 = op3.reg;
    *dst = *src + *src2;
    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_ADD_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the ADDI instruction
 */
void execute_inst_ADDI(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "ADDI";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_ADDI_starts(worker_number, pc_orig);
#endif
    reg_t *dst = op1.reg;
    reg_t *src = op2.reg;
    // FIXME: Will there be problems if instant_t adds reg_t?
    *dst = *src + op3.imm;
    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_ADDI_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the ADV instruction
 */
void execute_inst_ADV(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "ADV";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_ADV_starts(worker_number, pc_orig);
#endif
    reg_t *base = op2.reg;
    reg_t *inc  = op3.reg;
    self_base_t* reactor =
        scheduler->reactor_self_instances[op1.imm];
    reactor->tag.time = *base + *inc;
    reactor->tag.microstep = 0;

    // Reset all "is_present" fields of the output ports of the reactor
    // Doing this here has the major implication that ADV has to execute AFTER 
    // all downstream reactions have finished, since it is modifying state that is
    // visible to those reactions.
    for (int i = 0; i < reactor->num_output_ports; i++) {
        reactor->output_ports[i]->is_present = false;
    }

    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_ADV_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the ADVI instruction
 */
void execute_inst_ADVI(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "ADVI";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s (reactor %p) %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.reg, *(op2.reg), op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_ADVI_starts(worker_number, pc_orig);
#endif
    self_base_t *reactor = (self_base_t*) op1.reg;
    reg_t *base = op2.reg;
    reactor->tag.time = *base + op3.imm;
    reactor->tag.microstep = 0;

    // Reset all "is_present" fields of the output ports of the reactor
    // Doing this here has the major implication that ADVI has to execute AFTER 
    // all downstream reactions have finished, since it is modifying state that is
    // visible to those reactions.
    for (int i = 0; i < reactor->num_output_ports; i++) {
        reactor->output_ports[i]->is_present = false;
    }

    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_ADVI_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the BEQ instruction
 */
void execute_inst_BEQ(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "BEQ";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_BEQ_starts(worker_number, pc_orig);
#endif
    reg_t *_op1 = op1.reg;
    reg_t *_op2 = op2.reg;
    // These NULL checks allow _op1 and _op2 to be uninitialized in the static
    // schedule, which can save a few lines in the schedule. But it is debatable
    // whether this is good practice.
    if (debug) {
        lf_print("DEBUG: _op1 = %p, _op2 = %p", _op1, _op2);
        if (_op1 != NULL) lf_print("DEBUG: *_op1 = %lld", *_op1);
        if (_op2 != NULL) lf_print("DEBUG: *_op2 = %lld", *_op2);
    }
    if (_op1 != NULL && _op2 != NULL && *_op1 == *_op2) *pc = op3.imm;
    else *pc += 1;
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_BEQ_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the BGE instruction
 */
void execute_inst_BGE(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "BGE";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_BGE_starts(worker_number, pc_orig);
#endif
    reg_t *_op1 = op1.reg;
    reg_t *_op2 = op2.reg;
    LF_PRINT_DEBUG("Worker %zu: BGE : operand 1 = %lld, operand 2 = %lld", worker_number, *_op1, *_op2);
    if (_op1 != NULL && _op2 != NULL && *_op1 >= *_op2) *pc = op3.imm;
    else *pc += 1;
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_BGE_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the BLT instruction
 */
void execute_inst_BLT(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "BLT";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_BLT_starts(worker_number, pc_orig);
#endif
    reg_t *_op1 = op1.reg;
    reg_t *_op2 = op2.reg;
    if (_op1 != NULL && _op2 != NULL && *_op1 < *_op2) *pc = op3.imm;
    else *pc += 1;
    if (debug) lf_print("op1: %lld, op2: %lld, op1 < op2: %d", *_op1, *_op2, *_op1 < *_op2);
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_BLT_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the BNE instruction
 */
void execute_inst_BNE(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "BNE";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_BNE_starts(worker_number, pc_orig);
#endif
    reg_t *_op1 = op1.reg;
    reg_t *_op2 = op2.reg;
    if (_op1 != NULL && _op2 != NULL && *_op1 != *_op2) *pc = op3.imm;
    else *pc += 1;
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_BNE_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the DU instruction
 */
void execute_inst_DU(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "DU";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_DU_starts(worker_number, pc_orig);
#endif
    // FIXME: There seems to be an overflow problem.
    // When wakeup_time overflows but lf_time_physical() doesn't,
    // _lf_interruptable_sleep_until_locked() terminates immediately.
    reg_t *src = op1.reg;
    instant_t current_time = lf_time_physical();
    instant_t wakeup_time = *src + op2.imm;
    LF_PRINT_DEBUG("DU wakeup time: %lld, base: %lld, offset: %lld", wakeup_time, *src, op2.imm);
    instant_t wait_interval = wakeup_time - current_time;
    // LF_PRINT_DEBUG("*** start_time: %lld, wakeup_time: %lld, op1: %lld, op2: %lld, current_physical_time: %lld\n", start_time, wakeup_time, *src, op2.imm, lf_time_physical());
    LF_PRINT_DEBUG("*** [Line %zu] Worker %zu delaying, current_physical_time: %lld, wakeup_time: %lld, wait_interval: %lld", *pc, worker_number, current_time, wakeup_time, wait_interval);
    if (wait_interval > 0) {
        // Approach 1: Only spin when the wait interval is less than SPIN_WAIT_THRESHOLD.
        if (wait_interval < SPIN_WAIT_THRESHOLD) {
            // Spin wait if the wait interval is less than 1 ms.
            while (lf_time_physical() < wakeup_time);
        } else {
            // Otherwise sleep.
            _lf_interruptable_sleep_until_locked(scheduler->env, wakeup_time);
        }
        // Approach 2: Spin wait.
        // while (lf_time_physical() < wakeup_time);
    }
    LF_PRINT_DEBUG("*** [Line %zu] Worker %zu done delaying", *pc, worker_number);
    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_DU_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the EXE instruction
 */
void execute_inst_EXE(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "EXE";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    if (op3.imm != ULLONG_MAX) {tracepoint_static_scheduler_EXE_reaction_starts((void*)op2.reg, worker_number, op3.imm);}
    else {tracepoint_static_scheduler_EXE_starts(worker_number, pc_orig);}
#else
    if (op3.imm != ULLONG_MAX) {tracepoint_static_scheduler_EXE_reaction_starts((void*)op2.reg, worker_number, op3.imm);}
#endif
    function_generic_t function = (function_generic_t)(uintptr_t)op1.reg;
    void *args = (void*)op2.reg;
    // Execute the function directly.
    LF_PRINT_DEBUG("*** [Line %zu] Worker %zu executing reaction", *pc, worker_number);
    function(args);
    LF_PRINT_DEBUG("*** [Line %zu] Worker %zu done executing reaction", *pc, worker_number);
    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    if (op3.imm != ULLONG_MAX) {tracepoint_static_scheduler_EXE_reaction_ends((void*)op2.reg, worker_number, op3.imm);}
    else {tracepoint_static_scheduler_EXE_ends(worker_number, pc_orig);}
#else 
    if (op3.imm != ULLONG_MAX) {tracepoint_static_scheduler_EXE_reaction_ends((void*)op2.reg, worker_number, op3.imm);}
#endif
}


/**
 * @brief The implementation of the WLT instruction
 */
void execute_inst_WLT(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "WLT";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_WLT_starts(worker_number, pc_orig);
#endif
    LF_PRINT_DEBUG("*** Worker %zu waiting", worker_number);
    reg_t *var = op1.reg;
    while(*var >= op2.imm);
    LF_PRINT_DEBUG("*** Worker %zu done waiting", worker_number);
    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_WLT_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the WU instruction
 */
void execute_inst_WU(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "WU";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_WU_starts(worker_number, pc_orig);
#endif
    LF_PRINT_DEBUG("*** Worker %zu waiting", worker_number);
    reg_t *var = op1.reg;
    while(*var < op2.imm);
    LF_PRINT_DEBUG("*** Worker %zu done waiting", worker_number);
    *pc += 1; // Increment pc.
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_WU_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the JAL instruction
 */
void execute_inst_JAL(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "JAL";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_JAL_starts(worker_number, pc_orig);
#endif
    // Use the destination register as the return address and, if the
    // destination register is not the zero register, store pc+1 in it.
    reg_t *destReg = op1.reg;
    if (destReg != &zero) *destReg = *pc + 1;
    *pc = op2.imm + op3.imm; // New pc = label + offset
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_JAL_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the JALR instruction
 */
void execute_inst_JALR(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "JALR";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_JALR_starts(worker_number, pc_orig);
#endif
    // Use the destination register as the return address and, if the
    // destination register is not the zero register, store pc+1 in it.
    reg_t *destReg = op1.reg;
    if (destReg != &zero) *destReg = *pc + 1;
    // Set pc to base addr + immediate.
    reg_t *baseAddr = op2.reg;
    *pc = *baseAddr + op3.imm;
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_JALR_ends(worker_number, pc_orig);
#endif
}

/**
 * @brief The implementation of the STP instruction
 */
void execute_inst_STP(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop) {
    char* op_str = "STP";
    LF_PRINT_DEBUG("*** Worker %zu executing instruction: [Line %zu] %s %" PRIu64 " %" PRIu64 " %" PRIu64, worker_number, *pc, op_str, op1.imm, op2.imm, op3.imm);
#if TRACE_ALL_INSTRUCTIONS
    int pc_orig = (int) *pc;
    tracepoint_static_scheduler_STP_starts(worker_number, pc_orig);
#endif
    *exit_loop = true;
#if TRACE_ALL_INSTRUCTIONS
    tracepoint_static_scheduler_STP_ends(worker_number, pc_orig);
#endif
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

        // Initialize the local tags for the SCHED_STATIC scheduler.
        for (int i = 0; i < env->scheduler->num_reactor_self_instances; i++) {
            env->scheduler->reactor_self_instances[i]->tag.time = start_time;
            env->scheduler->reactor_self_instances[i]->tag.microstep = 0;
        }

        // Already initialized
        return;
    }

    env->scheduler->pc = calloc(number_of_workers, sizeof(size_t));
    env->scheduler->static_schedules = &static_schedules[0];
    env->scheduler->counters = counters;

    initialize_static_schedule();
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

    function_virtual_instruction_t func;
    operand_t       op1;
    operand_t       op2;
    operand_t       op3;
    bool            debug;

    while (!exit_loop) {
        func = current_schedule[*pc].func;
        op1 = current_schedule[*pc].op1;
        op2 = current_schedule[*pc].op2;
        op3 = current_schedule[*pc].op3;
        debug = current_schedule[*pc].debug;

        // Execute the current instruction
        func(scheduler, worker_number, op1, op2, op3, debug, pc,
                    &returned_reaction, &exit_loop);
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
                                 reaction_t* done_reaction) {}

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
void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number) {}
#endif
#endif
