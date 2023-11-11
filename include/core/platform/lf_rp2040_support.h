/* RP2040 API support for the C target of Lingua Franca */

/**
 * @brief pico support for reactor-c
 */

#ifndef LF_RP2040_SUPPORT_H
#define LF_RP2040_SUPPORT_H

#include <pico/stdlib.h>
#include <pico/sync.h>

#define NO_TTY

// Defines for formatting time in printf for pico
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"
#define PRINTF_TIME "%lld"
#define PRINTF_MICROSTEP "%d"

#define LF_TIME_BUFFER_LENGTH 80
#define _LF_TIMEOUT 1

#endif // LF_PICO_SUPPORT_H
