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
    char inst;  // Exec, Wait, Stop
    size_t nid; // Reaction ID.
    size_t loc; // Location to jump to.
} inst_t;

#endif // SCHEDULER_QS