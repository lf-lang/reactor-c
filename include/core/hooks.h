#ifndef HOOKS_H
#define HOOKS_H
#include <string.h>
#include <stdint.h>

typedef int64_t interval_t;

int lf_sleep(interval_t sleep_duration);

typedef struct delay_pair_t {
    size_t sequence_number;
    interval_t delay;
} delay_pair_t;

typedef struct hook_delay_array_t {
    char* hook_id;
    delay_pair_t* delay_vector;
    size_t delay_vector_len;
} hook_delay_array_t;

#if !defined(LF_SINGLE_THREADED)
#include "platform.h"
#endif

typedef struct global_delay_array_t {
    hook_delay_array_t* hooks;
    size_t hooks_len;
#if !defined(LF_SINGLE_THREADED)
    lf_mutex_t mutex;
#endif
} global_delay_array_t;

extern global_delay_array_t _lf_global_delay_array;

/**
 * @brief Parse the global delay array from the environment variable.
 *
 * This must be invoked on startup.
 */
static inline void parse_global_delay_array(global_delay_array_t* gda) {
#if !defined(LF_SINGLE_THREADED)
    lf_mutex_init(&gda->mutex);
#endif
    char* gda_evar = getenv("LF_FED_DELAYS");
    if (!gda_evar || !(*gda_evar)) {
        gda->hooks = NULL;
        gda->hooks_len = 0;
        return;
    }
    FILE* fp = fopen(gda_evar, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s\n", gda_evar);
        exit(1);  // This is bad error handling, but that's fine.
    }
    char* line;
    char* saveptr;
    size_t len = 0;
    size_t read;
    getline(&line, &len, fp);
    gda->hooks_len = atoi(line);
    gda->hooks = (hook_delay_array_t*) malloc(sizeof(hook_delay_array_t) * gda->hooks_len);
    int idx = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        char* hook_id = strtok_r(line, "\n", &saveptr);
        gda->hooks[idx].hook_id = (char*) calloc(sizeof(char), ((((size_t) hook_id) && strlen(hook_id)) + 1));
        if (hook_id) strcpy(gda->hooks[idx].hook_id, hook_id);
        getline(&line, &len, fp);
        gda->hooks[idx].delay_vector_len = atoi(line);
        gda->hooks[idx].delay_vector = (delay_pair_t*) malloc(sizeof(delay_pair_t) * gda->hooks[idx].delay_vector_len);
        for (int idx2 = 0; idx2 < gda->hooks[idx].delay_vector_len; idx2++) {
            getline(&line, &len, fp);
            char* sequence_number = strtok_r(line, " ", &saveptr);
            char* delay = strtok_r(NULL, "\n", &saveptr);
            gda->hooks[idx].delay_vector[idx2].sequence_number = atoi(sequence_number);
            gda->hooks[idx].delay_vector[idx2].delay = atoi(delay);
        }
        idx++;
    }
    fclose(fp);
    if (line) free(line);
}

/**
 * @brief Finds the hook delay array for the given hook id.
 *
 * Assumes the delay array is sorted. Returns NULL if the hook ID is not found.
 */
static inline hook_delay_array_t* find_hook_delay_array(char* hook_id) {
    int low = 0;
    int high = _lf_global_delay_array.hooks_len - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = strcmp(_lf_global_delay_array.hooks[mid].hook_id, hook_id);
        if (cmp < 0) {
            low = mid + 1;
        } else if (cmp > 0) {
            high = mid - 1;
        } else {
            return &_lf_global_delay_array.hooks[mid];
        }
    }
    return NULL;
}

/**
 * @brief Locks the delay array if the program is threaded.
 */
static inline void lock_delay_array_if_threaded() {
#if !defined(LF_SINGLE_THREADED)
    lf_mutex_lock(&_lf_global_delay_array.mutex);
#endif
}

/**
 * @brief Unlocks the delay array if the program is threaded.
 */
static inline void unlock_delay_array_if_threaded() {
#if !defined(LF_SINGLE_THREADED)
    lf_mutex_unlock(&_lf_global_delay_array.mutex);
#endif
}

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
        lock_delay_array_if_threaded(); \
        static int lf_hook_location_id_set = 0; \
        static char lf_hook_location_id[120]; \
        static char* lf_hook_file; \
        static int lf_hook_file_idx; \
        static int lf_hook_line; \
        static hook_delay_array_t* lf_hook_delay_array; \
        static int lf_hook_sequence_number = 0; \
        static int lf_hook_delay_vector_index = 0; \
        if (!lf_hook_location_id_set) { /* slow path */ \
            size_t len = strlen(__FILE__); \
            lf_hook_file = get_file_name(__FILE__); \
            lf_hook_line = __LINE__; \
            lf_hook_file_idx = get_file_idx(trace, -1, lf_hook_file); \
            /* snprintf(lf_hook_location_id, 120, "%s %d %d", lf_hook_file, lf_hook_line, _lf_my_fed_id); */ \
            snprintf(lf_hook_location_id, 120, "%d %d", lf_hook_line, _lf_my_fed_id); /* FIXME: it currently isn't necessary to use the file name simply because there is only one file per executable that has hooks. This could change. */ \
            lf_hook_location_id_set = 1; \
        }

#define LF_DO_HOOK_END \
        if (!lf_hook_delay_array) { \
            lf_hook_delay_array = find_hook_delay_array(lf_hook_location_id); \
            if (!lf_hook_delay_array) { \
                lf_hook_delay_vector_index = -1; \
                lf_hook_delay_array = malloc(sizeof(hook_delay_array_t)); \
                *lf_hook_delay_array = (hook_delay_array_t) { \
                    .hook_id=NULL, \
                    .delay_vector=NULL, \
                    .delay_vector_len=0 \
                }; \
            } \
        } else if (lf_hook_delay_vector_index >= 0) { \
            if (lf_hook_delay_vector_index >= lf_hook_delay_array->delay_vector_len) { \
                lf_hook_delay_vector_index = -1; \
            } else if (lf_hook_delay_array->delay_vector[lf_hook_delay_vector_index].sequence_number == lf_hook_sequence_number) { \
                printf("DEBUG: sleeping for time %lld milliseconds\n", (long long) lf_hook_delay_array->delay_vector[lf_hook_delay_vector_index].delay); \
                lf_sleep(lf_hook_delay_array->delay_vector[lf_hook_delay_vector_index].delay * 1000000); \
                lf_hook_delay_vector_index++; \
            } \
        } \
        lf_hook_sequence_number++; \
        unlock_delay_array_if_threaded(); \
    }
#endif // HOOKS_H
