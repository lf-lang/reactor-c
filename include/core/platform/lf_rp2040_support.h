/* RP2040 API support for the C target of Lingua Franca */

/**
 * @brief pico support for reactor-c
 */

#ifndef LF_PICO_SUPPORT_H
#define LF_PICO_SUPPORT_H

#include <pico/stdlib.h>
#include <pico/sync.h>

// Defines for time and microstep data types
#define instant_t int64_t
#define interval_t int64_t
#define microstep_t uint32_t

// Defines for formatting time in printf for pico
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"
#define PRINTF_TIME "%lld"
#define PRINTF_MICROSTEP "%d"

#define LF_TIME_BUFFER_LENGTH 80
#define _LF_TIMEOUT 1

#ifdef LF_THREADED
// TODO: if threaded, use the free-rtos defines
#endif // LF_THREADED 

#endif // LF_PICO_SUPPORT_H
