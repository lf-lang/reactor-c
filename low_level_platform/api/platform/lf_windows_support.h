/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Erling Jellum
 *
 * @brief Windows API support for the C target of Lingua Franca.
 *
 * The API is implemented in the header files. This is also the case for Linux
 * and macos.
 *
 * All functions return 0 on success.
 *
 * @see https://gist.github.com/Soroosh129/127d1893fa4c1da6d3e1db33381bb273
 */

#ifndef LF_WINDOWS_SUPPORT_H
#define LF_WINDOWS_SUPPORT_H

#include <stdint.h> // For fixed-width integral types
#include <windows.h>

#if !defined LF_SINGLE_THREADED
/**
 * On Windows, one could use both a mutex or
 * a critical section for the same purpose. However,
 * critical sections are lighter and limited to one process
 * and thus fit the requirements of Lingua Franca.
 */
typedef CRITICAL_SECTION lf_mutex_t;
/**
 * For compatibility with other platform APIs, we assume
 * that mutex is analogous to critical section.
 */
typedef lf_mutex_t _lf_critical_section_t;
typedef struct {
  _lf_critical_section_t* critical_section;
  CONDITION_VARIABLE condition;
} lf_cond_t;
typedef HANDLE lf_thread_t;
#endif

// Use 64-bit times and 32-bit unsigned microsteps
#include "lf_tag_64_32.h"

#endif // LF_WINDOWS_SUPPORT_H
