/*************
Copyright (c) 2022, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/**
 * Scheduling-related data collection and analysis that is performed at run-time.
 * @author{Peter Donovan <peterdonovan@berkeley.edu>}
 */

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
 * are too large or small to represent valid states of the system (i.e., state transitions to them
 * are instantaneously reflected).
 */
static size_t* possible_nums_workers;

extern size_t num_levels;
extern size_t max_num_workers;
instant_t lf_time_physical(void);

#define START_EXPERIMENTS 8
#define SLOW_EXPERIMENTS 256
#define EXECUTION_TIME_MEMORY 15

/** @brief Initialize the possible_nums_workers array. */
static void possible_nums_workers_init() {
    // Start with 0 and end with two numbers strictly greater than max_num_workers. This must start
    // at 4 because the first two and last two entries are not counted.
    size_t pnw_length = 4;
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

/**
 * @brief Return a random integer in the interval [-1, +1] representing whether the number of
 * workers used on a certain level should increase, decrease, or remain the same, with a probability
 * distribution possibly dependent on the parameters.
 * @param current_state The index currently used by this level in the possible_nums_workers array.
 * @param execution_time An estimate of the execution time of the level in the case for which we
 * would like to optimize.
 */
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

/** @brief Get the number of workers resulting from a random state transition. */
static size_t get_nums_workers_neighboring_state(size_t current_state, interval_t execution_time) {
    size_t jitter = get_jitter(current_state, execution_time);
    if (!jitter) return current_state;
    size_t i = 1;
    // TODO: There are more efficient ways to do this.
    while (possible_nums_workers[i] < current_state) i++;
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

/** @brief Record that the execution of the given level is beginning. */
static void data_collection_start_level(size_t level) {
    if (collecting_data) start_times_by_level[level] = lf_time_physical();
}

/** @brief Record that the execution of the given level has completed. */
static void data_collection_end_level(size_t level, size_t num_workers) {
    if (collecting_data && start_times_by_level[level]) {
        interval_t dt = lf_time_physical() - start_times_by_level[level];
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

/**
 * @brief Update num_workers_by_level in-place.
 * @param num_workers_by_level The number of workers that should be used to execute each level.
 * @param max_num_workers_by_level The maximum possible number of workers that could reasonably be
 * assigned to each level.
 * @param jitter Whether the possibility of state transitions to numbers of workers that are not
 * (yet) empirically optimal is desired.
 */
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
            ideal_number_of_workers = get_nums_workers_neighboring_state(
                ideal_number_of_workers, this_execution_time
            );
        }
        int minimum_workers = 1;
#ifdef WORKERS_NEEDED_FOR_FEDERATE
        // TODO: only apply this constraint on levels containing control reactions
        minimum_workers = WORKERS_NEEDED_FOR_FEDERATE > max_reasonable_num_workers ?
            max_reasonable_num_workers : WORKERS_NEEDED_FOR_FEDERATE;
#endif
        num_workers_by_level[level] = restrict_to_range(
            minimum_workers, max_reasonable_num_workers, ideal_number_of_workers
        );
    }
}

/**
 * @brief Update minimum and argmin (wrt number of workers used) execution times according the most
 * recent execution times recorded.
 * @param num_workers_by_level The number of workers most recently used to execute each level.
 */
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
            execution_times_mins[level] = score;
            execution_times_argmins[level] = num_workers_by_level[level];
        }
    }
}

/**
 * @brief Record that the execution of a tag has completed.
 * @param num_workers_by_level The number of workers used to execute each level of the tag.
 * @param max_num_workers_by_level The maximum number of workers that could reasonably be used to
 * execute each level, for any tag.
 */
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
