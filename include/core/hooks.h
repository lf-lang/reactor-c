#ifndef HOOKS_H
#define HOOKS_H
#include <string.h>
#include <stdint.h>

#include "c-ordering-client.h"

typedef int64_t interval_t;

int lf_sleep(interval_t sleep_duration);

extern OrderingClientApi* ordering_client_api;
extern void* ordering_client;

#if !defined(LF_SINGLE_THREADED)
#include "platform.h"
#endif

static inline char* get_file_name() {
  char* current_file = strrchr(__FILE__, '/');
  if (current_file == NULL) {
    current_file = __FILE__;
  } else {
    current_file++;
  }
  return current_file;
}

static inline int get_file_idx(trace_t* trace, int worker, char* file_name) {
    int idx = (worker < 0) ? 0 : worker;
    int i;
    for (i = 0; i < trace->_lf_file_names_size[idx]; i++) {
        if (strcmp(trace->_lf_file_names[idx][i], file_name) == 0) {
            return i;
        }
    }
    // Not found. Add it.
    trace->_lf_file_names[idx][trace->_lf_file_names_size[idx]] = file_name;
    trace->_lf_file_names_size[idx]++;
    return trace->_lf_file_names_size[idx] - 1;
}

#define LF_DO_HOOK_START(trace) \
    { \
        char lf_hook_location_id[120]; \
        int lf_hook_line; \
        static volatile int lf_hook_sequence_number_volatile = 0; \
        int lf_hook_sequence_number = lf_atomic_fetch_add(&lf_hook_sequence_number_volatile, 1); \
        lf_hook_line = __LINE__; \
        /* snprintf(lf_hook_location_id, 120, "%s %d %d", lf_hook_file, lf_hook_line, _lf_my_fed_id); */ \
        snprintf(lf_hook_location_id, 120, "%d %d", lf_hook_line, _lf_my_fed_id); /* FIXME: it currently isn't necessary to use the file name simply because there is only one file per executable that has hooks. This could change. and FIXME: This snprintf might be a significant inefficiency. */ \

#define LF_DO_HOOK_END \
        ordering_client_api->tracepoint_maybe_do(ordering_client, &lf_hook_location_id[0], _lf_my_fed_id, lf_hook_sequence_number); \
    }
#endif // HOOKS_H
