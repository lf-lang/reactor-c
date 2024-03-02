/**
 * @file
 * @author Edward A. Lee
 *
 * @section LICENSE
Copyright (c) 2020, The University of California at Berkeley

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

 * @section DESCRIPTION
 * Standalone program to convert a Lingua Franca trace file to a comma-separated values
 * text file.
 */
#define LF_TRACE
#include <stdio.h>
#include <string.h>
#include "reactor.h"
#include "trace.h"
#include "trace_util.h"
#include "trace_impl.h"

#define MAX_NUM_REACTIONS 64  // Maximum number of reactions reported in summary stats.
#define MAX_NUM_WORKERS 64

/** File containing the trace binary data. */
FILE* trace_file = NULL;

/** File for writing the output data. */
FILE* output_file = NULL;

/** File for writing summary statistics. */
FILE* summary_file = NULL;

/** Size of the stats table is object_table_size plus twice MAX_NUM_WORKERS. */
int table_size;

/**
 * Print a usage message.
 */
void usage() {
    printf("\nUsage: trace_to_csv [options] trace_file_root (with .lft extension)\n\n");
    printf("\nOptions: \n\n");
    printf("  -s, --start [time_spec] [units]\n");
    printf("   The target time to begin tracing.\n\n");
    printf("  -e, --end [time_spec] [units]\n");
    printf("   The target time to stop tracing.\n\n");
    printf("\n\n");
}

/**
 * Struct for collecting summary statistics for reaction invocations.
 */
typedef struct reaction_stats_t {
    int occurrences;
    instant_t latest_start_time;
    interval_t total_exec_time;
    interval_t max_exec_time;
    interval_t min_exec_time;
} reaction_stats_t;

/**
 * Struct for collecting summary statistics.
 */
typedef struct summary_stats_t {
    trace_event_t event_type; // Use reaction_ends for reactions.
    const char* description;  // Description in the reaction table (e.g. reactor name).
    int occurrences;          // Number of occurrences of this description.
    int num_reactions_seen;
    reaction_stats_t reactions[MAX_NUM_REACTIONS];
} summary_stats_t;

/**
 * Summary stats array. This array has the same size as the
 * object table. Pointer in the array will be void if there
 * are no stats for the object table item.
 */
summary_stats_t** summary_stats;

/** Largest timestamp seen. */
instant_t latest_time = 0LL;

/**
 * Read a trace in the trace_file and write it to the output_file as CSV.
 * @return The number of records read or 0 upon seeing an EOF.
 */
