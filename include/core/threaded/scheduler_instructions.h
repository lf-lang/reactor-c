/**
 * @brief Format of the instruction set
 */
#ifndef SCHEDULER_INSTRUCTIONS_H
#define SCHEDULER_INSTRUCTIONS_H

typedef enum {
    ADD,
    ADDI,
    ADV,
    ADVI,
    BEQ,
    BGE,
    BLT,
    BNE,
    DU,
    EXE,
    JAL,
    JALR,
    STP,
    WLT,
    WU,
} opcode_t;


/**
 * @brief Convenient typedefs for the data types used by the C implementation of
 * PRET VM. A register is 64bits and an immediate is 64bits. This avoids any
 * issue with time and overflow. Arguably it is worth it even for smaller
 * platforms.
 *
 */
typedef volatile uint64_t reg_t;
typedef uint64_t imm_t;

/**
 * @brief An union representing a single operand for the PRET VM. A union
 * means that we have one piece of memory, which is big enough to fit either
 * one of the two members of the union.
 * 
 */
typedef union {
    reg_t* reg;
    imm_t imm;
} operand_t;

/**
 * @brief Virtual instruction function pointer
 */
typedef void (*function_virtual_instruction_t)(
    lf_scheduler_t* scheduler, 
    size_t worker_number, 
    operand_t op1, 
    operand_t op2, 
    operand_t op3, 
    bool debug, 
    size_t* pc, 
    reaction_t** returned_reaction, 
    bool* exit_loop);

/**
 * @brief This struct represents a PRET VM instruction for C platforms.
 * There is an opcode and three operands. The operands are unions so they
 * can be either a pointer or an immediate
 * 
 */
typedef struct inst_t {
    function_virtual_instruction_t func;
    opcode_t opcode;
    operand_t op1;
    operand_t op2;
    operand_t op3;
    bool debug;
} inst_t;

#endif