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
#include <getopt.h>
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "reactor.h"
#include "trace.h"
#include "trace_util.h"

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

/** Structure for passed command line options */
typedef struct {
    trace_event_t filter; // Filter
    char *suffix;
    char *r_name;
} program_options_t;

program_options_t opts = {
        .filter = -1,
        .suffix = ""
};

/** All Events of specified Types */
enum filter_options_t {
    reaction_events = NUM_EVENT_TYPES + 1,
    user_events,
    worker_wait_events,
    scheduler_advancing_time_events,
    send_events,
    receive_events
};

/** Trace Processor Fptr */
typedef void (*processor_f) (const char *fname);

/** Available Filter Options */
struct filter {
    int value;
    const char *str;
} filters[] = {
        [user_event] = {.str = "user_event", .value = user_event },
        [user_value] = {.str = "user_value", .value = user_value },
        [user_stats] = {.str = "user_stats", .value = user_stats },
        [schedule_called] = {.str = "schedule_called", .value = schedule_called },
        [federated] = {.str = "federated", .value = federated },
        [NUM_EVENT_TYPES] = {.str = "none", .value = -1 },
        [reaction_events] = {.str = "reaction_events", .value = reaction_events },
        [user_events] = {.str = "user_events", .value = user_events},
        [worker_wait_events] = {.str = "worker_wait_events", .value = worker_wait_events},
        [scheduler_advancing_time_events] = {.str = "scheduler_advancing_time_events", .value = scheduler_advancing_time_events},
        [send_events] = {.str = "send_events", .value = send_events},
        [receive_events] = {.str = "receive_events", .value = receive_events}
};

/**
 * Print a usage message.
 */
void usage() {
    printf("\nBasic Usage: trace_to_csv [options] trace_file_root (without .lft extension)\n\n");
    printf("Advanced Usage (Directory Mode)\nUsage: trace_to_csv [options] [args] path_to_root_directory\n\n");
    printf("Options:\n-r:\t Recursively find *.lft files on provided path\n");
    printf("-d:\t Root Directory Path\n");
    printf("-f:\t Filter events\n");
    printf("-o:\t Outfile Prefix\n");
    printf("\nFilters:\n[");
    for (int i = 0; i < sizeof (filters) / sizeof (struct filter); ++i) {
        if (filters[i].str != NULL && filters[i].value != -1) {
            printf(" %s |", filters[i].str);
        }
    }
    printf(" none ]\n");

    /* No options yet:
    printf("\nOptions: \n\n");
    printf("  -f, --fast [true | false]\n");
    printf("   Whether to wait for physical time to match logical time.\n\n");
    printf("\n\n");
    */
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
 * Sumary stats array. This array has the same size as the
 * object table. Pointer in the array will be void if there
 * are no stats for the object table item.
 */
summary_stats_t** summary_stats = NULL;

/** Largest timestamp seen. */
instant_t latest_time = 0LL;

/**
 * Checks if specified event ev is Filtered for this run
 * @param ev Current Tracing Event
 * @returns True if this event needs to be filtered or false otherwise
 * */
bool is_filtered_event(trace_event_t ev) {
    if (ev == opts.filter) {
        // Definitive Event
        return true;
    } else {
        if (opts.filter == reaction_events) {
            switch (ev) {
                case reaction_starts:
                case reaction_ends:
                case reaction_deadline_missed:
                    return true;
                default:
                    return false;
            }
        } else if (opts.filter == user_events) {
            switch (ev) {
                case user_value:
                case user_event:
                case user_stats:
                    return true;
                default:
                    return false;
            }
        } else if (opts.filter == worker_wait_events) {
            switch (ev) {
                case worker_wait_starts:
                case worker_wait_ends:
                    return true;
                default:
                    return false;
            }
        } else if (opts.filter == scheduler_advancing_time_events) {
            switch (ev) {
                case scheduler_advancing_time_starts:
                case scheduler_advancing_time_ends:
                    return true;
                default:
                    return false;
            }
        } else if (opts.filter == send_events) {
            switch (ev) {
                case send_ACK ... send_ADR_QR:
                    return true;
                default:
                    return false;
            }
        } else if (opts.filter == receive_events) {
            switch (ev) {
                case receive_ACK ... receive_UNIDENTIFIED:
                    return true;
                default:
                    return false;
            }
        }
    }
    return false;
}

/**
 * Read a trace in the specified file and write it to the specified CSV file.
 * @return The number of records read or 0 upon seeing an EOF.
 */
size_t read_and_write_trace() {
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
        if ((opts.filter != -1) && is_filtered_event(trace[i].event_type)) {
            fprintf(output_file, "%s, %s, %s, %s, %d, %d, %lld, %d, %lld, %s, %lld, %Lf\n",
                    opts.r_name,
                    opts.suffix,
                    trace_event_names[trace[i].event_type],
                    reactor_name,
                    trace[i].src_id,
                    trace[i].dst_id,
                    trace[i].logical_time - start_time,
                    trace[i].microstep,
                    trace[i].physical_time - start_time,
                    trigger_name,
                    trace[i].extra_delay,
                    trace[i].extra_value
            );
        } else {
            fprintf(output_file, "%s, %s, %d, %d, %lld, %d, %lld, %s, %lld, %Lf\n",
                    trace_event_names[trace[i].event_type],
                    reactor_name,
                    trace[i].src_id,
                    trace[i].dst_id,
                    trace[i].logical_time - start_time,
                    trace[i].microstep,
                    trace[i].physical_time - start_time,
                    trigger_name,
                    trace[i].extra_delay,
                    trace[i].extra_value
            );
        }

        if (summary_file) {
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
                case user_stats:
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
        }
    }
    return trace_length;
}