size_t read_and_write_trace(instant_t trace_start_time, instant_t trace_end_time) {
    int trace_length = read_trace();
    if (trace_length == 0) return 0;
    // Write each line.
    for (int i = 0; i < trace_length; i++) {
        // printf("DEBUG: reactor self struct pointer: %p\n", trace[i].pointer);
        int object_instance = -1;
        char* reactor_name = get_object_description(trace[i].pointer, &object_instance);
        if (reactor_name == NULL) {
            reactor_name = "NO REACTOR";
        }
        int trigger_instance = -1;
        char* trigger_name = get_trigger_name(trace[i].trigger, &trigger_instance);
        if (trigger_name == NULL) {
            trigger_name = "NO TRIGGER";
        }
        if ((trace[i].logical_time - start_time) >= trace_start_time
            && (trace[i].logical_time - start_time) < trace_end_time) {
            fprintf(output_file, "%s, %s, %d, %d, " PRINTF_TIME ", %d, " PRINTF_TIME ", %s, " PRINTF_TIME "\n",
                    trace_event_names[trace[i].event_type],
                    reactor_name,
                    trace[i].src_id,
                    trace[i].dst_id,
                    trace[i].logical_time - start_time,
                    trace[i].microstep,
                    trace[i].physical_time - start_time,
                    trigger_name,
                    trace[i].extra_delay
            );
            // Update summary statistics.
            if (trace[i].physical_time > latest_time) {
                latest_time = trace[i].physical_time;
            }
            if (object_instance >= 0 && summary_stats[NUM_EVENT_TYPES + object_instance] == NULL) {
                summary_stats[NUM_EVENT_TYPES + object_instance] = (summary_stats_t*)calloc(1, sizeof(summary_stats_t));
            }
            if (trigger_instance >= 0 && summary_stats[NUM_EVENT_TYPES + trigger_instance] == NULL) {
                summary_stats[NUM_EVENT_TYPES + trigger_instance] = (summary_stats_t*)calloc(1, sizeof(summary_stats_t));
            }

            summary_stats_t* stats = NULL;
            interval_t exec_time;
            reaction_stats_t* rstats;
            int index;

            // Count of event type.
            if (summary_stats[trace[i].event_type] == NULL) {
                summary_stats[trace[i].event_type] = (summary_stats_t*)calloc(1, sizeof(summary_stats_t));
            }
            summary_stats[trace[i].event_type]->event_type = trace[i].event_type;
            summary_stats[trace[i].event_type]->description = trace_event_names[trace[i].event_type];
            summary_stats[trace[i].event_type]->occurrences++;

            switch(trace[i].event_type) {
                case reaction_starts:
                case reaction_ends:
                    // This code relies on the mutual exclusion of reactions in a reactor
                    // and the ordering of reaction_starts and reaction_ends events.
                    if (trace[i].dst_id >= MAX_NUM_REACTIONS) {
                        fprintf(stderr, "WARNING: Too many reactions. Not all will be shown in summary file.\n");
                        continue;
                    }
                    stats = summary_stats[NUM_EVENT_TYPES + object_instance];
                    stats->description = reactor_name;
                    if (trace[i].dst_id >= stats->num_reactions_seen) {
                        stats->num_reactions_seen = trace[i].dst_id + 1;
                    }
                    rstats = &stats->reactions[trace[i].dst_id];
                    if (trace[i].event_type == reaction_starts) {
                        rstats->latest_start_time = trace[i].physical_time;
                    } else {
                        rstats->occurrences++;
                        exec_time = trace[i].physical_time - rstats->latest_start_time;
                        rstats->latest_start_time = 0LL;
                        rstats->total_exec_time += exec_time;
                        if (exec_time > rstats->max_exec_time) {
                            rstats->max_exec_time = exec_time;
                        }
                        if (exec_time < rstats->min_exec_time || rstats->min_exec_time == 0LL) {
                            rstats->min_exec_time = exec_time;
                        }
                    }
                    break;
                case schedule_called:
                    if (trigger_instance < 0) {
                        // No trigger. Do not report.
                        continue;
                    }
                    stats = summary_stats[NUM_EVENT_TYPES + trigger_instance];
                    stats->description = trigger_name;
                    break;
                case user_event:
                    // Although these are not exec times and not reactions,
                    // commandeer the first entry in the reactions array to track values.
                    stats = summary_stats[NUM_EVENT_TYPES + object_instance];
                    stats->description = reactor_name;
                    break;
                case user_value:
                    // Although these are not exec times and not reactions,
                    // commandeer the first entry in the reactions array to track values.
                    stats = summary_stats[NUM_EVENT_TYPES + object_instance];
                    stats->description = reactor_name;
                    rstats = &stats->reactions[0];
                    rstats->occurrences++;
                    // User values are stored in the "extra_delay" field, which is an interval_t.
                    interval_t value = trace[i].extra_delay;
                    rstats->total_exec_time += value;
                    if (value > rstats->max_exec_time) {
                        rstats->max_exec_time = value;
                    }
                    if (value < rstats->min_exec_time || rstats->min_exec_time == 0LL) {
                        rstats->min_exec_time = value;
                    }
                    break;
                case worker_wait_starts:
                case worker_wait_ends:
                case scheduler_advancing_time_starts:
                case scheduler_advancing_time_ends:
                    // Use the reactions array to store data.
                    // There will be two entries per worker, one for waits on the
                    // reaction queue and one for waits while advancing time.
                    index = trace[i].src_id * 2;
                    // Even numbered indices are used for waits on reaction queue.
                    // Odd numbered indices for waits for time advancement.
                    if (trace[i].event_type == scheduler_advancing_time_starts
                            || trace[i].event_type == scheduler_advancing_time_ends) {
                        index++;
                    }
                    if (object_table_size + index >= table_size) {
                        fprintf(stderr, "WARNING: Too many workers. Not all will be shown in summary file.\n");
                        continue;
                    }
                    stats = summary_stats[NUM_EVENT_TYPES + object_table_size + index];
                    if (stats == NULL) {
                        stats = (summary_stats_t*)calloc(1, sizeof(summary_stats_t));
                        summary_stats[NUM_EVENT_TYPES + object_table_size + index] = stats;
                    }
                    // num_reactions_seen here will be used to store the number of
                    // entries in the reactions array, which is twice the number of workers.
                    if (index >= stats->num_reactions_seen) {
                        stats->num_reactions_seen = index;
                    }
                    rstats = &stats->reactions[index];
                    if (trace[i].event_type == worker_wait_starts
                            || trace[i].event_type == scheduler_advancing_time_starts
                    ) {
                        rstats->latest_start_time = trace[i].physical_time;
                    } else {
                        rstats->occurrences++;
                        exec_time = trace[i].physical_time - rstats->latest_start_time;
                        rstats->latest_start_time = 0LL;
                        rstats->total_exec_time += exec_time;
                        if (exec_time > rstats->max_exec_time) {
                            rstats->max_exec_time = exec_time;
                        }
                        if (exec_time < rstats->min_exec_time || rstats->min_exec_time == 0LL) {
                            rstats->min_exec_time = exec_time;
                        }
                    }
                    break;
                default:
                    // No special summary statistics for the rest.
                    break;
            }
            // Common stats across event types.
            if (stats != NULL) {
                stats->occurrences++;
                stats->event_type = trace[i].event_type;
            }
        } else {
            // Out of scope.
        }
    }
    return trace_length;
}

