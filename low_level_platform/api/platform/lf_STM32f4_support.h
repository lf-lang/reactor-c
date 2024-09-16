/* STM32 API support for the C target of Lingua Franca. */

#ifndef LF_STM32F4_SUPPORT_H
#define LF_STM32F4_SUPPORT_H

#define NO_TTY

#include <stm32f4xx_hal.h>

#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"
#define PRINTF_TIME "%lld"
#define PRINTF_MICROSTEP "%d"

#define LF_TIME_BUFFER_LENGTH 80
#define _LF_TIMEOUT 1

#ifdef LF_THREADED
#error "Threaded runtime not supported on STM32"
#endif // LF_THREADED

#endif