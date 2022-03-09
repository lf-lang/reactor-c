
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
static bool completing_experiment = false;
static int experimental_jitter = 0;

extern size_t num_levels;
extern size_t max_num_workers;

#define OPTIMAL_NANOSECONDS_WORK 32768
#define STOP_USING_OPTIMAL_NANOSECONDS_WORK 5
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
        interval_t score = execution_times_by_level[level] + execution_times_by_level[
            (level + num_levels - 1) % num_levels
        ];
        if (!execution_times_mins[level] | (score < execution_times_mins[level]) | (num_workers == execution_times_argmins[level])) {
            // printf(
            //     "Argmin update: %ld(%ld) -> %ld(%ld) @ %ld\n",
            //     execution_times_argmins[level], execution_times_mins[level],
            //     num_workers, score,
            //     level
            // );
            execution_times_mins[level] = score;
            execution_times_argmins[level] = num_workers;
        }
    }
    if (level == 0) {
        data_collection_counter++;
        int shift = (data_collection_counter > SLOW_EXPERIMENTS) << 3;
        size_t shifted = data_collection_counter >> shift;
        completing_experiment = !completing_experiment & collecting_data;
        collecting_data = completing_experiment | (data_collection_counter == (shifted << shift));
        experimental_jitter = ((int) (shifted % 3)) - 1;
    }
}

static size_t restrict_to_range(size_t start_inclusive, size_t end_inclusive, size_t value) {
    if (value < start_inclusive) return start_inclusive;
    if (value > end_inclusive) return end_inclusive;
    return value;
}

static void data_collection_compute_number_of_workers(
    size_t* num_workers_by_level,
    size_t* max_num_workers_by_level
) {
    if (!collecting_data) return;
    for (size_t level = 0; level < num_levels; level++) {
        size_t ideal_number_of_workers;
        size_t max_reasonable_num_workers = max_num_workers_by_level[level];
        if (data_collection_counter < STOP_USING_OPTIMAL_NANOSECONDS_WORK) {
            ideal_number_of_workers = execution_times_by_level[level] / OPTIMAL_NANOSECONDS_WORK;
        } else {
            ideal_number_of_workers = execution_times_argmins[level];
            if (!completing_experiment) ideal_number_of_workers += experimental_jitter;
            // printf("Assigning %ld @ %ld.\n", ideal_number_of_workers, level);
        }
        // printf("level=%ld, num_workers=%ld, jitter=%d\n", level, ideal_number_of_workers, experimental_jitter);
        num_workers_by_level[level] = restrict_to_range(
            1, max_reasonable_num_workers, ideal_number_of_workers
        );
    }
}

#endif
