/* STM32 API support for the C target of Lingua Franca. */

#ifndef LF_STM32F4_SUPPORT_H
#define LF_STM32F4_SUPPORT_H

// I have no idea what the fuck TTY is so i guess we dont support it
#define NO_TTY

#include <stm32f4xx_hal.h>

// Defines for formatting time in printf for pico
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"
#define PRINTF_TIME "%lld"
#define PRINTF_MICROSTEP "%d"

#define LF_TIME_BUFFER_LENGTH 80
#define _LF_TIMEOUT 1

#ifdef LF_THREADED
#error "I have no idea how to support threading"
#endif // LF_THREADED

#endif