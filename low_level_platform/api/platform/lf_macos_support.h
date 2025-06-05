/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief MacOS API support for the C target of Lingua Franca.
 */

#ifndef LF_MACOS_SUPPORT_H
#define LF_MACOS_SUPPORT_H

#include <stdint.h> // For fixed-width integral types

// Use 64-bit times and 32-bit unsigned microsteps
#include "lf_tag_64_32.h"

#if !defined LF_SINGLE_THREADED
#include "lf_POSIX_threads_support.h"
#endif

#endif // LF_MACOS_SUPPORT_H
