/* POSIX API support for the C target of Lingua Franca. */

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

/**
 * POSIX API support for the C target of Lingua Franca.
 *
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 *
 * All functions return 0 on success.
 */

#ifndef LF_POSIX_THREADS_SUPPORT_H
#define LF_POSIX_THREADS_SUPPORT_H

#include <pthread.h>
#include <errno.h>
#include <stdint.h> // For fixed-width integral types

typedef pthread_mutex_t lf_mutex_t;
typedef struct {
    lf_mutex_t* mutex;
    pthread_cond_t condition;
} lf_cond_t;
typedef pthread_t lf_thread_t;

#define _LF_TIMEOUT ETIMEDOUT

/**
 * Create a new thread, starting with execution of lf_thread getting passed
 * arguments. The new handle is stored in thread.
 *
 * @return 0 on success, error number otherwise (see pthread_create()).
 */
static int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    return pthread_create((pthread_t*)thread, NULL, lf_thread, arguments);
}

/**
 * Make calling thread wait for termination of the thread.  The
 * exit status of the thread is stored in thread_return, if thread_return
 * is not NULL.
 *
 * @return 0 on success, error number otherwise (see pthread_join()).
 */
static int lf_thread_join(lf_thread_t thread, void** thread_return) {
    return pthread_join((pthread_t)thread, thread_return);
}

/**
 * Initialize a mutex.
 *
 * @return 0 on success, error number otherwise (see pthread_mutex_init()).
 */
static int lf_mutex_init(lf_mutex_t* mutex) {
    // Set up a recursive mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // Initialize the mutex to be recursive, meaning that it is OK
    // for the same thread to lock and unlock the mutex even if it already holds
    // the lock.
    // FIXME: This is dangerous. The docs say this: "It is advised that an
    // application should not use a PTHREAD_MUTEX_RECURSIVE mutex with
    // condition variables because the implicit unlock performed for a
    // pthread_cond_wait() or pthread_cond_timedwait() may not actually
    // release the mutex (if it had been locked multiple times).
    // If this happens, no other thread can satisfy the condition
    // of the predicate.”  This seems like a bug in the implementation of
    // pthreads. Maybe it has been fixed?
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    return pthread_mutex_init((pthread_mutex_t*)mutex, &attr);
}

/**
 * Lock a mutex.
 *
 * @return 0 on success, error number otherwise (see pthread_mutex_lock()).
 */
static int lf_mutex_lock(lf_mutex_t* mutex) {
    return pthread_mutex_lock((pthread_mutex_t*)mutex);
}

/**
 * Unlock a mutex.
 *
 * @return 0 on success, error number otherwise (see pthread_mutex_unlock()).
 */
static int lf_mutex_unlock(lf_mutex_t* mutex) {
    return pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

/**
 * Initialize a conditional variable.
 *
 * @return 0 on success, error number otherwise (see pthread_cond_init()).
 */
static int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    cond->mutex = mutex;
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    // Limit the scope of the condition variable to this process (default)
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_PRIVATE);
    return pthread_cond_init(&cond->condition, &cond_attr);
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, error number otherwise (see pthread_cond_broadcast()).
 */
static int lf_cond_broadcast(lf_cond_t* cond) {
    return pthread_cond_broadcast((pthread_cond_t*)&cond->condition);
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, error number otherwise (see pthread_cond_signal()).
 */
static int lf_cond_signal(lf_cond_t* cond) {
    return pthread_cond_signal((pthread_cond_t*)&cond->condition);
}

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, error number otherwise (see pthread_cond_wait()).
 */
static int lf_cond_wait(lf_cond_t* cond) {
    return pthread_cond_wait((pthread_cond_t*)&cond->condition, (pthread_mutex_t*)cond->mutex);
}

/**
 * Block current thread on the condition variable until condition variable
 * pointed by "cond" is signaled or time pointed by "absolute_time_ns" in
 * nanoseconds is reached.
 *
 * @return 0 on success, LF_TIMEOUT on timeout, and platform-specific error
 *  number otherwise (see pthread_cond_timedwait).
 */
static int lf_cond_timedwait(lf_cond_t* cond, int64_t absolute_time_ns) {
    // Convert the absolute time to a timespec.
    // timespec is seconds and nanoseconds.
    struct timespec timespec_absolute_time
            = {(time_t)absolute_time_ns / 1000000000LL, (long)absolute_time_ns % 1000000000LL};
    int return_value = 0;
    return_value = pthread_cond_timedwait(
        (pthread_cond_t*)&cond->condition,
        (pthread_mutex_t*)cond->mutex,
        &timespec_absolute_time
    );
    switch (return_value) {
        case ETIMEDOUT:
            return_value = _LF_TIMEOUT;
            break;

        default:
            break;
    }
    return return_value;
}
#endif
