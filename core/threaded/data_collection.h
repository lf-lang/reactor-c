
#ifndef DATA_COLLECTION
#define DATA_COLLECTION

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include <assert.h>
#include "scheduler.h"

static interval_t* start_times_by_level;
static interval_t** execution_times_by_num_workers_by_level;
static interval_t* execution_times_mins;
static size_t* execution_times_argmins;
static size_t data_collection_counter = 0;
static bool collecting_data = false;

extern size_t num_levels;
extern size_t max_num_workers;

#define SLOW_EXPERIMENTS 256
#define PARALLELISM_COST 20000
#define EXECUTION_TIME_MEMORY 15

static void data_collection_init(sched_params_t* params) {
    size_t num_levels = params->num_reactions_per_level_size;
    start_times_by_level = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_by_num_workers_by_level = (interval_t**) calloc(
        num_levels, sizeof(interval_t*)
    );
    execution_times_mins = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_argmins = (size_t*) malloc(num_levels * sizeof(size_t));
    for (size_t i = 0; i < num_levels; i++) {
        execution_times_argmins[i] = max_num_workers;
        execution_times_by_num_workers_by_level[i] = (interval_t*) calloc(
            max_num_workers, sizeof(interval_t)
        ) - 1;
    }
}

static void data_collection_free() {
    free(start_times_by_level);
    for (size_t i = 0; i < num_levels; i++) {
        free(execution_times_by_num_workers_by_level[i] + 1);
    }
    free(execution_times_by_num_workers_by_level);
}

static void data_collection_start_level(size_t level) {
    if (collecting_data) start_times_by_level[level] = get_physical_time();
}

static void data_collection_end_level(size_t level, size_t num_workers) {
    if (collecting_data && start_times_by_level[level]) {
        if (!execution_times_by_num_workers_by_level[level][num_workers]) {
            execution_times_by_num_workers_by_level[level][num_workers]
                = get_physical_time() - start_times_by_level[level];
            printf("Initialize.\n");
        } else {
            interval_t prior_et = execution_times_by_num_workers_by_level[level][num_workers];
            execution_times_by_num_workers_by_level[level][num_workers] = (
                execution_times_by_num_workers_by_level[level][num_workers] * EXECUTION_TIME_MEMORY
                + get_physical_time() - start_times_by_level[level]
            ) / (EXECUTION_TIME_MEMORY + 1);
        }
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
    bool jitter
) {
    for (size_t level = 0; level < num_levels; level++) {
        interval_t this_execution_time = execution_times_by_num_workers_by_level[level][
            num_workers_by_level[level]
        ];
        if (0 < this_execution_time && this_execution_time < PARALLELISM_COST && (rand() & 1)) {
            num_workers_by_level[level] = 1;
            continue;
        }
        size_t ideal_number_of_workers;
        size_t max_reasonable_num_workers = max_num_workers_by_level[level];
        ideal_number_of_workers = execution_times_argmins[level];
        int range = 1;
        if (jitter) ideal_number_of_workers += ((int) (rand() % (2 * range + 1))) - range;
        num_workers_by_level[level] = restrict_to_range(
            1, max_reasonable_num_workers, ideal_number_of_workers
        );
        // printf("level=%ld, jitter=%d, inow=%ld, mrnw=%ld, result=%ld\n",
        // level, jitter, ideal_number_of_workers, max_reasonable_num_workers, num_workers_by_level[level]);
    }
}

static void compute_costs(size_t* num_workers_by_level) {
    for (size_t level = 0; level < num_levels; level++) {
        interval_t score = execution_times_by_num_workers_by_level[level][
            num_workers_by_level[level]
        ];
        if (num_workers_by_level[level] > 1) score += PARALLELISM_COST;
        if (
            !execution_times_mins[level]
            | (score < execution_times_mins[level])
            | (num_workers_by_level[level] == execution_times_argmins[level])
        ) {
            if (num_workers_by_level[level] != execution_times_argmins[level]) printf(
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

static void data_collection_end_tag(
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level
) {
    if (collecting_data && start_times_by_level[0]) {
        compute_costs(num_workers_by_level);
    }
    data_collection_counter++;
    size_t period = 2 + 128 * (data_collection_counter > SLOW_EXPERIMENTS);
    size_t state = data_collection_counter % period;
    if (state == 0) {
        compute_number_of_workers(num_workers_by_level, max_num_workers_by_level, true);
        collecting_data = true;
    } else if (state == 1) {
        compute_number_of_workers(num_workers_by_level, max_num_workers_by_level, false);
        collecting_data = false;
    }
}

#endif
