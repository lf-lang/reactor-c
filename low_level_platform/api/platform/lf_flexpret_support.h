/**
 * @file lf_flexpret_support.h
 * @brief Platform-specific support for FlexPRET in the Lingua Franca C runtime.
 *
 * @author Magnus Mæhlum
 *
 * This header file provides platform-specific definitions and implementations
 * for running Lingua Franca programs on FlexPRET. It includes type definitions,
 * platform-specific constants, and interrupt handling mechanisms needed for
 * the runtime.
 */

#ifndef LF_FLEXPRET_SUPPORT_H
#define LF_FLEXPRET_SUPPORT_H

#include <flexpret/flexpret.h>

/**
 * Like nRF52, for FlexPRET, each mutex will control an interrupt.
 *
 * The mutex holds the interrupt number.
 * For example, a mutex might be defined for the GPIOTE peripheral interrupt number
 *
 * When initialized, the interrupt is inserted into a global linked list
 * for disabling and enabling all interrupts during sleep functions.
 * - All interrupts are disabled by default after initialization
 * - Priority levels are restricted between (0-7)
 *
 */

#include <inttypes.h> // Needed to define PRId64 and PRIu32
#define PRINTF_TIME "%" PRId64
#define PRINTF_MICROSTEP "%" PRIu32

// For convenience, the following string can be inserted in a printf
// format for printing both time and microstep as follows:
//     printf("Tag is " PRINTF_TAG "\n", time_value, microstep);
#define PRINTF_TAG "(%" PRId64 ", %" PRIu32 ")"

#if !defined(LF_SINGLE_THREADED)
typedef fp_lock_t lf_mutex_t;
typedef fp_thread_t lf_thread_t;
typedef fp_cond_t lf_cond_t;
#endif

// This will filter out some unecessary calls to standard library functions
// and save code space
#define NO_CLI
#define MINIMAL_STDLIB

/**
 * Need to include `stdio` here, because we #define `fprintf` and `vfprintf` below.
 * Since stdio.h contains declarations for these functions, including it
 * after will result in the following:
 *
 * #define fprintf(s, f, ...) printf(f, ##__VA_ARGS__)
 *
 * int	fprintf (FILE *__restrict, const char *__restrict, ...)
 *             _ATTRIBUTE ((__format__ (__printf__, 2, 3)));
 *
 * Which the preprocessor will replace with:
 *
 * int	printf (FILE *__restrict, const char *__restrict, ...)
 *             _ATTRIBUTE ((__format__ (__printf__, 2, 3)));
 *
 * Which will yield an error.
 *
 */
#include <stdio.h>

// Likewise, fprintf is used to print to `stderr`, but FlexPRET has no `stderr`
// We instead redirect its output to normal printf
// Note: Most compilers do not support passing this on the command line, so CMake
//       will drop it if you try... But that would be the better option.
#define fprintf(stream, fmt, ...) printf(fmt, ##__VA_ARGS__)
#define vfprintf(fp, fmt, args) vprintf(fmt, args)

#endif // LF_FLEXPRET_SUPPORT_H
