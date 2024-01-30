#include <stdio.h>  // debugging only
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "trace.h"

// FIXME: Decide on a safe way to expose such APIs as these to plugins implemented in C
#include "platform.h"
// int lf_mutex_unlock(lf_mutex_t* mutex);
// int lf_mutex_init(lf_mutex_t* mutex);
// int lf_mutex_lock(lf_mutex_t* mutex);

// FIXME: Target property should specify the capacity of the trace buffer.
#define TRACE_BUFFER_CAPACITY 2048

/** Size of the table of trace objects. */
#define TRACE_OBJECT_TABLE_SIZE 1024

/** Macro to use when access to trace file fails. */
#define _LF_TRACE_FAILURE(trace) \
    do { \
        fprintf(stderr, "WARNING: Access to trace file failed.\n"); \
        fclose(trace->_lf_trace_file); \
        trace->_lf_trace_file = NULL; \
        return -1; \
    } while(0)

// TYPE DEFINITIONS **********************************************************

/**
 * @brief This struct holds all the state associated with tracing in a single environment.
 * Each environment which has tracing enabled will have such a struct on its environment struct.
 *
 */
typedef struct trace_t {
    /**
     * Array of buffers into which traces are written.
     * When a buffer becomes full, the contents is flushed to the file,
     * which will create a significant pause in the calling thread.
     */
    trace_record_nodeps_t** _lf_trace_buffer;
    int* _lf_trace_buffer_size;

    /** The number of trace buffers allocated when tracing starts. */
    int _lf_number_of_trace_buffers;

    /** Marker that tracing is stopping or has stopped. */
    int _lf_trace_stop;

    /** The file into which traces are written. */
    FILE* _lf_trace_file;

    /** The file name where the traces are written*/
    char *filename;

    /** Table of pointers to a description of the object. */
    object_description_t _lf_trace_object_descriptions[TRACE_OBJECT_TABLE_SIZE];
    int _lf_trace_object_descriptions_size;

    /** Indicator that the trace header information has been written to the file. */
    bool _lf_trace_header_written;

    // /** Pointer back to the environment which we are tracing within*/
    // environment_t* env;
} trace_t;

// PRIVATE DATA STRUCTURES ***************************************************

static lf_platform_mutex_ptr_t trace_mutex;
static trace_t trace;
static int64_t start_time;

// PRIVATE HELPERS ***********************************************************

/**
 * Write the trace header information.
 * See trace.h.
 * @return The number of items written to the object table or -1 for failure.
 */
