
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

extern size_t num_levels;
extern size_t max_num_workers;

#define SLOW_EXPERIMENTS 256

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
        execution_times_by_level[level] = get_physical_time() - start_times_by_level[level];
    }
}

static size_t restrict_to_range(size_t start_inclusive, size_t end_inclusive, size_t value) {
    if (value < start_inclusive) return start_inclusive;
    if (value > end_inclusive) return end_inclusive;
    return value;
}

static void compute_number_of_workers(
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level,
    int jitter
) {
    for (size_t level = 0; level < num_levels; level++) {
        size_t ideal_number_of_workers;
        size_t max_reasonable_num_workers = max_num_workers_by_level[level];
        ideal_number_of_workers = execution_times_argmins[level] + jitter;
        num_workers_by_level[level] = restrict_to_range(
            1, max_reasonable_num_workers, ideal_number_of_workers
        );
        // printf("level=%ld, jitter=%d, inow=%ld, mrnw=%ld, result=%ld\n",
        // level, jitter, ideal_number_of_workers, max_reasonable_num_workers, num_workers_by_level[level]);
    }
}

static void data_collection_end_tag(
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level
) {
    if (collecting_data && execution_times_by_level[0]) {
        for (size_t level = 0; level < num_levels; level++) {
            interval_t score = execution_times_by_level[level];
            if (
                !execution_times_mins[level]
                | (score < execution_times_mins[level])
                | (num_workers_by_level[level] == execution_times_argmins[level])
            ) {
                printf(
                    "Argmin update: %ld(%ld) -> %ld(%ld) @ %ld\n",
                    execution_times_argmins[level], execution_times_mins[level],
                    num_workers_by_level[level], score,
                    level
                );
                execution_times_mins[level] = score;
                execution_times_argmins[level] = num_workers_by_level[level];
            }
        }
    }
    data_collection_counter++;
    size_t period = 2 + 128 * (data_collection_counter > SLOW_EXPERIMENTS);
    size_t state = data_collection_counter % period;
    if (state == 0) {
        compute_number_of_workers(
            num_workers_by_level,
            max_num_workers_by_level,
            ((int) (rand() % 3)) - 1
        );
        collecting_data = true;
        // printf("collecting data");
    } else if (state == 1) {
        compute_number_of_workers(num_workers_by_level, max_num_workers_by_level, 0);
        collecting_data = false;
        // printf("not collecting data");
    }
}

#endif
