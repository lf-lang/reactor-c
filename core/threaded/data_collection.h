
#ifndef DATA_COLLECTION
#define DATA_COLLECTION

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include "scheduler.h"

static interval_t* start_times_by_level;
static interval_t* execution_times_by_level;
static size_t* expected_num_reactions_by_level;
extern size_t num_levels;
extern size_t max_num_workers;

#define OPTIMAL_NANOSECONDS_WORK 32768

static void data_collection_init(sched_params_t* params) {
    size_t num_levels = params->num_reactions_per_level_size;
    start_times_by_level = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_by_level = (interval_t*) calloc(num_levels, sizeof(interval_t));
    expected_num_reactions_by_level = (interval_t*) calloc(num_levels, sizeof(interval_t));
    for (size_t i = 0; i < num_levels; i++) {
        expected_num_reactions_by_level[i] = params->num_reactions_per_level[i];
    }
}

static void data_collection_free() {
    free(start_times_by_level);
    free(execution_times_by_level);
    free(expected_num_reactions_by_level);
}

static void data_collection_start_level(
    size_t level, size_t* num_reactions_by_worker, size_t num_workers
) {
    size_t total = expected_num_reactions_by_level[level];
    for (int w = 0; w < num_workers; w++) {
        total += num_reactions_by_worker[w];
    }
    expected_num_reactions_by_level[level] = total >> 1;
    start_times_by_level[level] = get_physical_time();
}

static void data_collection_end_level(size_t level) {
    if (start_times_by_level[level]) {
        execution_times_by_level[level] = (
            3 * execution_times_by_level[level]
            + get_physical_time() - start_times_by_level[level]
        ) >> 2;
    }
}

static void data_collection_compute_number_of_workers(size_t* num_workers_by_level) {
    for (size_t level = 0; level < num_levels; level++) {
        size_t ideal_number_of_workers = execution_times_by_level[level] / OPTIMAL_NANOSECONDS_WORK;
        size_t max_reasonable_num_workers = (
            max_num_workers < expected_num_reactions_by_level[level] ? max_num_workers : (
            expected_num_reactions_by_level[level] < 1 ? 1 : expected_num_reactions_by_level[level]
        ));
        num_workers_by_level[level] = (ideal_number_of_workers < 1) ? 1 : (
            (ideal_number_of_workers > max_reasonable_num_workers) ? max_reasonable_num_workers :
            ideal_number_of_workers
        );
    }
}

#endif