/**
 * Write the summary file.
 */
void write_summary_file() {
    // Overall stats.
    fprintf(summary_file, "Start time:, " PRINTF_TIME "\n", start_time);
    fprintf(summary_file, "End time:, " PRINTF_TIME "\n", latest_time);
    fprintf(summary_file, "Total time:, " PRINTF_TIME "\n", latest_time - start_time);

    fprintf(summary_file, "\nTotal Event Occurrences\n");
    for (int i = 0; i < NUM_EVENT_TYPES; i++) {
        summary_stats_t* stats = summary_stats[i];
        if (stats != NULL) {
            fprintf(summary_file, "%s, %d\n",
                stats->description,
                stats->occurrences
            );
        }
    }

    // First pass looks for reaction invocations.
    // First print a header.
    fprintf(summary_file, "\nReaction Executions\n");
    fprintf(summary_file, "Reactor, Reaction, Occurrences, Total Time, Pct Total Time, Avg Time, Max Time, Min Time\n");
    for (int i = NUM_EVENT_TYPES; i < table_size; i++) {
        summary_stats_t* stats = summary_stats[i];
        if (stats != NULL && stats->num_reactions_seen > 0) {
            for (int j = 0; j < stats->num_reactions_seen; j++) {
                reaction_stats_t* rstats = &stats->reactions[j];
                if (rstats->occurrences > 0) {
                    fprintf(summary_file, "%s, %d, %d, " PRINTF_TIME ", %f, " PRINTF_TIME ", " PRINTF_TIME ", " PRINTF_TIME "\n",
                            stats->description,
                            j, // Reaction number.
                            rstats->occurrences,
                            rstats->total_exec_time,
                            rstats->total_exec_time * 100.0 / (latest_time - start_time),
                            rstats->total_exec_time / rstats->occurrences,
                            rstats->max_exec_time,
                            rstats->min_exec_time
                    );
                }
            }
        }
    }

    // Next pass looks for calls to schedule.
    bool first = true;
    for (int i = NUM_EVENT_TYPES; i < table_size; i++) {
        summary_stats_t* stats = summary_stats[i];
        if (stats != NULL && stats->event_type == schedule_called && stats->occurrences > 0) {
            if (first) {
                first = false;
                fprintf(summary_file, "\nSchedule calls\n");
                fprintf(summary_file, "Trigger, Occurrences\n");
            }
            fprintf(summary_file, "%s, %d\n", stats->description, stats->occurrences);
        }
    }

    // Next pass looks for user-defined events.
    first = true;
    for (int i = NUM_EVENT_TYPES; i < table_size; i++) {
        summary_stats_t* stats = summary_stats[i];
        if (stats != NULL
                && (stats->event_type == user_event || stats->event_type == user_value)
                && stats->occurrences > 0) {
            if (first) {
                first = false;
                fprintf(summary_file, "\nUser events\n");
                fprintf(summary_file, "Description, Occurrences, Total Value, Avg Value, Max Value, Min Value\n");
            }
            fprintf(summary_file, "%s, %d", stats->description, stats->occurrences);
            if (stats->event_type == user_value && stats->reactions[0].occurrences > 0) {
                // This assumes that the first "reactions" entry has been comandeered for this data.
                fprintf(summary_file, ", " PRINTF_TIME ", " PRINTF_TIME ", " PRINTF_TIME ", " PRINTF_TIME "\n",
                        stats->reactions[0].total_exec_time,
                        stats->reactions[0].total_exec_time / stats->reactions[0].occurrences,
                        stats->reactions[0].max_exec_time,
                        stats->reactions[0].min_exec_time
                );
            } else {
                fprintf(summary_file, "\n");
            }
        }
    }

    // Next pass looks for wait events.
    first = true;
    for (int i = NUM_EVENT_TYPES; i < table_size; i++) {
        summary_stats_t* stats = summary_stats[i];
        if (stats != NULL && (
                stats->event_type == worker_wait_ends
                || stats->event_type == scheduler_advancing_time_ends)
        ) {
            if (first) {
                first = false;
                fprintf(summary_file, "\nWorkers Waiting\n");
                fprintf(summary_file, "Worker, Waiting On, Occurrences, Total Time, Pct Total Time, Avg Time, Max Time, Min Time\n");
            }
            char* waitee = "reaction queue";
            if (stats->event_type == scheduler_advancing_time_ends
                    || stats->event_type == scheduler_advancing_time_starts) {
                waitee = "advancing time";
            }
            for (int j = 0; j <= stats->num_reactions_seen; j++) {
                reaction_stats_t* rstats = &stats->reactions[j];
                if (rstats->occurrences > 0) {
                    fprintf(summary_file, "%d, %s, %d, " PRINTF_TIME ", %f, " PRINTF_TIME ", " PRINTF_TIME ", " PRINTF_TIME "\n",
                            j / 2,
                            waitee,
                            rstats->occurrences,
                            rstats->total_exec_time,
                            rstats->total_exec_time * 100.0 / (latest_time - start_time),
                            rstats->total_exec_time / rstats->occurrences,
                            rstats->max_exec_time,
                            rstats->min_exec_time
                    );
                }
            }
        }
    }
}

