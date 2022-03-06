
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "scheduler.h"

static interval_t* start_times_by_level;
static interval_t* execution_times_by_level;
extern size_t num_levels;
extern size_t max_num_workers;

#define OPTIMAL_NANOSECONDS_WORK 65536

void data_collection_init(sched_params_t* params) {
    start_times_by_level = (interval_t*) calloc(
        params->num_reactions_per_level_size, sizeof(interval_t)
    );
    execution_times_by_level = (interval_t*) calloc(
        params->num_reactions_per_level_size, sizeof(interval_t)
    );
}

void data_collection_free() {
    free(start_times_by_level);
    free(execution_times_by_level);
}

void data_collection_start_level(size_t level) {
    start_times_by_level[level] = get_physical_time();
}

void data_collection_end_level(size_t level) {
    if (start_times_by_level[level]) {
        execution_times_by_level[level] = (
            3 * execution_times_by_level[level]
            + get_physical_time() - start_times_by_level[level]
        ) >> 2;
    }
}

void data_collection_compute_number_of_workers(size_t* num_workers_by_level) {
    for (size_t level = 0; level < num_levels; level++) {
        size_t ideal_number_of_workers = execution_times_by_level[level] / OPTIMAL_NANOSECONDS_WORK;
        num_workers_by_level[level] = (ideal_number_of_workers < 1) ? 1 : (
            (ideal_number_of_workers > max_num_workers) ? max_num_workers : ideal_number_of_workers
        );
    }
}
