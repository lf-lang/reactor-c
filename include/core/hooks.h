#ifndef HOOKS_H
#define HOOKS_H
#include <string.h>
#include <stdint.h>

#include "c-ordering-client.h"

typedef int64_t interval_t;

int lf_sleep(interval_t sleep_duration);

extern OrderingClientApi* ordering_client_api;
extern ClientAndJoinHandle ordering_client_and_join_handle;

#if !defined(LF_SINGLE_THREADED)
#include "platform.h"
#endif

#define LF_DO_HOOK_START(trace) \
    { \
        lf_mutex_lock(&trace->mutex); \
        char lf_hook_location_id[120]; \
        int lf_hook_line; \
        static volatile int lf_hook_sequence_number_volatile = 0; \
        int lf_hook_sequence_number = lf_atomic_fetch_add(&lf_hook_sequence_number_volatile, 1); \
        lf_hook_line = __LINE__; \
        /* snprintf(lf_hook_location_id, 120, "%s %d %d", lf_hook_file, lf_hook_line, _lf_my_fed_id); */ \
        snprintf(lf_hook_location_id, 120, "%d %d", lf_hook_line, _lf_my_fed_id); /* FIXME: it currently isn't necessary to use the file name simply because there is only one file per executable that has hooks. This could change. and FIXME: This snprintf might be a significant inefficiency. */ \

#define LF_DO_HOOK_END(trace) \
        ordering_client_api->tracepoint_maybe_do(ordering_client_and_join_handle.client, &lf_hook_location_id[0], _lf_my_fed_id, lf_hook_sequence_number); \
        lf_mutex_unlock(&trace->mutex); \
    }
#endif // HOOKS_H
