/* Arduino Platform API support for the C target of Lingua Franca. */

/*************
Copyright (c) 2022, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/* Arduino Platform API support for the C target of Lingua Franca.
 *  
 *  @author{Anirudh Rengarajan <arengarajan@berkeley.edu>}
 */

#ifndef LF_ARDUINO_SUPPORT_H
#define LF_ARDUINO_SUPPORT_H

#include <stdint.h> // For fixed-width integral types
#include <time.h>   // For CLOCK_MONOTONIC
#include <stdbool.h>

#define AVR 0
#define megaAVR 1
#define SAMD 2
#define SAM 3
#define MBED 4

#ifndef BOARD
#if defined(ARDUINO_AVR_ADK)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_BT)    // Bluetooth
    #define BOARD AVR
#elif defined(ARDUINO_AVR_DUEMILANOVE)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_ESPLORA)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_ETHERNET)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_FIO)      
    #define BOARD AVR
#elif defined(ARDUINO_AVR_GEMMA)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_LEONARDO)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_LILYPAD)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_LILYPAD_USB)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_MEGA)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_MEGA2560)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_MICRO)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_MINI)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_NANO)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_NG)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_PRO)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_ROBOT_CONTROL)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_ROBOT_MOTOR)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_UNO) || defined(__AVR_ATmega4809__)
    #define BOARD AVR
#elif defined(ARDUINO_AVR_YUN)
    #define BOARD AVR

// These boards must be installed separately:
#elif defined(ARDUINO_SAM_DUE)
    #define BOARD SAM
#elif defined(ARDUINO_SAMD_ZERO)
    #define BOARD SAMD
#elif defined(ARDUINO_ARC32_TOOLS)
    #define BOARD SAM
#elif defined(ARDUINO_ARDUINO_NANO33BLE)
    #define BOARD MBED
#endif
#endif

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifndef __timespec_defined
#define	__timespec_defined
#ifndef _SYS__TIMESPEC_H_
#define _SYS__TIMESPEC_H_
struct timespec {
	long long	tv_sec;		/* seconds */
	long	tv_nsec;	/* and nanoseconds */
};
#endif
#endif

#if !defined(LF_SINGLE_THREADED)
#warning "Threaded support on Arduino is still experimental"

typedef void* lf_mutex_t;
typedef void* lf_cond_t;
typedef void* lf_thread_t;

#endif // !LF_SNIGLE_THREADED

#define PRINTF_TIME "%" PRIu32
#define PRINTF_MICROSTEP "%" PRIu32
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"

#define LLONG_MAX __LONG_LONG_MAX__
#define LLONG_MIN (-LLONG_MAX - 1LL)
#define ULLONG_MAX (LLONG_MAX * 2ULL + 1ULL)

// Arduinos are embedded platforms with no tty
#define NO_TTY

#endif // LF_ARDUINO_SUPPORT_H
