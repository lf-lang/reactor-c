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

#define LF_MIN_SLEEP_NS USEC(10)

#if defined LF_THREADED || defined _LF_TRACE

/**
 * @brief Get the number of cores on the host machine.
 */
int lf_available_cores() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

#else
#include "lf_os_single_threaded_support.c"
#endif

/**
 * Initialize the LF clock.
 */
void lf_initialize_clock() {
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
int lf_clock_gettime(instant_t* t) {
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
            lf_print_error("lf_clock_gettime(): Failed to read the value of the physical clock.");
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

int lf_sleep_until_locked(instant_t wakeup_time) {
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
#endif