static int write_trace_header(trace_t* trace) {
    if (trace->_lf_trace_file != NULL) {
        // The first item in the header is the start time.
        // This is both the starting physical time and the starting logical time.
        // int64_t start_time = lf_time_start();
        // int64_t start_time = 0; // FIXME: We want the actual start time, but the trouble is that it is possible that many tracepoints could have been registered before the start time is known. I am not aware of an easy way to fix this that is more robust than just assuming that the number of tracepoints before you reach the start time is small enough that flushing doesn't happen first -- which will be true in practice, but it doesn't generalize in other implementations where you want to always flush right away.
        // printf("DEBUG: Start time written to trace file is %lld.\n", start_time);
        size_t items_written = fwrite(
                &start_time,
                sizeof(int64_t),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) _LF_TRACE_FAILURE(trace);

        // The next item in the header is the size of the
        // _lf_trace_object_descriptions table.
        // printf("DEBUG: Table size written to trace file is %d.\n", _lf_trace_object_descriptions_size);
        items_written = fwrite(
                &trace->_lf_trace_object_descriptions_size,
                sizeof(int),
                1,
                trace->_lf_trace_file
        );
        if (items_written != 1) _LF_TRACE_FAILURE(trace);

        // Next we write the table.
        for (int i = 0; i < trace->_lf_trace_object_descriptions_size; i++) {
            // printf("DEBUG: Object pointer: %p.\n", _lf_trace_object_descriptions[i].pointer);
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
            // printf("DEBUG: Object description: %s.\n", trace->_lf_trace_object_descriptions[i].description);
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
                // lf_print_error("Failed to write trace header. Trace file will be incomplete.");
                printf("Failed to write trace header. Trace file will be incomplete."); // FIXME
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
    // FIXME: location of trace file should be customizable.
    trace->_lf_trace_file = fopen(trace->filename, "w");
    if (trace->_lf_trace_file == NULL) {
        fprintf(stderr, "WARNING: Failed to open log file with error code %d."
                "No log will be written.\n", errno);
    }
    // Do not write the trace header information to the file yet
    // so that startup reactions can register user-defined trace objects.
    // write_trace_header();
    trace->_lf_trace_header_written = false;

    // Allocate an array of arrays of trace records, one per worker thread plus one
    // for the 0 thread (the main thread, or in an single-threaded program, the only
    // thread).
    trace->_lf_number_of_trace_buffers = max_num_local_threads;
    trace->_lf_trace_buffer = (trace_record_nodeps_t**)malloc(sizeof(trace_record_nodeps_t*) * trace->_lf_number_of_trace_buffers);
    for (int i = 0; i < trace->_lf_number_of_trace_buffers; i++) {
        trace->_lf_trace_buffer[i] = (trace_record_nodeps_t*)malloc(sizeof(trace_record_nodeps_t) * TRACE_BUFFER_CAPACITY);
    }
    // Array of counters that track the size of each trace record (per thread).
    trace->_lf_trace_buffer_size = (int*)calloc(sizeof(int), trace->_lf_number_of_trace_buffers);

    trace->_lf_trace_stop = 0;
    // LF_PRINT_DEBUG("Started tracing."); // FIXME
}

static void trace_new(char * filename) {

    // trace->_lf_trace_stop=1; // FIXME: This was in the original code and IDK why
    // trace->env = env;

    // Determine length of the filename
    size_t len = strlen(filename) + 1;

    // Allocate memory for the filename on the trace struct
    trace.filename = (char*) malloc(len * sizeof(char));
    // LF_ASSERT(trace.filename, "Out of memory");
    assert(trace.filename); // FIXME

    // Copy it to the struct
    strncpy(trace.filename, filename, len);
}

static void trace_free(trace_t *trace) {
    free(trace->filename);
}

static void stop_trace_locked(trace_t* trace) {
    if (trace->_lf_trace_stop) {
        // Trace was already stopped. Nothing to do.
        return;
    }
    // In multithreaded execution, thread 0 invokes wrapup reactions, so we
    // put that trace last. However, it could also include some startup events.
    // In any case, the trace file does not guarantee any ordering.
    for (int i = 1; i < trace->_lf_number_of_trace_buffers; i++) {
        // Flush the buffer if it has data.
        printf("DEBUG: Trace buffer %d has %d records.\n", i, trace->_lf_trace_buffer_size[i]);
        if (trace->_lf_trace_buffer_size && trace->_lf_trace_buffer_size[i] > 0) {
            flush_trace_locked(trace, i);
        }
    }
    if (trace->_lf_trace_buffer_size && trace->_lf_trace_buffer_size[0] > 0) {
        flush_trace_locked(trace, 0);
    }
    trace->_lf_trace_stop = 1;
    if (trace->_lf_trace_file != NULL) {
        fclose(trace->_lf_trace_file);
        trace->_lf_trace_file = NULL;
    }
    // LF_PRINT_DEBUG("Stopped tracing.");
    printf("Stopped tracing."); // FIXME
}

static void stop_trace(trace_t* trace) {
    lf_platform_mutex_lock(trace_mutex);
    stop_trace_locked(trace);
    lf_platform_mutex_unlock(trace_mutex);
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

void tracepoint(int worker, trace_record_nodeps_t* tr) {
    printf("default tracepoint called in worker %d\n", worker);
    // trace_t* trace = (trace_t*) trace_void;

    // environment_t *env = trace->env;
    // Worker argument determines which buffer to write to.
    int index = (worker >= 0) ? worker : 0;

    // Flush the buffer if it is full.
    if (trace._lf_trace_buffer_size[index] >= TRACE_BUFFER_CAPACITY) {
        // No more room in the buffer. Write the buffer to the file.
        flush_trace(&trace, index);
    }
    // The above flush_trace resets the write pointer.
    int i = trace._lf_trace_buffer_size[index];
    // Write to memory buffer.
    // Get the correct time of the event

    trace._lf_trace_buffer[index][i] = *tr;
    trace._lf_trace_buffer_size[index]++;
}

void lf_tracing_global_init(int process_id, int max_num_local_threads) {
    printf("default lf_tracing_global_init called\n");
    trace_mutex = lf_platform_mutex_new();
    if (!trace_mutex) {
        fprintf(stderr, "WARNING: Failed to initialize trace mutex.\n");
        exit(1);
    }
    char filename[100];
    sprintf(filename, "trace_%d.lft", process_id);
    trace_new(filename);
    start_trace(&trace, max_num_local_threads);
}
void lf_tracing_set_start_time(int64_t time) {
    printf("default lf_tracing_set_start_time called\n");
    start_time = time;
}
void lf_tracing_global_shutdown() {
    printf("default lf_tracing_global_shutdown called\n");
    stop_trace(&trace);
    trace_free(&trace);
    lf_platform_mutex_free(trace_mutex);
}
