/**
 * @file
 * @author Edward A. Lee
 *
 * @brief Utility functions for initializing parameters and state variables from a file.
 * @ingroup Utilities
 */
#ifndef INITIALIZE_FROM_FILE_H
#define INITIALIZE_FROM_FILE_H

#include <stddef.h> // Defines size_t
#include "reactor.h"
#include "lf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SC_CSV_LINE_MAX 256
#define SC_CSV_MAX_COLS 256

/**
 * @brief Read one delimited row from a file and parse as doubles.
 * @ingroup Utilities
 *
 * Given a file path (either absolute or relative to the current working directory),
 * this function reads the specified row, which it assumes is a list of doubles separated by a delimiter,
 * and parses the values into a list of specified variables given as a list of double* pointers
 * terminated with NULL.
 * Example:
 * ```
 *   double a, b;
 *   int count = lf_initialize_double("x.csv", ',', 2, &a, &b, NULL);
 * ```
 * This will read the third row of the file "x.csv" (row numbers start from 0)
 * and parse the values into the variables `a` and `b`.
 * The file "x.csv" may look like this::
 * ```csv
 * a,b
 * 1.0,2.0
 * 3.0,4.0
 * ```
 * Including a header row with the names of the variables is optional (but recommended).
 *
 * The return value is the number of values parsed (2 in this case), or -1 if an error occurred,
 * for example if the file does not exist or the row number is out of range.
 *
 * To use this to initialize parameters and/or state variables of a reactor, you can do the following:
 *
 * ```lf
 * main reactor MyReactor(x: double = 0.0, row_number: int = 0) {
 *   state y: double = 0.0;
 *   reaction(startup) {=
 *     lf_initialize_double("params.csv", ',', self->row_number, &self->x, &self->y, NULL);
 *   =}
 * }
 * ```
 *
 * If the `row_number` is a top-level parameter (of the main reactor), as it is above, then you
 * can override this parameter on the command line when running the program as follows:
 *
 * ```
 * bin/MyReactor --row_number 1
 * ```
 * This gives an easy way to compile once and run with different parameter values.
 *
 * If you wish to initialize parameters or state variables of a reactor within a bank, you
 * can create a CSV file with one row per bank member and use the `bank_index` parameter to
 * select the row to read. For example:
 * ```lf
 * reactor MyReactor(bank_index: int = 0) {
 *   reaction(startup) {=
 *     lf_initialize_double("params.csv", ',', self->bank_index + 1, &self->x, &self->y, NULL);
 *   =}
 * }
 * ```
 * The `bank_index` parameter is the index within the bank, starting from 0.
 * The `+ 1` specifies to skip the header row.
 * This way, each bank member can have a different set of parameter values.
 *
 * To use this in a Lingua Franca program, you must include the following in the target declaration:
 * ```lf
 * target C {
 *  cmake-include: "/lib/c/reactor-c/util/initialize_from_file.cmake",
 *  files: [
 *      "/lib/c/reactor-c/util/initialize_from_file.c",
 *      "/lib/c/reactor-c/util/initialize_from_file.h"]
 * }
 * ```
 *
 * Then, in the reactor, you can include the header file as follows:
 * ```lf
 * reactor MyReactor {
 *   preamble {=
 *     #include "initialize_from_file.h"
 *   =}
 *   ...
 * }
 * ```
 *
 * @param filename The name of the file to read.
 * @param delimiter The delimiter character to use.
 * @param row_number The row number in the file to read.
 * @param ... The double* pointers to the variables to store the values in, terminated with NULL.
 * @return The number of values parsed, or -1 if an error occurred.
 */
int lf_initialize_double(const char* filename, char delimiter, size_t row_number, ...);

/**
 * @brief Read one delimited row from a file and parse as integers.
 * @ingroup Utilities
 *
 * Given a file path (either absolute or relative to the current working directory),
 * this function reads the specified row, which it assumes is a list of integers separated by a delimiter,
 * and parses the values into a list of specified variables given as a list of int* pointers
 * terminated with NULL.
 * Example:
 * ```
 *   int a, b;
 *   int count = lf_initialize_int("x.csv", ',', 2, &a, &b, NULL);
 * ```
 * This will read the third row of the file "x.csv" (row numbers start from 0)
 * and parse the values into the variables `a` and `b`.
 * The file "x.csv" may look like this::
 * ```csv
 * a,b
 * 1,2
 * 3,4
 * ```
 * Including a header row with the names of the variables is optional (but recommended).
 *
 * The return value is the number of values parsed (2 in this case), or -1 if an error occurred,
 * for example if the file does not exist or the row number is out of range.
 *
 * To use this to initialize parameters and/or state variables of a reactor, you can do the following:
 *
 * ```lf
 * main reactor MyReactor(x: int = 0, row_number: int = 0) {
 *   state y: int = 0;
 *   reaction(startup) {=
 *     lf_initialize_int("params.csv", ',', self->row_number, &self->x, &self->y, NULL);
 *   =}
 * }
 * ```
 *
 * If the `row_number` is a top-level parameter (of the main reactor), as it is above, then you
 * can override this parameter on the command line when running the program as follows:
 *
 * ```
 * bin/MyReactor --row_number 1
 * ```
 * This gives an easy way to compile once and run with different parameter values.
 *
 * If you wish to initialize parameters or state variables of a reactor within a bank, you
 * can create a CSV file with one row per bank member and use the `bank_index` parameter to
 * select the row to read. For example:
 * ```lf
 * reactor MyReactor(bank_index: int = 0) {
 *   reaction(startup) {=
 *     lf_initialize_int("params.csv", ',', self->bank_index + 1, &self->x, &self->y, NULL);
 *   =}
 * }
 * ```
 * The `bank_index` parameter is the index within the bank, starting from 0.
 * The `+ 1` specifies to skip the header row.
 * This way, each bank member can have a different set of parameter values.
 *
 * To use this in a Lingua Franca program, you must include the following in the target declaration:
 * ```lf
 * target C {
 *  cmake-include: "/lib/c/reactor-c/util/initialize_from_file.cmake",
 *  files: [
 *      "/lib/c/reactor-c/util/initialize_from_file.c",
 *      "/lib/c/reactor-c/util/initialize_from_file.h"]
 * }
 * ```
 *
 * Then, in the reactor, you can include the header file as follows:
 * ```lf
 * reactor MyReactor {
 *   preamble {=
 *     #include "initialize_from_file.h"
 *   =}
 *   ...
 * }
 * ```
 *
 * @param filename The name of the file to read.
 * @param delimiter The delimiter character to use.
 * @param row_number The row number in the file to read.
 * @param ... The int* pointers to the variables to store the values in, terminated with NULL.
 * @return The number of values parsed, or -1 if an error occurred.
 */
