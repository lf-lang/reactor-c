#include <stdio.h> // debugging only
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "trace.h"
#include "platform.h"
#include "logging_macros.h"
#include "trace_impl.h"

/** Macro to use when access to trace file fails. */
#define _LF_TRACE_FAILURE(trace)                                                                                       \
  do {                                                                                                                 \
    fprintf(stderr, "WARNING: Access to trace file failed.\n");                                                        \
    fclose(trace->_lf_trace_file);                                                                                     \
    trace->_lf_trace_file = NULL;                                                                                      \
    return -1;                                                                                                         \
  } while (0)

// PRIVATE DATA STRUCTURES ***************************************************

static lf_platform_mutex_ptr_t trace_mutex;
static trace_t trace;
static int process_id;
static int64_t start_time;
static version_t version = {.build_config =
                                {
                                    .single_threaded = TRIBOOL_DOES_NOT_MATTER,
#ifdef NDEBUG
                                    .build_type_is_debug = TRIBOOL_FALSE,
#else
                                    .build_type_is_debug = TRIBOOL_TRUE,
#endif
                                    .log_level = LOG_LEVEL,
                                },
                            .core_version_name = NULL};

// PRIVATE HELPERS ***********************************************************

/**
 * Write the trace header information.
 * See trace.h.
 * @return The number of items written to the object table or -1 for failure.
 */
static int write_trace_header(trace_t* t) {
  if (t->_lf_trace_file != NULL) {
    size_t items_written = fwrite(&start_time, sizeof(int64_t), 1, t->_lf_trace_file);
    if (items_written != 1)
      _LF_TRACE_FAILURE(t);

    // The next item in the header is the size of the
    // _lf_trace_object_descriptions table.
    items_written = fwrite(&t->_lf_trace_object_descriptions_size, sizeof(int), 1, t->_lf_trace_file);
    if (items_written != 1)
      _LF_TRACE_FAILURE(t);

    // Next we write the table.
    for (size_t i = 0; i < t->_lf_trace_object_descriptions_size; i++) {
      // Write the pointer to the self struct.
      items_written = fwrite(&t->_lf_trace_object_descriptions[i].pointer, sizeof(void*), 1, t->_lf_trace_file);
      if (items_written != 1)
        _LF_TRACE_FAILURE(t);

      // Write the pointer to the trigger_t struct.
      items_written = fwrite(&t->_lf_trace_object_descriptions[i].trigger, sizeof(void*), 1, t->_lf_trace_file);
      if (items_written != 1)
        _LF_TRACE_FAILURE(t);

      // Write the object type.
      items_written = fwrite(&t->_lf_trace_object_descriptions[i].type, // Write the pointer value.
                             sizeof(_lf_trace_object_t), 1, t->_lf_trace_file);
      if (items_written != 1)
        _LF_TRACE_FAILURE(t);

      // Write the description.
      size_t description_size = strlen(t->_lf_trace_object_descriptions[i].description);
      items_written = fwrite(t->_lf_trace_object_descriptions[i].description, sizeof(char),
                             description_size + 1, // Include null terminator.
                             t->_lf_trace_file);
      if (items_written != description_size + 1)
        _LF_TRACE_FAILURE(t);
    }
  }
  return (int)t->_lf_trace_object_descriptions_size;
}

/**
 * @brief Flush the specified buffer to a file.
 * This assumes the caller has entered a critical section.
 * @param worker Index specifying the trace to flush.
 */
