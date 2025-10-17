/**
 * @file
 * @author Soroush Bateni
 * @author Abhi Gundrala
 * @author Erling Rennemo Jellum
 *
 * @brief nRF52 API support for the C target of Lingua Franca.
 */

#ifndef LF_NRF52_SUPPORT_H
#define LF_NRF52_SUPPORT_H

// This embedded platform has no command line interface
#define NO_CLI
#define MINIMAL_STDLIB

#include <stdint.h> // For fixed-width integral types
#include <stdbool.h>

#include <inttypes.h> // Needed to define PRId64 and PRIu32
#define PRINTF_TIME "%" PRId64
#define PRINTF_MICROSTEP "%" PRIu32
#define PRINTF_TAG "(%" PRId64 ", %" PRIu32 ")"

/**
 * No mutex or condition variable needed for single threaded NRF platforms
 */
typedef void* lf_mutex_t;
typedef void _lf_cond_var_t;

#endif // LF_nRF52832_SUPPORT_H
