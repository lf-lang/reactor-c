/* Windows API support for the C target of Lingua Franca. */

/*************
Copyright (c) 2021, The University of California at Berkeley.

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

/** Windows API support for the C target of Lingua Franca.
 *
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 *  @author{Erling Jellum <erling.r.jellum@ntnu>}
 *  
 * The API is implemented in the header files. This is also the case for Linux
 * and macos. This is to enable having both the unthreaded and the threaded API
 * available in the same program. This is needed for unthreaded programs with 
 * tracing. trace.c can then do #define _LF_TRACE and then include this file
 *  
 * All functions return 0 on success.
 *
 * @see https://gist.github.com/Soroosh129/127d1893fa4c1da6d3e1db33381bb273
 */

#ifndef LF_WINDOWS_SUPPORT_H
#define LF_WINDOWS_SUPPORT_H

#include <windows.h>
#include <process.h>
#include <windef.h>
#include <stdint.h> // For fixed-width integral types

#define _LF_TIMEOUT ETIMEDOUT
// Use 64-bit times and 32-bit unsigned microsteps
#include "lf_tag_64_32.h"
#include "tag.h"

// Forward declare lf_clock_gettime which is needed by lf_cond_timedwait
extern int lf_clock_gettime(instant_t* t);

#if defined LF_THREADED || defined _LF_TRACE
#if __STDC_VERSION__ < 201112L || defined (__STDC_NO_THREADS__) // (Not C++11 or later) or no threads support
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

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in thread.
 *
 * @return 0 on success, errno otherwise.
 */
static int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    uintptr_t handle = _beginthreadex(NULL, 0, lf_thread, arguments, 0, NULL);
    *thread = (HANDLE)handle;
    if(handle == 0){
        return errno;
    }else{
        return 0;
    }
}

/**
 * Make calling thread wait for termination of the thread.  The
 * exit status of the thread is stored in thread_return, if thread_return
 * is not NULL.
 *
 * @return 0 on success, EINVAL otherwise.
 */
static int lf_thread_join(lf_thread_t thread, void** thread_return) {
    DWORD retvalue = WaitForSingleObject(thread, INFINITE);
    if(retvalue == WAIT_FAILED){
        return EINVAL;
    }
    return 0;
}

/**
 * Initialize a critical section.
 *
 * @return 0 on success, 1 otherwise.
 */
static int lf_mutex_init(_lf_critical_section_t* critical_section) {
    // Set up a recursive mutex
    InitializeCriticalSection((PCRITICAL_SECTION)critical_section);
    if(critical_section != NULL){
        return 0;
    }else{
        return 1;
    }
}

/**
 * Lock a critical section.
 *
 * From https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-entercriticalsection:
 *    "This function can raise EXCEPTION_POSSIBLE_DEADLOCK if a wait operation on the critical section times out.
 *     The timeout interval is specified by the following registry value:
 *     HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\CriticalSectionTimeout.
 *     Do not handle a possible deadlock exception; instead, debug the application."
 *
 * @return 0
 */
static int lf_mutex_lock(_lf_critical_section_t* critical_section) {
    // The following Windows API does not return a value. It can
    // raise a EXCEPTION_POSSIBLE_DEADLOCK. See synchapi.h.
    EnterCriticalSection((PCRITICAL_SECTION)critical_section);
    return 0;
}

/**
 * Leave a critical_section.
 *
 * @return 0
 */
static int lf_mutex_unlock(_lf_critical_section_t* critical_section) {
    // The following Windows API does not return a value.
    LeaveCriticalSection((PCRITICAL_SECTION)critical_section);
    return 0;
}

/**
 * Initialize a conditional variable.
 *
 * @return 0
 */
static int lf_cond_init(lf_cond_t* cond, _lf_critical_section_t* critical_section) {
    // The following Windows API does not return a value.
    cond->critical_section = critical_section;
    InitializeConditionVariable((PCONDITION_VARIABLE)&cond->condition);
    return 0;
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0
 */
static int lf_cond_broadcast(lf_cond_t* cond) {
    // The following Windows API does not return a value.
    WakeAllConditionVariable((PCONDITION_VARIABLE)&cond->condition);
    return 0;
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0
 */
static int lf_cond_signal(lf_cond_t* cond) {
    // The following Windows API does not return a value.
    WakeConditionVariable((PCONDITION_VARIABLE)&cond->condition);
    return 0;
}

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, 1 otherwise.
 */
static int lf_cond_wait(lf_cond_t* cond) {
    // According to synchapi.h, the following Windows API returns 0 on failure,
    // and non-zero on success.
    int return_value =
     (int)SleepConditionVariableCS(
         (PCONDITION_VARIABLE)&cond->condition,
         (PCRITICAL_SECTION)cond->critical_section,
         INFINITE
     );
     switch (return_value) {
        case 0:
            // Error
            return 1;
            break;

        default:
            // Success
            return 0;
            break;
     }
}

/**
 * Block current thread on the condition variable until condition variable
 * pointed by "cond" is signaled or time pointed by "absolute_time_ns" in
 * nanoseconds is reached.
 *
 * @return 0 on success and LF_TIMEOUT on timeout, 1 otherwise.
 */
static int lf_cond_timedwait(lf_cond_t* cond, instant_t absolute_time_ns) {
    // Convert the absolute time to a relative time
    instant_t current_time_ns;
    lf_clock_gettime(&current_time_ns);
    interval_t relative_time_ns = (absolute_time_ns - current_time_ns);
    if (relative_time_ns <= 0) {
      // physical time has already caught up sufficiently and we do not need to wait anymore
      return 0;
    }

    // convert ns to ms and round up to closest full integer
    DWORD relative_time_ms = (relative_time_ns + 999999LL) / 1000000LL;

    int return_value =
     (int)SleepConditionVariableCS(
         (PCONDITION_VARIABLE)&cond->condition,
         (PCRITICAL_SECTION)cond->critical_section,
         relative_time_ms
     );
    if (return_value == 0) {
      // Error
      if (GetLastError() == ERROR_TIMEOUT) {
        return _LF_TIMEOUT;
      }
      return 1;
    }

    // Success
    return 0;
}
#else
#include "lf_C11_threads_support.h"
#endif
#endif



#endif // LF_WINDOWS_SUPPORT_H

