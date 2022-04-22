/* The generated schedule */
#include <stdio.h>
#include <stdint.h>
#include "instruction_QS.h"

/* Can be auto-generated */
// The total number of reactions
static const int reaction_count = 4;

// Schedule 1: startup is present.
static const inst_t s1_w1[] = { {.inst='e', .nid=0, .loc=2 }, {.inst='e', .nid=1, .loc=2 }, {.inst='s', .nid=0, .loc=0 } };
static const inst_t s1_w2[] = { {.inst='s', .nid=0, .loc=0 } };
static const inst_t s1_w3[] = { {.inst='s', .nid=0, .loc=0 } };
// Schedule 2: logical action is present.
static const inst_t s2_w1[] = { {.inst='e', .nid=2, .loc=2 }, {.inst='e', .nid=3, .loc=2 }, {.inst='s', .nid=0, .loc=0 } };
static const inst_t s2_w2[] = { {.inst='s', .nid=0, .loc=0 } };
static const inst_t s2_w3[] = { {.inst='s', .nid=0, .loc=0 } };

static const inst_t* s1[] = { s1_w1, s1_w2, s1_w3 };
static const inst_t* s2[] = { s2_w1, s2_w2, s2_w3 };

static const inst_t** schedules[] = { s1, s2 };
static const uint32_t s1_length[] = {3, 1, 1};
static const uint32_t s2_length[] = {3, 1, 1};
static const uint32_t* lengths[] = {s1_length, s2_length};
/*************************/

int main(int argc, char *argv[]) {
    int schedule_index = 1;
    const inst_t** current_schedule = schedules[schedule_index];
    printf("Current schedule index: %d\n", schedule_index);
    for (int w = 0; w < 3; w++) {
        printf("Worker %d's schedule:\n", w);
        for (int pc = 0; pc < lengths[schedule_index][w]; pc++) {
            printf("%c %zu %zu\n", current_schedule[w][pc].inst, current_schedule[w][pc].nid, current_schedule[w][pc].loc);
        }
    }
}