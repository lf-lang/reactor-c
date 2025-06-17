/**
 * @file trace_util.h
 * @author Edward A. Lee
 *
 * @brief Header file for common utilities used in converting Lingua Franca trace files
 * into other formats.
 * @ingroup Tracing
 */
#define LF_TRACE
#include "reactor.h"
#include "trace.h"

/*
 * String description of event types.
 */
extern const char* trace_event_names[];

/**
 * @brief Macro to use when access to trace file fails.
 * @ingroup Tracing
 */
#define _LF_TRACE_FAILURE(trace_file)                                                                                  \
  do {                                                                                                                 \
    fprintf(stderr, "ERROR: Access to trace file failed.\n");                                                          \
    fclose(trace_file);                                                                                                \
    trace_file = NULL;                                                                                                 \
    exit(1);                                                                                                           \
  } while (0)

/**
 * @brief Buffer size for reading object descriptions.
 * @ingroup Tracing
 */
#define BUFFER_SIZE 1024

/* Buffer for reading trace records. */
extern trace_record_t trace[];

/* File containing the trace binary data. */
extern FILE* trace_file;

/* File for writing the output data. */
extern FILE* output_file;

/* File for writing summary statistics. */
extern FILE* summary_file;

/**
 * @brief Print a usage message.
 * @ingroup Tracing
 */
void usage();

/* The start time read from the trace file. */
extern instant_t start_time;

/* Table of pointers to a description of the object. */
extern object_description_t* object_table;
extern int object_table_size;

/* Name of the top-level reactor (first entry in symbol table). */
extern char* top_level;

/**
 * @brief Return the root file name from the given path.
 * @ingroup Tracing
 *
 * Given a path to a file, this function returns a dynamically
 * allocated string (which you must free) that points to the root
 * filename without the preceding path and without the file extension.
 * @param path The path including the full filename.
 * @return The root name of the file or NULL for failure.
 */
char* root_name(const char* path);

/**
 * @brief Open the specified file for reading or writing.
 * @ingroup Tracing
 *
 * This function records the file for closing at termination.
 * @param path The path to the file.
 * @param mode "r" for reading and "w" for writing.
 * @return A pointer to the open file or NULL for failure.
 */
FILE* open_file(const char* path, const char* mode);

/**
 * @brief Get the description of the object pointed to by the specified pointer.
 * @ingroup Tracing
 *
 * For example, this can be the name of a reactor (pointer points to
 * the self struct) or a user-defined string.
 * If there is no such pointer in the symbol table, return NULL.
 * If the index argument is non-null, then put the index
 * of the entry in the table into the int pointed to
 * or -1 if none was found.
 *
 * @param reactor The pointer to to an object, e.g. a self struct.
 * @param index An optional pointer into which to write the index.
 * @return The description of the object or NULL if not found.
 */
char* get_object_description(void* reactor, int* index);

/**
 * @brief Get the trigger name for the specified pointer.
 * @ingroup Tracing
 *
 * If there is no such trigger, return NULL.
 * If the index argument is non-null, then put the index
 * of the trigger in the table into the int pointed to
 * or -1 if none was found.
 *
 * @param trigger The pointer to a trigger struct.
 * @param index An optional pointer into which to write the index.
 * @return The name of the trigger or NULL if not found.
 */
char* get_trigger_name(void* trigger, int* index);

/**
 * @brief Print the object to description table.
 * @ingroup Tracing
 */
void print_table();

/**
 * @brief Read header information.
 * @ingroup Tracing
 *
 * @return The number of objects in the object table or -1 for failure.
 */
size_t read_header();

/**
 * @brief Read the trace from the trace_file and put it in the trace global variable.
 * @ingroup Tracing
 *
 * @return The number of trace records read or 0 upon seeing an EOF.
 */
int read_trace();
