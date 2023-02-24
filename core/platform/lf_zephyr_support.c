#ifdef PLATFORM_ZEPHYR
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
#include "utils/util.h"
#include "tag.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>

// Combine 2 32-bit words to a 64-bit word
#define COMBINE_HI_LO(hi,lo) ((((uint64_t) hi) << 32) | ((uint64_t) lo))

// Keep track of overflows to keep clocks monotonic
static int64_t _lf_timer_epoch_duration_nsec;
static int64_t _lf_timer_epoch_duration_usec;
static volatile int64_t _lf_timer_last_epoch_nsec = 0;

#if defined(LF_ZEPHYR_CLOCK_HI_RES)
// Create semaphore for async wakeup from physical action
K_SEM_DEFINE(_lf_sem,0,1)

static struct counter_alarm_cfg _lf_alarm_cfg;
const struct device *const _lf_counter_dev = DEVICE_DT_GET(LF_TIMER);   
static volatile bool _lf_alarm_fired;
static uint32_t _lf_timer_freq;



// Timer overflow callback
static void  _lf_timer_overflow_callback(const struct device *dev, void *user_data) {
        _lf_timer_last_epoch_nsec += _lf_timer_epoch_duration_nsec;
}


static void _lf_wakeup_alarm(const struct device *counter_dev,
				      uint8_t chan_id, uint32_t ticks,
				      void *user_data)
{
    _lf_alarm_fired=true;
    k_sem_give(&_lf_sem);
}

#endif

// Keep track of nested critical sections
static uint32_t _lf_num_nested_critical_sections=0;
// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of IRQ mask when entering critical section so we can enable again after
static volatile unsigned _lf_irq_mask = 0;


/**
 * Initialize the LF clock
 */
