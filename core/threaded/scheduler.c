#if SCHEDULER == ADAPTIVE
#include "scheduler_adaptive.c"

#elif SCHEDULER == GEDF_NP_CI
#include "scheduler_GEDF_NP_CI.c"

#elif SCHEDULER == GEDF_NP
#include "scheduler_GEDF_NP.c"

#else
#include "scheduler_NP.c"

#endif
