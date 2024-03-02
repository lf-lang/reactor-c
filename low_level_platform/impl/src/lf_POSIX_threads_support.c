#if !defined(LF_SINGLE_THREADED) && !defined(PLATFORM_ARDUINO)
#include "low_level_platform.h"
#include "platform/lf_POSIX_threads_support.h"
#include "platform/lf_unix_clock_support.h"

#include <pthread.h>
#include <errno.h>
#include <stdint.h> // For fixed-width integral types

int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    return pthread_create((pthread_t*)thread, NULL, lf_thread, arguments);
}

int lf_thread_join(lf_thread_t thread, void** thread_return) {
    return pthread_join((pthread_t)thread, thread_return);
}

int lf_mutex_init(lf_mutex_t* mutex) {
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

int lf_mutex_lock(lf_mutex_t* mutex) {
    return pthread_mutex_lock((pthread_mutex_t*)mutex);
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
    return pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    cond->mutex = mutex;
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    // Limit the scope of the condition variable to this process (default)
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_PRIVATE);
    return pthread_cond_init(&cond->condition, &cond_attr);
}

int lf_cond_broadcast(lf_cond_t* cond) {
    return pthread_cond_broadcast((pthread_cond_t*)&cond->condition);
}

int lf_cond_signal(lf_cond_t* cond) {
    return pthread_cond_signal((pthread_cond_t*)&cond->condition);
}

int lf_cond_wait(lf_cond_t* cond) {
    return pthread_cond_wait((pthread_cond_t*)&cond->condition, (pthread_mutex_t*)cond->mutex);
}

int _lf_cond_timedwait(lf_cond_t* cond, instant_t wakeup_time) {
    struct timespec timespec_absolute_time = convert_ns_to_timespec(wakeup_time);
    int return_value = pthread_cond_timedwait(
        (pthread_cond_t*)&cond->condition,
        (pthread_mutex_t*)cond->mutex,
        &timespec_absolute_time
    );
    switch (return_value) {
        case ETIMEDOUT:
            return_value = LF_TIMEOUT;
            break;

        default:
            break;
    }
    return return_value;
}
#endif