static void flush_trace_locked(trace_t* trace, int worker) {
  if (trace->_lf_trace_stop == 0 && trace->_lf_trace_file != NULL && trace->_lf_trace_buffer_size[worker] > 0) {
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
    size_t items_written = fwrite(&trace->_lf_trace_buffer_size[worker], sizeof(int), 1, trace->_lf_trace_file);
    if (items_written != 1) {
      fprintf(stderr, "WARNING: Access to trace file failed.\n");
      fclose(trace->_lf_trace_file);
      trace->_lf_trace_file = NULL;
    } else {
      // Write the contents.
      items_written = fwrite(trace->_lf_trace_buffer[worker], sizeof(trace_record_nodeps_t),
                             trace->_lf_trace_buffer_size[worker], trace->_lf_trace_file);
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
 * @param t The trace struct.
 * @param worker Index specifying the trace to flush.
 */
static void flush_trace(trace_t* t, int worker) {
  // To avoid having more than one worker writing to the file at the same time,
  // enter a critical section.
  lf_platform_mutex_lock(trace_mutex);
  flush_trace_locked(t, worker);
  lf_platform_mutex_unlock(trace_mutex);
}

static void start_trace(trace_t* t, int max_num_local_threads) {
  // Do not write the trace header information to the file yet
  // so that startup reactions can register user-defined t objects.
  // write_trace_header();
  t->_lf_trace_header_written = false;

  // Allocate an array of arrays of trace records, one per worker thread plus one
  // for the 0 thread (the main thread, or in an single-threaded program, the only
  // thread).
  t->_lf_number_of_trace_buffers = max_num_local_threads;
  t->_lf_trace_buffer =
      (trace_record_nodeps_t**)malloc(sizeof(trace_record_nodeps_t*) * (t->_lf_number_of_trace_buffers + 1));
  t->_lf_trace_buffer++; // the buffer at index -1 is a fallback for user threads.
  for (int i = -1; i < (int)t->_lf_number_of_trace_buffers; i++) {
    t->_lf_trace_buffer[i] = (trace_record_nodeps_t*)malloc(sizeof(trace_record_nodeps_t) * TRACE_BUFFER_CAPACITY);
  }
  // Array of counters that track the size of each trace record (per thread).
  t->_lf_trace_buffer_size = (size_t*)calloc(t->_lf_number_of_trace_buffers + 1, sizeof(size_t));
  t->_lf_trace_buffer_size++;

  t->_lf_trace_stop = 0;
  LF_PRINT_DEBUG("Started tracing.");
}

static void trace_new(char* filename) {

  // Copy it to the struct
  strncpy(trace.filename, filename, TRACE_MAX_FILENAME_LENGTH);
  // FIXME: location of trace file should be customizable.
  trace._lf_trace_file = fopen(trace.filename, "w");
  if (trace._lf_trace_file == NULL) {
    fprintf(stderr,
            "WARNING: Failed to open log file with error code %d."
            "No log will be written.\n",
            errno);
  } else {
    LF_PRINT_DEBUG("Opened trace file %s.", trace.filename);
  }
}

static void stop_trace_locked(trace_t* trace) {
  if (trace->_lf_trace_stop) {
    // Trace was already stopped. Nothing to do.
    return;
  }
  for (int i = -1; i < (int)trace->_lf_number_of_trace_buffers; i++) {
    // Flush the buffer if it has data.
    LF_PRINT_DEBUG("Trace buffer %d has %zu records.", i, trace->_lf_trace_buffer_size[i]);
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

const version_t* lf_version_tracing() { return &version; }

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
  (void)worker;
  // Worker argument determines which buffer to write to.
  int tid = lf_thread_id();
  if (tid < 0) {
    // The current thread was created by the user. It is not managed by LF, its ID is not known,
    // and most importantly it does not count toward the limit on the total number of threads.
    // Therefore we should fall back to using a mutex.
    lf_platform_mutex_lock(trace_mutex);
  }
  if (tid > (int)trace._lf_number_of_trace_buffers) {
    lf_print_error_and_exit("the thread id (%d) exceeds the number of trace buffers (%zu)", tid,
                            trace._lf_number_of_trace_buffers);
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

void lf_tracing_global_init(char* process_name, char* process_names, int fedid, int max_num_local_threads) {
  (void)process_names;
  trace_mutex = lf_platform_mutex_new();
  if (!trace_mutex) {
    fprintf(stderr, "WARNING: Failed to initialize trace mutex.\n");
    exit(1);
  }
  process_id = fedid;
  char filename[100];
  if (strcmp(process_name, "rti") == 0) {
    snprintf(filename, sizeof(filename), "%s.lft", process_name);
  } else {
    snprintf(filename, sizeof(filename), "%s_%d.lft", process_name, process_id);
  }
  trace_new(filename);
  start_trace(&trace, max_num_local_threads);
}
void lf_tracing_set_start_time(int64_t time) { start_time = time; }
void lf_tracing_global_shutdown() {
  stop_trace(&trace);
  lf_platform_mutex_free(trace_mutex);
}