int lf_initialize_int(const char* filename, char delimiter, size_t row_number, ...);

/**
 * @brief Read one delimited row from a file and parse as strings.
 * @ingroup Utilities
 *
 * Given a file path (either absolute or relative to the current working directory),
 * this function reads the specified row, which it assumes is a list of strings separated by a delimiter,
 * and parses the values into a list of specified variables given as a list of char** pointers
 * terminated with NULL. If a field is enclosed in quotation marks (double or single), the marks
 * are stripped. Memory for each string is allocated using `lf_allocate` and recorded in the
 * reactor's allocation list so that it is freed automatically when the reactor is freed.
 *
 * This macro is meant to be called from a reaction, not directly. If you wish to call it from
 * somewhere other than a reaction, you can use the following function:
 * ```lf
 * int _lf_initialize_string(const char* filename, char delimiter, size_t row_number,
 *                          struct allocation_record_t** allocations, ...);
 * ```
 * If you pass NULL for the allocations parameter, then you will be responsible for freeing the allocated memory.
 * Example:
 * ```
 *   char* a;
 *   char* b;
 *   int count = _lf_initialize_string("x.csv", ',', 2, NULL, &a, &b, NULL);
 *   ...
 *   free(a);
 *   free(b);
 * ```
 *
 * This will read the third row of the file "x.csv" (row numbers start from 0)
 * and parse the values into the variables `a` and `b`.
 * The file "x.csv" may look like this::
 * ```csv
 * a,b
 * "hello","world"
 * foo,bar
 * ```
 * The quotation marks are optional and will be stripped if present.
 * Note that putting a delimiter within the quotation marks is not supported and will give unpredictable results.
 * Including a header row with the names of the variables is optional (but recommended).
 *
 * The return value is the number of values parsed (2 in this case), or -1 if an error occurred,
 * for example if the file does not exist or the row number is out of range.
 *
 * To use this to initialize parameters and/or state variables of a reactor, you can do the following:
 *
 * ```lf
 * main reactor MyReactor(row_number: int = 0) {
 *   state name: string = "";
 *   reaction(startup) {=
 *     lf_initialize_string("params.csv", ',', self->row_number, &self->name, NULL);
 *   =}
 * }
 * ```
 *
 * If the `row_number` is a top-level parameter (of the main reactor), as it is above, then you
 * can override this parameter on the command line when running the program as follows:
 *
 * ```
 * bin/MyReactor --row_number 1
 * ```
 * This gives an easy way to compile once and run with different parameter values.
 *
 * If you wish to initialize parameters or state variables of a reactor within a bank, you
 * can create a CSV file with one row per bank member and use the `bank_index` parameter to
 * select the row to read. For example:
 * ```lf
 * reactor MyReactor(bank_index: int = 0) {
 *   reaction(startup) {=
 *     lf_initialize_string("params.csv", ',', self->bank_index + 1, &self->name, NULL);
 *   =}
 * }
 * ```
 * The `bank_index` parameter is the index within the bank, starting from 0.
 * The `+ 1` specifies to skip the header row.
 * This way, each bank member can have a different set of parameter values.
 *
 * To use this in a Lingua Franca program, you must include the following in the target declaration:
 * ```lf
 * target C {
 *  cmake-include: "/lib/c/reactor-c/util/initialize_from_file.cmake",
 *  files: [
 *      "/lib/c/reactor-c/util/initialize_from_file.c",
 *      "/lib/c/reactor-c/util/initialize_from_file.h"]
 * }
 * ```
 *
 * Then, in the reactor, you can include the header file as follows:
 * ```lf
 * reactor MyReactor {
 *   preamble {=
 *     #include "initialize_from_file.h"
 *   =}
 *   ...
 * }
 * ```
 *
 * @param filename The name of the file to read.
 * @param delimiter The delimiter character to use.
 * @param row_number The row number in the file to read.
 * @param ... The char** pointers to the variables to store the values in, terminated with NULL.
 * @return The number of values parsed, or -1 if an error occurred.
 */
#define lf_initialize_string(filename, delimiter, row_number, ...)                                                     \
  _lf_initialize_string(filename, delimiter, row_number, &((self_base_t*)self)->allocations, __VA_ARGS__)

int _lf_initialize_string(const char* filename, char delimiter, size_t row_number,
                          struct allocation_record_t** allocations, ...);

#ifdef __cplusplus
}
#endif

#endif // INITIALIZE_FROM_FILE_H