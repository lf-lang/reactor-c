/**
 * @file instruction_QS.h
 * @author Shaokai Lin <shaokai@eecs.berkeley.edu>
 * @brief Format of the instruction set
 */
#ifdef SCHEDULER_QS

#include <stdint.h>

typedef struct {
    char inst;  // Exec, Wait, Notify, Stop
    size_t op; // Operand
} inst_t;

#endif // SCHEDULER_QS