/**
 * Write the summary file.
 */
void write_summary_file() {
    // Overall stats.
    fprintf(summary_file, "Start time:, %lld\n", start_time);
    fprintf(summary_file, "End time:, %lld\n", latest_time);
    fprintf(summary_file, "Total time:, %lld\n", latest_time - start_time);

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
                    fprintf(summary_file, "%s, %d, %d, %lld, %f, %lld, %lld, %lld\n",
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
                fprintf(summary_file, ", %lld, %lld, %lld, %lld\n",
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
                    fprintf(summary_file, "%d, %s, %d, %lld, %f, %lld, %lld, %lld\n",
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

/** Filter Options Table */
trace_event_t get_filtered_option(const char *opt) {
    for (int i = 0; i < sizeof (filters) / sizeof (struct filter); ++i) {
        if (filters[i].str != NULL && strcmp(opt, filters[i].str) == 0) {
            return filters[i].value;
        }
    }
    return -1;
}

/** Parse and set Program Options */
void scan_program_options(int argc, char **argv) {
    if (argc > 2) {
        // scan for program options
        int arg;
        printf( "------- options specified:\n");
        while((arg = getopt(argc, argv, "i:f:s:")) != -1) {
            switch (arg) {
                case 'f':
                    opts.filter = get_filtered_option(optarg);
                    printf ("\tFilter=%s", filters[opts.filter].str);
                    break;
                case 'i':
                    opts.r_name = strdup (optarg);
                    printf ("\tFileName=%s", opts.r_name);
                    break;
                case 's':
                    opts.suffix = strdup(optarg);
                    printf ("\tSuffix=%s", opts.suffix);
                    break;
                default:
                    break;
            }
        }
        printf ("\n---------\n");
    } else {
        opts.r_name = argv[1];
    }
}

void open_output_files(const char *fname) {
    // Construct the name of the csv output file and open it.
    char* root = NULL;
    if (strcmp (opts.suffix, "") == 0) {
        root = root_name(fname);
    } else {
        char* last_period = strrchr(fname, '.');
        size_t length = (last_period == NULL) ?
            strlen(fname) : last_period - fname;
        root = (char*)calloc(length + 1, sizeof (char));
        if (root == NULL) exit (1);
        strncpy(root, fname, length);
    }
    char *csv_filename;
    int csv_len = snprintf (NULL, 0, "%s%s%s", root, opts.suffix, ".csv");
    csv_filename = calloc (csv_len + 1, sizeof (char));
    assert (csv_filename);
    sprintf (csv_filename, "%s%s%s", root, opts.suffix, ".csv");
    output_file = open_file(csv_filename, "w");
    if (output_file == NULL) exit(1);
    free (csv_filename);

    if (opts.filter == -1) {
        // Construct the name of the summary output file and open it.
        char *summary_filename;
        int summary_len = snprintf (NULL, 0, "%s%s%s", root, opts.suffix, "_summary.csv");
        summary_filename = calloc (summary_len + 1, sizeof (char));
        assert (summary_filename);
        sprintf (summary_filename, "%s%s%s", root, opts.suffix, "_summary.csv");
        summary_file = open_file(summary_filename, "w");
        if (summary_file == NULL) exit(1);
        free (summary_filename);
    }

    free(root);
}

void close_output_files() {
    fclose(output_file);
    if (summary_file) {
        fclose(summary_file);
    }
}

void trace_processor(const char *fname) {
    // Open the trace file.
    trace_file = open_file(fname, "r");
    if (trace_file == NULL) exit(1);

    if (read_header() > 0) {
        // Allocate an array for summary statistics.
        table_size = NUM_EVENT_TYPES + object_table_size + (MAX_NUM_WORKERS * 2);
        summary_stats = (summary_file) ? (summary_stats_t**)calloc(table_size, sizeof(summary_stats_t*)) : NULL;

        while (read_and_write_trace() != 0) {};

        if (summary_file) {
            write_summary_file();
            for (int i = 0; i < table_size; ++i) {
                if (summary_stats[i] != NULL) {
                    free (summary_stats[i]);
                }
            }
            free (summary_stats);
            summary_stats = NULL;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        exit(0);
    }
    scan_program_options(argc, argv);
    open_output_files(opts.r_name);

    // Write a header line into the CSV file.
    if (opts.filter != -1) {
        fprintf(output_file,
                "Trace File, Trace Index, Event, Reactor, Source, Destination, Elapsed Logical Time, Microstep, Elapsed Physical Time, Trigger, Extra Delay, Extra Value\n");
    } else {
        fprintf(output_file, "Event, Reactor, Source, Destination, Elapsed Logical Time, Microstep, Elapsed Physical Time, Trigger, Extra Delay, Extra Value\n");
    }
    trace_processor(opts.r_name);
    close_output_files();
}
