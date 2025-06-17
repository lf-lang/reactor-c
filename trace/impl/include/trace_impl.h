#include "trace.h"

// FIXME: Target property should specify the capacity of the trace buffer.
#define TRACE_BUFFER_CAPACITY 2048

/** Size of the table of trace objects. */
#define TRACE_OBJECT_TABLE_SIZE 1024

/** Max length of trace file name*/
#define TRACE_MAX_FILENAME_LENGTH 128

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
  size_t* _lf_trace_buffer_size;

  /** The number of trace buffers allocated when tracing starts. */
  size_t _lf_number_of_trace_buffers;

  /** Marker that tracing is stopping or has stopped. */
  int _lf_trace_stop;

  /** The file into which traces are written. */
  FILE* _lf_trace_file;

  /** The file name where the traces are written*/
  char filename[TRACE_MAX_FILENAME_LENGTH];

  /** Table of pointers to a description of the object. */
  object_description_t _lf_trace_object_descriptions[TRACE_OBJECT_TABLE_SIZE];
  size_t _lf_trace_object_descriptions_size;

  /** Indicator that the trace header information has been written to the file. */
  bool _lf_trace_header_written;

  // /** Pointer back to the environment which we are tracing within*/
  // environment_t* env;
} trace_t;
