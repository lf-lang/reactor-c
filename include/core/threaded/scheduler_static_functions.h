#ifndef SCHEDULER_STATIC_FUNCTION_H
#define SCHEDULER_STATIC_FUNCTION_H

/**
 * @brief Function type with a void* argument. To make this type represent a
 * generic function, one can write a wrapper function around the target function
 * and use the first argument as a pointer to a struct of input arguments
 * and return values.
 */
typedef void(*function_generic_t)(void*);

/**
 * @brief Wrapper function for peeking a priority queue.
 */
void push_pop_peek_pqueue(void* self);

void execute_inst_ADD(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_ADDI(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_ADV(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_ADVI(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_BEQ(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_BGE(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_BLT(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_BNE(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_DU(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_EXE(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_WLT(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_WU(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_JAL(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_JALR(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);
void execute_inst_STP(lf_scheduler_t* scheduler, size_t worker_number, operand_t op1, operand_t op2, operand_t op3, bool debug, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop);

#endif