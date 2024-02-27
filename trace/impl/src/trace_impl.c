#include <stdio.h>  // debugging only
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "trace.h"
#include "platform.h"
#include "logging_macros.h"
#include "trace_impl.h"

/** Macro to use when access to trace file fails. */
#define _LF_TRACE_FAILURE(trace) \
    do { \
        fprintf(stderr, "WARNING: Access to trace file failed.\n"); \
        fclose(trace->_lf_trace_file); \
        trace->_lf_trace_file = NULL; \
        return -1; \
    } while(0)

// PRIVATE DATA STRUCTURES ***************************************************

static lf_platform_mutex_ptr_t trace_mutex;
static trace_t trace;
static int process_id;
static int64_t start_time;

// PRIVATE HELPERS ***********************************************************

/**
 * Write the trace header information.
 * See trace.h.
 * @return The number of items written to the object table or -1 for failure.
 */
static int write_trace_header(trace_t* trace) {
    if (trace->_lf_trace_file != NULL) {
        size_t items_written = fwrite(
                &start_time,
                sizeof(int64_t),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) _LF_TRACE_FAILURE(trace);

        // The next item in the header is the size of the
        // _lf_trace_object_descriptions table.
        items_written = fwrite(
                &trace->_lf_trace_object_descriptions_size,
                sizeof(int),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) _LF_TRACE_FAILURE(trace);

        // Next we write the table.
        for (int i = 0; i < trace->_lf_trace_object_descriptions_size; i++) {
            // Write the pointer to the self struct.
            items_written = fwrite(
                        &trace->_lf_trace_object_descriptions[i].pointer,
                        sizeof(void*),
                        1,
                        trace->_lf_trace_file
            );
            if (items_written != 1) _LF_TRACE_FAILURE(trace);

            // Write the pointer to the trigger_t struct.
            items_written = fwrite(
                        &trace->_lf_trace_object_descriptions[i].trigger,
                        sizeof(void*),
                        1,
                        trace->_lf_trace_file
            );
            if (items_written != 1) _LF_TRACE_FAILURE(trace);

            // Write the object type.
            items_written = fwrite(
                        &trace->_lf_trace_object_descriptions[i].type, // Write the pointer value.
                        sizeof(_lf_trace_object_t),
                        1,
                        trace->_lf_trace_file
            );
            if (items_written != 1) _LF_TRACE_FAILURE(trace);

            // Write the description.
            int description_size = strlen(trace->_lf_trace_object_descriptions[i].description);
            items_written = fwrite(
                        trace->_lf_trace_object_descriptions[i].description,
                        sizeof(char),
                        description_size + 1, // Include null terminator.
                        trace->_lf_trace_file
            );
            if (items_written != description_size + 1) _LF_TRACE_FAILURE(trace);
        }
    }
    return trace->_lf_trace_object_descriptions_size;
}

/**
 * @brief Flush the specified buffer to a file.
 * This assumes the caller has entered a critical section.
 * @param worker Index specifying the trace to flush.
 */
static void flush_trace_locked(trace_t* trace, int worker) {
    if (trace->_lf_trace_stop == 0
        && trace->_lf_trace_file != NULL
        && trace->_lf_trace_buffer_size[worker] > 0
    ) {
        // If the trace header has not been written, write it now.
        // This is deferred to here so that user trace objects can be
        // registered in startup reactions.
        if (!trace->_lf_trace_header_written) {
            if (write_trace_header(trace) < 0) {
                lf_print_error("Failed to write trace header. Trace file will be incomplete.");
                return;
            }
            trace->_lf_trace_header_written = true;
        }

        // Write first the length of the array.
        size_t items_written = fwrite(
                &trace->_lf_trace_buffer_size[worker],
                sizeof(int),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) {
            fprintf(stderr, "WARNING: Access to trace file failed.\n");
            fclose(trace->_lf_trace_file);
            trace->_lf_trace_file = NULL;
        } else {
            // Write the contents.
            items_written = fwrite(
                    trace->_lf_trace_buffer[worker],
                    sizeof(trace_record_nodeps_t),
                    trace->_lf_trace_buffer_size[worker],
                    trace->_lf_trace_file
            );
            if (items_written != trace->_lf_trace_buffer_size[worker]) {
                fprintf(stderr, "WARNING: Access to trace file failed.\n");
                fclose(trace->_lf_trace_file);
                trace->_lf_trace_file = NULL;
            }
        }
        trace->_lf_trace_buffer_size[worker] = 0;
    }
}

/**
 * @brief Flush the specified buffer to a file.
 * @param worker Index specifying the trace to flush.
 */
static void flush_trace(trace_t* trace, int worker) {
    // To avoid having more than one worker writing to the file at the same time,
    // enter a critical section.
    lf_platform_mutex_lock(trace_mutex);
    flush_trace_locked(trace, worker);
    lf_platform_mutex_unlock(trace_mutex);
}

static void start_trace(trace_t* trace, int max_num_local_threads) {
    // Do not write the trace header information to the file yet
    // so that startup reactions can register user-defined trace objects.
    // write_trace_header();
    trace->_lf_trace_header_written = false;

    // Allocate an array of arrays of trace records, one per worker thread plus one
    // for the 0 thread (the main thread, or in an single-threaded program, the only
    // thread).
    trace->_lf_number_of_trace_buffers = max_num_local_threads;
    trace->_lf_trace_buffer = (trace_record_nodeps_t**)malloc(sizeof(trace_record_nodeps_t*) * (trace->_lf_number_of_trace_buffers + 1));
    trace->_lf_trace_buffer++; // the buffer at index -1 is a fallback for user threads.
    for (int i = -1; i < trace->_lf_number_of_trace_buffers; i++) {
        trace->_lf_trace_buffer[i] = (trace_record_nodeps_t*)malloc(sizeof(trace_record_nodeps_t) * TRACE_BUFFER_CAPACITY);
    }
    // Array of counters that track the size of each trace record (per thread).
    trace->_lf_trace_buffer_size = (int*)calloc(sizeof(int), trace->_lf_number_of_trace_buffers + 1);
    trace->_lf_trace_buffer_size++;

    trace->_lf_trace_stop = 0;
    LF_PRINT_DEBUG("Started tracing.");
}