instant_t string_to_instant(const char* time_spec, const char* units) {
    instant_t duration;
    #if defined(PLATFORM_ARDUINO)
    duration = atol(time_spec);
    #else
    duration = atoll(time_spec);
    #endif
    // A parse error returns 0LL, so check to see whether that is what is meant.
    if (duration == 0LL && strncmp(time_spec, "0", 1) != 0) {
        // Parse error.
        printf("Invalid time value: %s", time_spec);
        return -1;
    }
    if (strncmp(units, "sec", 3) == 0) {
        duration = SEC(duration);
    } else if (strncmp(units, "msec", 4) == 0) {
        duration = MSEC(duration);
    } else if (strncmp(units, "usec", 4) == 0) {
        duration = USEC(duration);
    } else if (strncmp(units, "nsec", 4) == 0) {
        duration = NSEC(duration);
    } else if (strncmp(units, "min", 3) == 0) {
        duration = MINUTE(duration);
    } else if (strncmp(units, "hour", 4) == 0) {
        duration = HOUR(duration);
    } else if (strncmp(units, "day", 3) == 0) {
        duration = DAY(duration);
    } else if (strncmp(units, "week", 4) == 0) {
        duration = WEEK(duration);
    } else {
        // Invalid units.
        printf("Invalid time units: %s", units);
        return -1;
    }
    return duration;
}

int process_args(int argc, const char* argv[], char** root, instant_t* start_time, instant_t* end_time) {
    int i = 1;
    while (i < argc){
        const char* arg = argv[i++];
        if (strcmp(strrchr(arg, '\0') - 4, ".lft") == 0) {
            // Open the trace file.
            trace_file = open_file(arg, "r");
            if (trace_file == NULL) exit(1);
            *root = root_name(arg);
        } else if (strcmp(arg, "-s") == 0) {
            // sscanf(argv[++i], "%ld", start_time);
            if (argc < i + 2) {
                printf("-s needs time value and unit.");
                usage();
                return -1;
            }
            const char* time_spec = argv[i++];
            const char* units = argv[i++];
            *start_time = string_to_instant(time_spec, units);
            if (*start_time == -1) {
                usage();
                return -1;
            }
        } else if (strcmp(arg, "-e") == 0) {
            if (argc < i + 2) {
                printf("-e needs time value and unit.");
                usage();
                return -1;
            }
            const char* time_spec = argv[i++];
            const char* units = argv[i++];
            *end_time = string_to_instant(time_spec, units);
            if (*end_time == -1) {
                usage();
                return -1;
            }
        } else {
            usage();
            exit(0);
        }
    }
    return 0;
}

int main(int argc, const char* argv[]) {
    instant_t trace_start_time = NEVER;
    instant_t trace_end_time = FOREVER;
    char* root;

    if (process_args(argc, argv, &root, &trace_start_time, &trace_end_time) != 0) {
        return -1;
    }

    // Construct the name of the csv output file and open it.
    char csv_filename[strlen(root) + 5];
    strcpy(csv_filename, root);
    strcat(csv_filename, ".csv");
    output_file = open_file(csv_filename, "w");
    if (output_file == NULL) exit(1);

    // Construct the name of the summary output file and open it.
    char summary_filename[strlen(root) + 13];
    strcpy(summary_filename, root);
    strcat(summary_filename, "_summary.csv");
    summary_file = open_file(summary_filename, "w");
    if (summary_file == NULL) exit(1);

    free(root);

    if (read_header() >= 0) {
        // Allocate an array for summary statistics.
        table_size = NUM_EVENT_TYPES + object_table_size + (MAX_NUM_WORKERS * 2);
        summary_stats = (summary_stats_t**)calloc(table_size, sizeof(summary_stats_t*));

        // Write a header line into the CSV file.
        fprintf(output_file, "Event, Reactor, Source, Destination, Elapsed Logical Time, Microstep, Elapsed Physical Time, Trigger, Extra Delay\n");
        while (read_and_write_trace(trace_start_time, trace_end_time) != 0) {};

        write_summary_file();

        // File closing is handled by termination function.
    }
}
