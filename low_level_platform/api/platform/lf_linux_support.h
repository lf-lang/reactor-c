/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief Linux API support for the C target of Lingua Franca.
 */

#ifndef LF_LINUX_SUPPORT_H
#define LF_LINUX_SUPPORT_H

#include <stdint.h> // For fixed-width integral types
#include <time.h>   // For CLOCK_MONOTONIC
#include <unistd.h> // _POSIX_TIMERS _POSIX_CLOCK_MONOTONIC

// Use 64-bit times and 32-bit unsigned microsteps
#include "lf_tag_64_32.h"

#if !defined LF_SINGLE_THREADED
#include "lf_POSIX_threads_support.h"
#endif

#if !defined(_POSIX_TIMERS) || _POSIX_TIMERS <= 0
#error Linux platform misses clock support
#endif

#endif // LF_LINUX_SUPPORT_H