static void trace_new(char * filename) {

    // Determine length of the filename
    size_t len = strlen(filename) + 1;

    // Allocate memory for the filename on the trace struct
    trace.filename = (char*) malloc(len * sizeof(char));
    LF_ASSERT(trace.filename, "Out of memory");

    // Copy it to the struct
    strncpy(trace.filename, filename, len);
    // FIXME: location of trace file should be customizable.
    trace._lf_trace_file = fopen(trace.filename, "w");
    if (trace._lf_trace_file == NULL) {
        fprintf(stderr, "WARNING: Failed to open log file with error code %d."
                "No log will be written.\n", errno);
    } else {
        LF_PRINT_DEBUG("Opened trace file %s.", trace.filename);
    }
}

static void trace_free(trace_t *trace) {
    free(trace->filename);
}

static void stop_trace_locked(trace_t* trace) {
    if (trace->_lf_trace_stop) {
        // Trace was already stopped. Nothing to do.
        return;
    }
    for (int i = -1; i < trace->_lf_number_of_trace_buffers; i++) {
        // Flush the buffer if it has data.
        LF_PRINT_DEBUG("Trace buffer %d has %d records.", i, trace->_lf_trace_buffer_size[i]);
        if (trace->_lf_trace_buffer_size && trace->_lf_trace_buffer_size[i] > 0) {
            flush_trace_locked(trace, i);
        }
    }
    trace->_lf_trace_stop = 1;
    if (trace->_lf_trace_file != NULL) {
        fclose(trace->_lf_trace_file);
        trace->_lf_trace_file = NULL;
    }
    LF_PRINT_DEBUG("Stopped tracing.");
}

static void stop_trace(trace_t* trace) {
    lf_platform_mutex_lock(trace_mutex);
    stop_trace_locked(trace);
    lf_platform_mutex_unlock(trace_mutex);
}

// IMPLEMENTATION OF VERSION API *********************************************

version_t lf_version_tracing() {
    return (version_t) {
        .build_config = (build_config_t) {
            .single_threaded = TRIBOOL_DOES_NOT_MATTER,
#ifdef NDEBUG
            .build_type_is_debug = TRIBOOL_FALSE,
#else
            .build_type_is_debug = TRIBOOL_TRUE,
#endif
            .log_level = LOG_LEVEL,
        },
        .core_version_name = NULL,
    };
}

// IMPLEMENTATION OF TRACE API ***********************************************

void lf_tracing_register_trace_event(object_description_t description) {
    lf_platform_mutex_lock(trace_mutex);
    if (trace._lf_trace_object_descriptions_size >= TRACE_OBJECT_TABLE_SIZE) {
        lf_platform_mutex_unlock(trace_mutex);
        fprintf(stderr, "WARNING: Exceeded trace object table size. Trace file will be incomplete.\n");
        return;
    }
    trace._lf_trace_object_descriptions[trace._lf_trace_object_descriptions_size++] = description;
    lf_platform_mutex_unlock(trace_mutex);
}

void lf_tracing_tracepoint(int worker, trace_record_nodeps_t* tr) {
    // Worker argument determines which buffer to write to.
    int tid = lf_thread_id();
    if (tid < 0) {
        // The current thread was created by the user. It is not managed by LF, its ID is not known,
        // and most importantly it does not count toward the limit on the total number of threads.
        // Therefore we should fall back to using a mutex.
        lf_platform_mutex_lock(trace_mutex);
    }
    if (tid > trace._lf_number_of_trace_buffers) {
        lf_print_error_and_exit("the thread id (%d) exceeds the number of trace buffers (%d)", tid, trace._lf_number_of_trace_buffers);
    }

    // Flush the buffer if it is full.
    if (trace._lf_trace_buffer_size[tid] >= TRACE_BUFFER_CAPACITY) {
        // No more room in the buffer. Write the buffer to the file.
        flush_trace(&trace, tid);
    }
    // The above flush_trace resets the write pointer.
    int i = trace._lf_trace_buffer_size[tid];
    // Write to memory buffer.
    // Get the correct time of the event

    trace._lf_trace_buffer[tid][i] = *tr;
    trace._lf_trace_buffer_size[tid]++;
    if (tid < 0) {
        lf_platform_mutex_unlock(trace_mutex);
    }
}

void lf_tracing_global_init(char* file_name_prefix, int fedid, int max_num_local_threads) {
    trace_mutex = lf_platform_mutex_new();
    if (!trace_mutex) {
        fprintf(stderr, "WARNING: Failed to initialize trace mutex.\n");
        exit(1);
    }
    process_id = fedid;
    char filename[100];
    if (strcmp(file_name_prefix, "rti") == 0) {
        sprintf(filename, "%s.lft", file_name_prefix);
    } else {
        sprintf(filename, "%s%d.lft", file_name_prefix, process_id);
    }
    trace_new(filename);
    start_trace(&trace, max_num_local_threads);
}
void lf_tracing_set_start_time(int64_t time) {
    start_time = time;
}
void lf_tracing_global_shutdown() {
    stop_trace(&trace);
    trace_free(&trace);
    lf_platform_mutex_free(trace_mutex);
}
