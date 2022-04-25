/**
 * @file instruction_QS.h
 * @author Shaokai Lin <shaokai@eecs.berkeley.edu>
 * @brief Format of the instruction set
 */
#ifndef SCHEDULER_QS
#define SCHEDULER_QS

#include <stdint.h>
#include "../reactor.h"

typedef struct {
    char inst;  // Exec, Wait, Stop, Branch
    size_t op1; // Operand 1
    size_t op2; // Operand 2
} inst_t;

#endif // SCHEDULER_QS