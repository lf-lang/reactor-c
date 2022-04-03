
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
/**
 * A monotonically increasing sequence of numbers of workers, the first and last elements of which
 * are too large or small to be realizable.
 */
static size_t* possible_nums_workers;

extern size_t num_levels;
extern size_t max_num_workers;

#define START_EXPERIMENTS 8
#define SLOW_EXPERIMENTS 256
#define EXECUTION_TIME_MEMORY 15

static void possible_nums_workers_init() {
    // Start with 0 and end with two numbers strictly greater than max_num_workers.
    size_t pnw_length = 3;
    size_t temp = max_num_workers;
    while ((temp >>= 1)) pnw_length++;
    possible_nums_workers = (size_t*) malloc(pnw_length * sizeof(size_t));
    temp = 1;
    possible_nums_workers[0] = 0;
    for (int i = 1; i < pnw_length; i++) {
        possible_nums_workers[i] = temp;
        temp *= 2;
    }
    assert(temp > max_num_workers);
}

static int get_jitter(size_t current_state, interval_t execution_time) {
    static const size_t parallelism_cost_max = 114688;
    // The following handles the case where the current level really is just fluff:
    // No parallelism needed, no work to be done.
    if (execution_time < 16384 && current_state == 1) return 0;
    int left_score = 16384;  // Want: For execution time = 65536, p(try left) = p(try right)
    int middle_score = 65536;
    int right_score = 65536;
    if (execution_time < parallelism_cost_max) left_score += parallelism_cost_max - execution_time;
    int result = rand() % (left_score + middle_score + right_score);
    if (result < left_score) return -1;
    if (result < left_score + middle_score) return 0;
    return 1;
}

/** Get the result of a state transition. */
static size_t get_nums_workers_neighboring_state(size_t current_state, interval_t execution_time) {
    size_t jitter = get_jitter(current_state, execution_time);
    if (!jitter) return current_state;
    size_t i = 1;
    while (possible_nums_workers[i] < current_state) i++; // TODO: There are more efficient ways to do this.
    return possible_nums_workers[i + jitter];
}

static void data_collection_init(sched_params_t* params) {
    size_t num_levels = params->num_reactions_per_level_size;
    start_times_by_level = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_by_num_workers_by_level = (interval_t**) calloc(
        num_levels, sizeof(interval_t*)
    );
    execution_times_mins = (interval_t*) calloc(num_levels, sizeof(interval_t));
    execution_times_argmins = (size_t*) calloc(num_levels, sizeof(size_t));
    for (size_t i = 0; i < num_levels; i++) {
        execution_times_argmins[i] = max_num_workers;
        execution_times_by_num_workers_by_level[i] = (interval_t*) calloc(
            max_num_workers + 1,  // Add 1 for 1-based indexing
            sizeof(interval_t)
        );
    }
    possible_nums_workers_init();
}

static void data_collection_free() {
    free(start_times_by_level);
    for (size_t i = 0; i < num_levels; i++) {
        free(execution_times_by_num_workers_by_level[i]);
    }
    free(execution_times_by_num_workers_by_level);
    free(possible_nums_workers);
}

static void data_collection_start_level(size_t level) {
    if (collecting_data) start_times_by_level[level] = get_physical_time();
}

static void data_collection_end_level(size_t level, size_t num_workers) {
    if (collecting_data && start_times_by_level[level]) {
        interval_t dt = get_physical_time() - start_times_by_level[level];
        if (!execution_times_by_num_workers_by_level[level][num_workers]) {
            execution_times_by_num_workers_by_level[level][num_workers] = MAX(
                dt,
                2 * execution_times_by_num_workers_by_level[level][execution_times_argmins[level]]
            );
        }
        interval_t* prior_et = &execution_times_by_num_workers_by_level[level][num_workers];
        *prior_et = (*prior_et * EXECUTION_TIME_MEMORY + dt) / (EXECUTION_TIME_MEMORY + 1);
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
        size_t ideal_number_of_workers;
        size_t max_reasonable_num_workers = max_num_workers_by_level[level];
        ideal_number_of_workers = execution_times_argmins[level];
        int range = 1;
        if (jitter) {
            ideal_number_of_workers = get_nums_workers_neighboring_state(ideal_number_of_workers, this_execution_time);
            // printf("%ld -> %ld @ %ld\n", execution_times_argmins[level], ideal_number_of_workers, level);
        }
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
        compute_number_of_workers(
            num_workers_by_level,
            max_num_workers_by_level,
            data_collection_counter > START_EXPERIMENTS
        );
        collecting_data = true;
    } else if (state == 1) {
        compute_number_of_workers(num_workers_by_level, max_num_workers_by_level, false);
        collecting_data = false;
    }
}

#endif
