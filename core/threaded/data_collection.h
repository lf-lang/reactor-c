
#ifndef DATA_COLLECTION
#define DATA_COLLECTION

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include "scheduler.h"

static interval_t* start_times_by_level;
static interval_t* execution_times_by_level;
static interval_t* execution_times_mins;
static size_t* execution_times_argmins;
static size_t data_collection_counter = 0;
static bool collecting_data = false;
static int experimental_jitter = 0;

extern size_t num_levels;
extern size_t max_num_workers;

#define OPTIMAL_NANOSECONDS_WORK 32768

static void data_collection_init(sched_params_t* params) {
    size_t num_levels = params->num_reactions_per_level_size;
    start_times_by_level = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_by_level = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_mins = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_argmins = (size_t*) malloc(num_levels * sizeof(size_t));
    for (size_t i = 0; i < num_levels; i++) {
        execution_times_argmins[i] = max_num_workers;
    }
}

static void data_collection_free() {
    free(start_times_by_level);
    free(execution_times_by_level);
}

static void data_collection_start_level(size_t level) {
    if (collecting_data) start_times_by_level[level] = get_physical_time();
}

static void data_collection_end_level(size_t level, size_t num_workers) {
    if (collecting_data && start_times_by_level[level]) {
        execution_times_by_level[level] = (
            3 * execution_times_by_level[level]
            + get_physical_time() - start_times_by_level[level]
        ) >> 2;
        interval_t score = execution_times_by_level[level] + execution_times_by_level[
            (level + num_levels - 1) % num_levels
        ];
        if (!execution_times_mins[level] | (score < execution_times_mins[level])) {
            execution_times_mins[level] = score;
            execution_times_argmins[level] = num_workers;
        }
    }
    if (level == 0) {
        data_collection_counter++;
        int shift = (data_collection_counter > 8) << 3;
        size_t shifted = data_collection_counter >> shift;
        collecting_data = data_collection_counter == (shifted << shift);
        experimental_jitter = ((int) (shifted % 3)) - 1;
    }
}

static void data_collection_compute_number_of_workers(
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level
) {
    if (!collecting_data) return;
    for (size_t level = 0; level < num_levels; level++) {
        size_t ideal_number_of_workers = execution_times_by_level[level] / OPTIMAL_NANOSECONDS_WORK;
        ideal_number_of_workers = (ideal_number_of_workers + execution_times_argmins[level]) >> 1;
        ideal_number_of_workers += experimental_jitter;
        size_t max_reasonable_num_workers = max_num_workers_by_level[level];
        num_workers_by_level[level] = (ideal_number_of_workers < 1) ? 1 : (
            (ideal_number_of_workers > max_reasonable_num_workers) ? max_reasonable_num_workers :
            ideal_number_of_workers
        );
    }
}

#endif
