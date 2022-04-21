/* Should be auto-generated */
#include "instruction_QS.h"

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

static const inst_t** static_schedules[] = { s1, s2 };

static const uint32_t s1_length[] = {3, 1, 1};
static const uint32_t s2_length[] = {3, 1, 1};
static const uint32_t* schedule_lengths[] = { s1_length, s2_length };