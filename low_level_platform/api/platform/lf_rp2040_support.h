/**
 * @file lf_rp2040_support.h
 * @brief Platform-specific support for Raspberry Pi Pico (RP2040) in the Lingua Franca C runtime.
 *
 * @author Edward A. Lee
 *
 * This header file provides platform-specific definitions and implementations
 * for running Lingua Franca programs on the Raspberry Pi Pico (RP2040) microcontroller.
 * It includes type definitions, platform-specific constants, and synchronization
 * primitives needed for the runtime.
 */

/**
 * @brief pico support for reactor-c
 */

#ifndef LF_RP2040_SUPPORT_H
#define LF_RP2040_SUPPORT_H

#include <pico/stdlib.h>
#include <pico/sync.h>

#define NO_CLI
#define MINIMAL_STDLIB

// Defines for formatting time in printf for pico
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"
#define PRINTF_TIME "%lld"
#define PRINTF_MICROSTEP "%d"

#define LF_TIME_BUFFER_LENGTH 80
#define _LF_TIMEOUT 1

#ifndef LF_SINGLE_THREADED
#warning "Threaded support on rp2040 is still experimental"

typedef recursive_mutex_t lf_mutex_t;
typedef struct {
  semaphore_t notifs[NUM_CORES];
  lf_mutex_t* mutex;
} lf_cond_t;
typedef int lf_thread_t;

#endif // LF_SINGLE_THREADED

#endif // LF_PICO_SUPPORT_H
