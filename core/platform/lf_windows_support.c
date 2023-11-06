#ifdef PLATFORM_Windows
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
 *
 * All functions return 0 on success.
 *
 * @see https://gist.github.com/Soroosh129/127d1893fa4c1da6d3e1db33381bb273
 */

#include <windows.h>  // Order in which windows.h is included does matter!
#include <errno.h>
#include <process.h>
#include <sysinfoapi.h>
#include <time.h>

#include "lf_windows_support.h"
#include "platform.h"
#include "tag.h"
#include "util.h"

#define LF_MIN_SLEEP_NS USEC(10)

/**
 * Indicate whether or not the underlying hardware
 * supports Windows' high-resolution counter. It should
 * always be supported for Windows Xp and later.
 */
int _lf_use_performance_counter = 0;

/**
 * The denominator to convert the performance counter
 * to nanoseconds.
 */
double _lf_frequency_to_ns = 1.0;

void _lf_initialize_clock() {
    // Check if the performance counter is available
    LARGE_INTEGER performance_frequency;
    _lf_use_performance_counter = QueryPerformanceFrequency(&performance_frequency);
    if (_lf_use_performance_counter) {
        _lf_frequency_to_ns = (double)performance_frequency.QuadPart / BILLION;
    } else {
        lf_print_error(
            "High resolution performance counter is not supported on this machine.");
        _lf_frequency_to_ns = 0.01;
    }
}


/**
 * Fetch the value of the physical clock (see lf_windows_support.h) and store it in t.
 * The timestamp value in 't' will be based on QueryPerformanceCounter, adjusted to
 * reflect time passed in nanoseconds, on most modern Windows systems.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set to EINVAL or EFAULT.
 */
int _lf_clock_now(instant_t* t) {
    // Adapted from gclib/GResUsage.cpp
    // (https://github.com/gpertea/gclib/blob/8aee376774ccb2f3bd3f8e3bf1c9df1528ac7c5b/GResUsage.cpp)
    // License: https://github.com/gpertea/gclib/blob/master/LICENSE.txt
    int result = -1;
    if (t == NULL) {
        // The t argument address references invalid memory
        errno = EFAULT;
        return result;
    }
    LARGE_INTEGER windows_time;
    if (_lf_use_performance_counter) {
        int result = QueryPerformanceCounter(&windows_time);
        if ( result == 0) {
            lf_print_error("_lf_clock_now(): Failed to read the value of the physical clock.");
            return result;
        }
    } else {
        FILETIME f;
        GetSystemTimeAsFileTime(&f);
        windows_time.QuadPart = f.dwHighDateTime;
        windows_time.QuadPart <<= 32;
        windows_time.QuadPart |= f.dwLowDateTime;
    }
    *t = (instant_t)((double)windows_time.QuadPart / _lf_frequency_to_ns);
    return (0);
}

/**
 * Pause execution for a number of nanoseconds.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set to
 *   - EINTR: The sleep was interrupted by a signal handler
 *   - EINVAL: All other errors
 */
int lf_sleep(interval_t sleep_duration) {
    /* Declarations */
    HANDLE timer;	/* Timer handle */
    LARGE_INTEGER li;	/* Time defintion */
    /* Create timer */
    if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL))) {
        return FALSE;
    }
    /**
    * Set timer properties.
    * A negative number indicates relative time to wait.
    * The requested sleep duration must be in number of 100 nanoseconds.
    */
    li.QuadPart = -1 * (sleep_duration / 100);
    if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
        CloseHandle(timer);
        return FALSE;
    }
    /* Start & wait for timer */
    WaitForSingleObject(timer, INFINITE);
    /* Clean resources */
    CloseHandle(timer);
    /* Slept without problems */
    return TRUE;
}

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
    interval_t sleep_duration = wakeup_time - lf_time_physical();

    if (sleep_duration < LF_MIN_SLEEP_NS) {
        return 0;
    } else {
        return lf_sleep(sleep_duration);
    }
}

int lf_nanosleep(interval_t sleep_duration) {
    return lf_sleep(sleep_duration);
}

#if defined(LF_SINGLE_THREADED)
#include "lf_os_single_threaded_support.c"
#endif


#if !defined(LF_SINGLE_THREADED)
int lf_available_cores() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

#if __STDC_VERSION__ < 201112L || defined (__STDC_NO_THREADS__) // (Not C++11 or later) or no threads support

int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
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
int lf_thread_join(lf_thread_t thread, void** thread_return) {
    DWORD retvalue = WaitForSingleObject(thread, INFINITE);
    if(retvalue == WAIT_FAILED){
        return EINVAL;
    }
    return 0;
}

int lf_mutex_init(_lf_critical_section_t* critical_section) {
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
int lf_mutex_lock(_lf_critical_section_t* critical_section) {
    // The following Windows API does not return a value. It can
    // raise a EXCEPTION_POSSIBLE_DEADLOCK. See synchapi.h.
    EnterCriticalSection((PCRITICAL_SECTION)critical_section);
    return 0;
}

int lf_mutex_unlock(_lf_critical_section_t* critical_section) {
    // The following Windows API does not return a value.
    LeaveCriticalSection((PCRITICAL_SECTION)critical_section);
    return 0;
}

int lf_cond_init(lf_cond_t* cond, _lf_critical_section_t* critical_section) {
    // The following Windows API does not return a value.
    cond->critical_section = critical_section;
    InitializeConditionVariable((PCONDITION_VARIABLE)&cond->condition);
    return 0;
}

int lf_cond_broadcast(lf_cond_t* cond) {
    // The following Windows API does not return a value.
    WakeAllConditionVariable((PCONDITION_VARIABLE)&cond->condition);
    return 0;
}

int lf_cond_signal(lf_cond_t* cond) {
    // The following Windows API does not return a value.
    WakeConditionVariable((PCONDITION_VARIABLE)&cond->condition);
    return 0;
}

int lf_cond_wait(lf_cond_t* cond) {
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

int lf_cond_timedwait(lf_cond_t* cond, instant_t absolute_time_ns) {
    // Convert the absolute time to a relative time
    instant_t current_time_ns;
    _lf_clock_now(&current_time_ns);
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
        return LF_TIMEOUT;
      }
      return -1;
    }

    // Success
    return 0;
}
#else // If there is C11 support
#include "lf_C11_threads_support.c"
#endif 
#endif


#endif
