/**
 * @file
 * @author Edward A. Lee
 *
 * @brief Utlitity functions for initializing parameters and state variables from a file.
 *
 * To use these functions, create a file that contains the parameter or state variable 
 * values, separated by a delimiter of your choice. For example, if you have parameters
 * `x`, `y`, and `z` of type doublethat you want to initialize from a file `params.csv`,
 * you can create a file `params.csv` that contains the values for `x`, `y`, and `z`:
 * 
 * ```csv
 * x,y,z
 * 1.0,2.0,3.0
 * 4.0,5.0,6.0
 * ```
 * Including a header row with the names of the parameters is optional (but recommended).
 * 
 * Then, you can initialize the parameters from the file `params.csv` as follows:
 * 
 * ```lf-c
 * main reactor MyReactor(x: double = 0.0, y: double = 0.0, z: double = 0.0, row_number: int = 0) {
 *   reaction(startup) {=
 *     lf_initialize_double("params.csv", ',', self->row_number, &x->value, &y->value, &z->value, NULL);
 *   =}
 * }
 * ```
 * The `row_number` parameter is the row number of the file to initialize from.
 * If the `row_number` is a top-level parameter (of the main reactor), then you
 * can override this parameter on the command line when running the program as follows:
 * 
 * ```
 * ./MyReactor --row_number=1
 * ```
 *
 * If you want to initialize state variables rather than parameters, then you can 
 * initialize them in the startup reaction as follows:
 * 
 * ```lf-c
 * reaction(startup) {=
 *   lf_initialize_double("params.csv", ',', self->row_number, &self->x, &self->y, &self->z, NULL);
 * =}
 * 
 * If you have reactors within a bank, then you can declare a `bank_index` parameter
 * and use it to calculate the row number of the file to initialize from. For example:
 * ```lf-c
 * reactor MyReactor(bank_index: int = 0) {
 *   reaction(startup) {=
 *     lf_initialize_double("params.csv", ',', self->bank_index + 1, &self->x, &self->y, &self->z, NULL);
 *   =}
 * }
 * ```
 * The `bank_index` parameter is the index within the bank, starting from 0.
 * The `+ 1` specifies to skip the header row.
 * This way, each bank member can have a different set of paramter values.
 *
 * To use this, include the following in your target properties:
 * ```
 * target C {
 *     cmake-include: "/lib/c/reactor-c/util/initialize_from_file.cmake",
 *     files: ["/lib/c/reactor-c/util/initialize_from_file.c", "/lib/c/reactor-c/util/initialize_from_file.h"]
 * };
 * ```
 * In addition, you need this in your Lingua Franca file:
 * ```
 * preamble {=
 *     #include "initialize_from_file.h"
 * =}
 * ```
 */
#ifndef INITIALIZE_FROM_FILE_H
#define INITIALIZE_FROM_FILE_H

#include <stddef.h> // Defines size_t

#define SC_CSV_LINE_MAX 256
#define SC_CSV_MAX_COLS 256

/**
 * @brief Read one delimited row and parse as doubles.
 * @ingroup Utilities
 *
 * The variadic arguments are double* pointers, terminated with NULL.
 * Example:
 * ```
 *   double a, b;
 *   lf_read_numeric_row("x.csv", ',', 3, &a, &b, NULL);
 * ```
 */
int lf_initialize_double(const char* filename, char delimiter, size_t row_number, ...);

#endif // INITIALIZE_FROM_FILE_H