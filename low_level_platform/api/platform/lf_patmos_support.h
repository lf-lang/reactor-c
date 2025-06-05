/**
 * @file
 * @brief Patmos API support for the C target of Lingua Franca.
 *
 * This is based on lf_nrf_support.h in icyphy/lf-buckler.
 *
 * @author Ehsan Khodadad
 * @author Luca Pezzarossa
 * @author Martin Schoeberl
 */

#ifndef LF_PATMOS_SUPPORT_H
#define LF_PATMOS_SUPPORT_H

// This embedded platform has no TTY suport
#define NO_TTY

#include <stdint.h> // For fixed-width integral types
#include <stdbool.h>

#include <inttypes.h> // Needed to define PRId64 and PRIu32
#define PRINTF_TIME "%" PRId64
#define PRINTF_MICROSTEP "%" PRIu32
#define PRINTF_TAG "(%" PRId64 ", %" PRIu32 ")"

#endif // LF_PATMOS_SUPPORT_H
