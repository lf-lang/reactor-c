#if defined(PLATFORM_ZEPHYR)
/*************
Copyright (c) 2023, Norwegian University of Science and Technology.

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
 * @brief Zephyr support for the C target of Lingua Franca.
 *
 * @author{Erling Jellum <erling.r.jellum@ntnu.no>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#include <time.h>
#include <errno.h>

#include "lf_zephyr_support.h"
#include "lf_zephyr_board_support.h"
#include "platform.h"
#include "reactor_common.h"
#include "utils/util.h"
#include "tag.h"

#include <zephyr/kernel.h>

// Keep track of nested critical sections
static uint32_t num_nested_critical_sections=0;
// Keep track of IRQ mask when entering critical section so we can enable again after
static volatile unsigned irq_mask = 0;

int lf_sleep(interval_t sleep_duration) {
    k_sleep(K_NSEC(sleep_duration));
    return 0;
}

int lf_nanosleep(interval_t sleep_duration) {
    return lf_sleep(sleep_duration);
}

int lf_disable_interrupts_nested() {
    if (num_nested_critical_sections++ == 0) {
        irq_mask = irq_lock();
    }
    return 0;
}

int lf_enable_interrupts_nested() {
    if (num_nested_critical_sections <= 0) {
        return 1;
    }
    
    if (--num_nested_critical_sections == 0) {
        irq_unlock(irq_mask);
    }
    return 0;
}

#if !defined(LF_SINGLE_THREADED)
#if !defined(LF_ZEPHYR_STACK_SIZE)
    #define LF_ZEPHYR_STACK_SIZE LF_ZEPHYR_STACK_SIZE_DEFAULT
#endif

#if !defined(LF_ZEPHYR_THREAD_PRIORITY)
    #define LF_ZEPHYR_THREAD_PRIORITY LF_ZEPHYR_THREAD_PRIORITY_DEFAULT
#endif

// If NUMBER_OF_WORKERS is not specified, or set to 0, then we default to 1.
#if !defined(NUMBER_OF_WORKERS) || NUMBER_OF_WORKERS==0
#undef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif

// If USER_THREADS is not specified, then default to 0.
#if !defined(USER_THREADS)
#define USER_THREADS 0
#endif

#define NUMBER_OF_THREADS (NUMBER_OF_WORKERS + USER_THREADS)

K_MUTEX_DEFINE(thread_mutex);

static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUMBER_OF_THREADS, LF_ZEPHYR_STACK_SIZE);
static struct k_thread threads[NUMBER_OF_THREADS];

// Typedef that represents the function pointers passed by LF runtime into lf_thread_create
typedef void *(*lf_function_t) (void *);

// Entry point for all worker threads. an intermediate step to connect Zephyr threads with LF runtimes idea of a thread
static void zephyr_worker_entry(void * func, void * args, void * unused2) {
    lf_function_t _func = (lf_function_t) func;
    _func(args);
}

int lf_available_cores() {
    #if defined(CONFIG_MP_NUM_CPUS)
        return CONFIG_MP_NUM_CPUS;
    #else
        return 1;
    #endif
}

int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    k_mutex_lock(&thread_mutex, K_FOREVER);

    // Use static id to map each created thread to a 
    static int tid = 0;

    // Make sure we dont try to create too many threads
    if (tid > (NUMBER_OF_THREADS-1)) {
        return -1;
    }

    k_tid_t my_tid = k_thread_create(&threads[tid], &stacks[tid][0],
                                    LF_ZEPHYR_STACK_SIZE, zephyr_worker_entry,
                                 (void *) lf_thread, arguments, NULL,
                                 LF_ZEPHYR_THREAD_PRIORITY, 0, K_NO_WAIT);


    // Pass the pointer to the k_thread struct out. This is needed
    // to join on the thread later.
    *thread = &threads[tid];   

    // Increment the tid counter so that next call to `lf_thread_create`
    // uses the next available k_thread struct and stack.
    tid++; 
    k_mutex_unlock(&thread_mutex);

    return 0;
}

int lf_thread_join(lf_thread_t thread, void** thread_return) {
    return k_thread_join(thread, K_FOREVER);
}

int lf_mutex_init(lf_mutex_t* mutex) {
    return k_mutex_init(mutex);    
}

int lf_mutex_lock(lf_mutex_t* mutex) {
    int res = k_mutex_lock(mutex, K_FOREVER);
    return res;
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
    int res = k_mutex_unlock(mutex);
    return res;
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    cond->mutex = mutex;
    return k_condvar_init(&cond->condition);
}

int lf_cond_broadcast(lf_cond_t* cond) {
    k_condvar_broadcast(&cond->condition);
    return 0;
}

int lf_cond_signal(lf_cond_t* cond) {
    return k_condvar_signal(&cond->condition);
}

int lf_cond_wait(lf_cond_t* cond) {
    return k_condvar_wait(&cond->condition, cond->mutex, K_FOREVER);
}

int lf_cond_timedwait(lf_cond_t* cond, instant_t absolute_time_ns) {
    instant_t now;
    _lf_clock_now(&now);
    interval_t sleep_duration_ns = absolute_time_ns - now;
    k_timeout_t timeout = K_NSEC(sleep_duration_ns);
    int res = k_condvar_wait(&cond->condition, cond->mutex, timeout);
    if (res == 0) {
        return 0;
    } else {
        return LF_TIMEOUT;
    }
}

// Atomics
//  Implemented by just entering a critical section and doing the arithmetic.
//  This is somewhat inefficient considering enclaves. Since we get a critical
//  section inbetween different enclaves

/**
 * @brief Add `value` to `*ptr` and return original value of `*ptr` 
 * 
 */
int _zephyr_atomic_fetch_add(int *ptr, int value) {
    lf_disable_interrupts_nested();
    int res = *ptr;
    *ptr += value;
    lf_enable_interrupts_nested();
    return res;
}
/**
 * @brief Add `value` to `*ptr` and return new updated value of `*ptr`
 */
int _zephyr_atomic_add_fetch(int *ptr, int value) {
    lf_disable_interrupts_nested();
    int res = *ptr + value;
    *ptr = res;
    lf_enable_interrupts_nested();
    return res;
}

/**
 * @brief Compare and swap for boolaen value.
 * If `*ptr` is equal to `value` then overwrite it 
 * with `newval`. If not do nothing. Retruns true on overwrite.
 */
bool _zephyr_bool_compare_and_swap(bool *ptr, bool value, bool newval) {
    lf_disable_interrupts_nested();
    bool res = false;
    if (*ptr == value) {
        *ptr = newval;
        res = true;
    }
    lf_enable_interrupts_nested();
    return res;
}

/**
 * @brief Compare and swap for integers. If `*ptr` is equal
 * to `value`, it is updated to `newval`. The function returns
 * the original value of `*ptr`.
 */
int  _zephyr_val_compare_and_swap(int *ptr, int value, int newval) {
    lf_disable_interrupts_nested();
    int res = *ptr;
    if (*ptr == value) {
        *ptr = newval;
    }
    lf_enable_interrupts_nested();
    return res;
}

#endif // NUMBER_OF_WORKERS
#endif