void lf_initialize_clock() {

    #if defined(LF_ZEPHYR_CLOCK_HI_RES)
    struct counter_top_cfg counter_top_cfg;
    uint32_t counter_max_ticks=0;
    int res;
    LF_PRINT_LOG("Using High resolution Zephyr Counter. Initializing  HW timer");
	
    // Verify that we have the device
    // FIXME: Try lf_print_error_and_exit? Or terminate in some way? Maybe return non-zero from this function
    if (!device_is_ready(_lf_counter_dev)) {

		lf_print_error_and_exit("ERROR: counter device not ready.\n");
    }

    // Verify that it is working as we think
    if(!counter_is_counting_up(_lf_counter_dev)) {
        lf_print_error_and_exit("ERROR: Timer is counting down \n");
        while(1) {};
    }

    // Calculate the duration of an epoch. Compute both
    //  nsec and usec now at boot to avoid these computations later
    counter_max_ticks = counter_get_max_top_value(_lf_counter_dev);
    _lf_timer_epoch_duration_usec = counter_ticks_to_us(_lf_counter_dev, counter_max_ticks);
    _lf_timer_epoch_duration_nsec = _lf_timer_epoch_duration_usec * 1000LL;
    
    // Set the max_top value to be the maximum
    counter_max_ticks = counter_get_max_top_value(_lf_counter_dev);
    counter_top_cfg.ticks = counter_max_ticks;
    counter_top_cfg.callback = _lf_timer_overflow_callback;
    res = counter_set_top_value(_lf_counter_dev, &counter_top_cfg);
    if (res != 0) {
        lf_print_error_and_exit("ERROR: Timer couldnt set top value\n");
        while(1) {};
    }

    LF_PRINT_LOG("HW Clock has frequency of %u Hz and wraps every %u sec\n", _lf_timer_freq, counter_max_ticks/_lf_timer_freq);
    
    
    // Prepare the alarm config
    _lf_alarm_cfg.flags = 0;
    _lf_alarm_cfg.ticks = 0;
    _lf_alarm_cfg.callback = _lf_wakeup_alarm;
    _lf_alarm_cfg.user_data = &_lf_alarm_cfg;

    // Start counter
    counter_start(_lf_counter_dev);
    #else
    LF_PRINT_LOG("Using Low resolution zephyr kernel clock");
    LF_PRINT_LOG("Kernel Clock has frequency of %u Hz\n", CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
    _lf_timer_last_epoch_nsec = 0;
    // Compute the duration of an epoch. Compute both
    //  nsec and usec now at boot to avoid these computations later
    _lf_timer_epoch_duration_nsec = ((1LL << 32) * SECONDS(1))/CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
    _lf_timer_epoch_duration_usec = _lf_timer_epoch_duration_nsec/1000;
    #endif
}   


#if defined(LF_ZEPHYR_CLOCK_HI_RES)
// Clock and sleep implementation for the HI_RES clock based on 
// Zephyrs Counter API

/**
 * Return the current time in nanoseconds. It gets the current value
 * of the hi-res counter device and also keeps track of overflows
 * to deliver a monotonically increasing clock.
 */
int lf_clock_gettime(instant_t* t) {
    uint32_t now_cycles;
    int res;
    uint64_t now_nsec;
    
    res = counter_get_value(_lf_counter_dev, &now_cycles);
    now_nsec = counter_ticks_to_us(_lf_counter_dev, now_cycles)*1000ULL;
    *t = now_nsec + _lf_timer_last_epoch_nsec;
    return 0;
}

/**
 * @brief Sleep until an absolute time.
 *  
 * @param wakeup int64_t time of wakeup 
 * @return int 0 if successful sleep, -1 if awoken by async event
 */
int lf_sleep_until_locked(instant_t wakeup) {
    // Reset flags
    _lf_alarm_fired = false;
    _lf_async_event = false;
    k_sem_reset(&_lf_sem);

    // Calculate the sleep duration
    uint32_t now_cycles, sleep_duration_ticks;
    counter_get_value(_lf_counter_dev, &now_cycles);
    instant_t now;
    lf_clock_gettime(&now);
    interval_t sleep_for_us = (wakeup - now)/1000;

    
    while ( !_lf_async_event && 
            sleep_for_us > (LF_WAKEUP_OVERHEAD_US + LF_MIN_SLEEP_US)
    ) {  
        if (sleep_for_us < _lf_timer_epoch_duration_usec) {
            sleep_duration_ticks = counter_us_to_ticks(_lf_counter_dev, ((uint64_t) sleep_for_us) - LF_WAKEUP_OVERHEAD_US);
        } else {
            sleep_duration_ticks = UINT32_MAX;
        }

        _lf_alarm_cfg.ticks = sleep_duration_ticks;
        int err = counter_set_channel_alarm(_lf_counter_dev, LF_TIMER_ALARM_CHANNEL,  &_lf_alarm_cfg);
     
        if (err != 0) {
            lf_print_error_and_exit("Could not setup alarm for sleeping. Errno %i", err);
        }
        
        lf_critical_section_exit();
        k_sem_take(&_lf_sem, K_FOREVER);
        lf_critical_section_enter();

        // Then calculating remaining sleep, unless we got woken up by an event
        if (!_lf_async_event) {
            lf_clock_gettime(&now);
            sleep_for_us = (wakeup - now)/1000;
        }
    } 
    
    // Do remaining sleep in busy_wait
    if (!_lf_async_event &&
        sleep_for_us > LF_RUNTIME_OVERHEAD_US) {        
        k_busy_wait((uint32_t) (sleep_for_us - LF_RUNTIME_OVERHEAD_US));
    }

    if (_lf_async_event) {
        // Cancel the outstanding alarm
        counter_cancel_channel_alarm(_lf_counter_dev, LF_TIMER_ALARM_CHANNEL);
        _lf_async_event = false;
        return -1;
    } else {
        return 0;
    }
}
#else
// Clock and sleep implementation for LO_RES clock. Handle wraps
//  by checking if two consecutive reads are monotonic
static uint32_t last_read_cycles=0;
int lf_clock_gettime(instant_t* t) {
    uint32_t now_cycles = k_cycle_get_32();

    if (now_cycles < last_read_cycles) {
        _lf_timer_last_epoch_nsec += _lf_timer_epoch_duration_nsec;
    }

    *t = (SECOND(1)/CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)*now_cycles + _lf_timer_last_epoch_nsec;

    last_read_cycles = now_cycles;
    return 0;
}

int lf_sleep_until_locked(instant_t wakeup) {
    _lf_async_event=false;    
    lf_critical_section_exit();

    instant_t now;
    do {
    lf_clock_gettime(&now);
    } while ( (now<wakeup) && !_lf_async_event);
    
    lf_critical_section_enter();

    if (_lf_async_event) {
        _lf_async_event=false;
        return -1;
    } else {
        return 0;
    }
}
#endif

/**
 * @brief Pause execution for a given duration.
 * 
 * This implementation performs a busy-wait because it is unclear what will
 * happen if this function is called from within an ISR.
 * 
 * @param sleep_duration 
 * @return 0 for success, or -1 for failure.
 */
int lf_sleep(interval_t sleep_duration) {
    instant_t target_time;
    instant_t current_time;
    lf_clock_gettime(&current_time);
    target_time = current_time + sleep_duration;
    while (current_time <= target_time) {
        lf_clock_gettime(&current_time);
    }
    return 0;
}

/**
 * Old nanosleep API. It is just forwarded to `lf_sleep`
*/
int lf_nanosleep(interval_t sleep_duration) {
    return lf_sleep(sleep_duration);
}



/**
 * Only provide implementation of critical sections and notify of event
 * for the unthreaded scenario. For threaded, it is implemented in
 * `reactor_threaded.c`
*/
#ifdef LF_UNTHREADED
/**
 * @brief Enter critical section by disabling interrupts.
 * Support nested critical sections by only disabling
 * interrupts on the first call 
 * 
 * @return int 
 */
int lf_critical_section_enter() {
    if (_lf_num_nested_critical_sections++ == 0) {
        _lf_irq_mask = irq_lock();
    }
    return 0;
}

/**
 * @brief Leave critical section by re-enabling interrupts
 * 
 * @return int 
 */
int lf_critical_section_exit() {
    if (_lf_num_nested_critical_sections <= 0) {
        return 1;
    }
    
    if (--_lf_num_nested_critical_sections == 0) {
        irq_unlock(_lf_irq_mask);
    }
    return 0;
}

int lf_notify_of_event() {
   _lf_async_event = true;
   // If we are using the HI_RES clock. Then we interrupt a sleep through
   // a semaphore. The LO_RES clock does a busy wait and is woken up by
   // flipping the `_lf_async_event` flag  
   #if defined(LF_ZEPHYR_CLOCK_HI_RES)
    k_sem_give(&_lf_sem);
   #endif
   return 0;
}

#endif



#ifdef LF_THREADED
#error "Threaded support on Zephyr is not supported"

// FIXME: What is an appropriate stack size?
#define _LF_STACK_SIZE 1024
// FIXME: What is an appropriate thread prio?
#define _LF_THREAD_PRIORITY 5

// If NUMBER_OF_WORKERS is not specified, or set to 0, then we default to 1.
#if !defined(NUMBER_OF_WORKERS) || NUMBER_OF_WORKERS==0
#undef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif

static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUMBER_OF_WORKERS, _LF_STACK_SIZE);
static struct k_thread threads[NUMBER_OF_WORKERS];

