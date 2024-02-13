#if !defined(LF_SINGLE_THREADED) && !defined(PLATFORM_ARDUINO)
#include "platform.h"
#include "util.h"
#include "lf_POSIX_threads_support.h"
#include "lf_unix_clock_support.h"

#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <stdint.h> // For fixed-width integral types
#include <unistd.h>

#if defined PLATFORM_Linux
#include <sys/syscall.h>
#endif

int lf_thread_set_scheduling_policy(lf_thread_t thread, lf_scheduling_policy_t *policy) {
    int posix_policy;
    struct sched_param schedparam;

    // Get the current scheduling policy
    if (pthread_getschedparam(thread, &posix_policy, &schedparam) != 0) {
        return -1;
    }

    // Update the policy
    switch(policy->policy) {
        case LF_SCHED_FAIR:
            posix_policy = SCHED_OTHER;
            break;
        case LF_SCHED_TIMESLICE:
            posix_policy = SCHED_RR;
            schedparam.sched_priority = ((lf_scheduling_policy_timeslice_t *) policy)->priority;
            break;
        case LF_SCHED_PRIORITY:
            posix_policy = SCHED_FIFO;
            schedparam.sched_priority = ((lf_scheduling_policy_priority_t *) policy)->priority;
            break;
        default:
            return -1;
            break;
    }

    // Write it back
    if (pthread_setschedparam(thread, posix_policy, &schedparam) != 0) {
        return -3;
    }

    return 0;
}

int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    static int core_id=0;
    pthread_create((pthread_t*)thread, NULL, lf_thread, arguments);
    if (lf_thread_set_cpu(*thread, core_id) != 0) {
        lf_print_error_and_exit("Could not set CPU");
    }
    core_id++;
    return 0;
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

int lf_thread_set_cpu(lf_thread_t thread, int cpu_number) {
    // First verify that we have num_cores>cpu_number
    if (lf_available_cores() <= cpu_number) {
        return -1;
    }

    // Create a CPU-set consisting of only the desired CPU
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_number, &cpu_set);

    return pthread_setaffinity_np(thread, sizeof(cpu_set), &cpu_set);
}


int lf_thread_set_priority(lf_thread_t thread, int priority) {
    return pthread_setschedprio(thread, priority);
}

lf_thread_t lf_thread_self() {
    return pthread_self();
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
