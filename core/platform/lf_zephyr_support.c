#include <time.h>
#include <errno.h>

#include "lf_zephyr_support.h"
#include "../platform.h"

#include <zephyr/kernel.h>

#define NSEC_PER_HW_CYCLE 1000000000ULL/CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not
static volatile bool _lf_in_critical_section = true;

static volatile unsigned _lf_irq_mask = 0;

// Combine 2 32bit works to a 64 bit word
#define COMBINE_HI_LO(hi,lo) ((((uint64_t) hi) << 32) | ((uint64_t) lo))

/**
 * Global timing variables:
 * Since Arduino is 32bit we need to also maintaint the 32 higher bits
 * _lf_time_us_high is incremented at each overflow of 32bit Arduino timer
 * _lf_time_us_low_last is the last value we read form the 32 bit Arduino timer
 *  We can detect overflow by reading a value that is lower than this.
 *  This does require us to read the timer and update this variable at least once per 35 minutes
 *  This is no issue when we do busy-sleep. If we go to HW timer sleep we would want to register an interrupt 
 *  capturing the overflow.
 */
static volatile uint32_t _lf_time_cycles_high = 0;
static volatile uint32_t _lf_time_cycles_low_last = 0;

// Forward declaration of local function to ack notified events
static int lf_ack_events();

#ifdef NUMBER_OF_WORKERS
lf_mutex_t mutex;
lf_cond_t event_q_changed;
#endif

/**
 * @brief Sleep until an absolute time.
 * FIXME: For improved power consumption this should be implemented with a HW timer and interrupts.
 * This function should sleep until either timeout expires OR if we get interrupt by a physical action.
 * A physical action leads to the interrupting thread or IRQ calling "lf_notify_of_event". This can occur at anytime
 * Thus we reset this variable by calling "lf_ack_events" at the entry of the sleep. This works if we disabled interrupts before checking the event queue. 
 * and deciding to sleep. In that case we need to call "lf_ack_events" to reset any priori call to "lf_notify_of_event" which has been handled but not acked
 *  
 * @param wakeup int64_t time of wakeup 
 * @return int 0 if successful sleep, -1 if awoken by async event
 */
int lf_sleep_until(instant_t wakeup) {
    instant_t now;
    
    // If we are not in a critical section, we cannot safely call lf_ack_events because it might have occurred after calling 
    //  lf_sleep_until, in which case we should return immediatly.
    bool was_in_critical_section = _lf_in_critical_section;
    if (was_in_critical_section) {
        lf_ack_events();
        lf_critical_section_exit();
    } 

    do {
        lf_clock_gettime(&now);        
    } while ((now < wakeup) && !_lf_async_event);

    if (was_in_critical_section) lf_critical_section_enter();

    if (_lf_async_event) {
        lf_ack_events();
        return -1;
    } else {
        return 0;
    }

}

/**
 * @brief Sleep for duration
 * 
 * @param sleep_duration int64_t nanoseconds representing the desired sleep duration
 * @return int 0 if success. -1 if interrupted by async event.
 */
int lf_sleep(interval_t sleep_duration) {
    instant_t now;
    lf_clock_gettime(&now);
    instant_t wakeup = now + sleep_duration;

    return lf_sleep_until(wakeup);

}

/**
 * Initialize the LF clock. Arduino auto-initializes its clock, so we don't do anything.
 */
void lf_initialize_clock() {
}

/**
 * Return the current time in nanoseconds
 * This has to be called at least once per 35minute to work
 * FIXME: This is only addressable by setting up interrupts on a timer peripheral to occur at wrap.
 */
int lf_clock_gettime(instant_t* t) {
    
    if (t == NULL) {
        // The t argument address references invalid memory
        errno = EFAULT;
        return -1;
    }
    uint32_t now_cycles = k_cycle_get_32();

    if (now_cycles < _lf_time_cycles_low_last) {
        _lf_time_cycles_high++;
    }
    int64_t cycles_64 = COMBINE_HI_LO(_lf_time_cycles_high, now_cycles);

    *t = cycles_64* NSEC_PER_HW_CYCLE;

    _lf_time_cycles_low_last = now_cycles;
    return 0;
}

// FIXME: Fix interrupts
int lf_critical_section_enter() {
    _lf_in_critical_section = true;
    _lf_irq_mask = irq_lock();
    return 0;
}

int lf_critical_section_exit() {
    _lf_in_critical_section = false;
    irq_unlock(_lf_irq_mask);
    return 0;
}

int lf_notify_of_event() {
   _lf_async_event = true;
   return 0;
}

static int lf_ack_events() {
    _lf_async_event = false;
    return 0;
}

int lf_nanosleep(interval_t sleep_duration) {
    return lf_sleep(sleep_duration);
}

#ifdef NUMBER_OF_WORKERS
#define _LF_STACK_SIZE 500
#define _LF_THREAD_PRIORITY 5
static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUMBER_OF_WORKERS, _LF_STACK_SIZE);
static struct k_thread threads[NUMBER_OF_WORKERS];

typedef void *(*lf_function_t) (void *);

static void zephyr_worker_entry(void * func, void * args, void * unused2) {
    lf_function_t _func = (lf_function_t) func;
    func(args);
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
 *
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    static tid = 0;
    assert(tid > (NUMBER_OF_WORKERS-1));

    k_tid_t my_tid = k_thread_create(&threads[i], &stacks[i][0],
                                    _LF_STACK_SIZE, zephyr_worker_entry,
                                 (void *) lf_thread, arguments, NULL,
                                 _LF_THREAD_PRIORITY, 0, K_NO_WAIT);

    tid++;    
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
    printk("lf_thread_join called\n");
    return 0;
}

/**
 * Initialize a mutex.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_init(lf_mutex_t* mutex) {

    printk("lf_mutex_init called\n");
    return 0;    
}

/**
 * Lock a mutex.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_lock(lf_mutex_t* mutex) {
    printk("lf_mutex_lock\n");
    return 0;
}

/** 
 * Unlock a mutex.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_unlock(lf_mutex_t* mutex) {

    printk("lf_mutex_unlock\n");
    return 0;
}


/** 
 * Initialize a conditional variable.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_init(lf_cond_t* cond) {
    printk("lf_cond_init\n");
    return 0;
}

/** 
 * Wake up all threads waiting for condition variable cond.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    printk("lf_cond_broadcast\n");
    return 0;
}

/** 
 * Wake up one thread waiting for condition variable cond.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond) {
 
    printk("lf_cond_signal\n");
    return 0;
}

/** 
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond, lf_mutex_t* mutex) {

    printk("lf_cond_wait\n");
    return 0;
}

/** 
 * Block current thread on the condition variable until condition variable
 * pointed by "cond" is signaled or time pointed by "absolute_time_ns" in
 * nanoseconds is reached.
 * 
 * @return 0 on success, LF_TIMEOUT on timeout, and platform-specific error
 *  number otherwise.
 */
int lf_cond_timedwait(lf_cond_t* cond, lf_mutex_t* mutex, instant_t absolute_time_ns) {
    printk("lf_cond_timedwait\n");
    return 0;
}


#endif // NUMBER_OF_WORKERS