// Typedef that represents the function pointers passed by LF runtime into lf_thread_create
typedef void *(*lf_function_t) (void *);

// Entry point for all worker threads. an intermediate step to connect Zephyr threads with LF runtimes idea of a thread
static void zephyr_worker_entry(void * func, void * args, void * unused2) {
    lf_function_t _func = (lf_function_t) func;
    _func(args);
}

/**
 * @brief Get the number of cores on the host machine.
 * FIXME: Use proper Zephyr API
 */
int lf_available_cores() {
    return 1;
}

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in thread_id.
 *
 * @return 0 on success, platform-specific error number otherwise.
 * FIXME: As this function is currently part of the user-facing API, 
 *  it should not care about the number of workers specified.
 *  If we want static allocation of workers, as implemented now,
 *  it must be removed from the API.
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    // Use static id to map each created thread to a 
    static int tid = 0;

    // Make sure we dont try to create too many threads
    if (tid > (NUMBER_OF_WORKERS-1)) {
        return -1;
    }


    k_tid_t my_tid = k_thread_create(&threads[tid], &stacks[tid][0],
                                    _LF_STACK_SIZE, zephyr_worker_entry,
                                 (void *) lf_thread, arguments, NULL,
                                 _LF_THREAD_PRIORITY, 0, K_NO_WAIT);

    tid++; 

    *thread = my_tid;   
    return 0;
}

/**
 * Make calling thread wait for termination of the thread.  The
 * exit status of the thread is stored in thread_return, if thread_return
 * is not NULL.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_thread_join(lf_thread_t thread, void** thread_return) {
    return k_thread_join(thread, K_FOREVER);
}

/**
 * Initialize a mutex.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_init(lf_mutex_t* mutex) {
    return k_mutex_init(mutex);    
}

/**
 * Lock a mutex.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_lock(lf_mutex_t* mutex) {
    int res = k_mutex_lock(mutex, K_FOREVER);
    return res;
}

/** 
 * Unlock a mutex.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_unlock(lf_mutex_t* mutex) {
    int res = k_mutex_unlock(mutex);
    return res;
}


/** 
 * Initialize a conditional variable.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    cond->mutex = mutex;
    return k_condvar_init(&cond->condition);
}

/** 
 * Wake up all threads waiting for condition variable cond.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    k_condvar_broadcast(&cond->condition);
    return 0;
}

/** 
 * Wake up one thread waiting for condition variable cond.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond) {
    return k_condvar_signal(&cond->condition);
}

/** 
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond) {
    return k_condvar_wait(&cond->condition, cond->mutex, K_FOREVER);
}

/** 
 * Block current thread on the condition variable until condition variable
 * pointed by "cond" is signaled or time pointed by "absolute_time_ns" in
 * nanoseconds is reached.
 * 
 * @return 0 on success, LF_TIMEOUT on timeout, and platform-specific error
 *  number otherwise.
 */
int lf_cond_timedwait(lf_cond_t* cond, instant_t absolute_time_ns) {
    instant_t now;
    lf_clock_gettime(&now);
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

/**
 * @brief Add `value` to `*ptr` and return original value of `*ptr` 
 * 
 */
int _zephyr_atomic_fetch_add(int *ptr, int value) {
    lf_critical_section_enter();
    int res = *ptr;
    *ptr += value;
    lf_critical_section_exit();
    return res;
}
/**
 * @brief Add `value` to `*ptr` and return new updated value of `*ptr`
 */
int _zephyr_atomic_add_fetch(int *ptr, int value) {
    lf_critical_section_enter();
    int res = *ptr + value;
    *ptr = res;
    lf_critical_section_exit();
    return res;
}

/**
 * @brief Compare and swap for boolaen value.
 * If `*ptr` is equal to `value` then overwrite it 
 * with `newval`. If not do nothing. Retruns true on overwrite.
 */
bool _zephyr_bool_compare_and_swap(bool *ptr, bool value, bool newval) {
    lf_critical_section_enter();
    bool res = false;
    if (*ptr == value) {
        *ptr = newval;
        res = true;
    }
    lf_critical_section_exit();
    return res;
}

/**
 * @brief Compare and swap for integers. If `*ptr` is equal
 * to `value`, it is updated to `newval`. The function returns
 * the original value of `*ptr`.
 */
int  _zephyr_val_compare_and_swap(int *ptr, int value, int newval) {
    lf_critical_section_enter();
    int res = *ptr;
    if (*ptr == value) {
        *ptr = newval;
    }
    lf_critical_section_exit();
    return res;
}

#endif // NUMBER_OF_WORKERS
#endif
