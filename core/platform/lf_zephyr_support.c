#include <time.h>
#include <errno.h>

#include "lf_zephyr_support.h"
#include "platform.h"
#include "utils/util.h"
#include "tag.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>

// Includes and functions for GPIO debugging
// Uncomment below to get gpio debug access
//#define GPIO_DEBUG
#ifdef GPIO_DEBUG
    #include <zephyr/drivers/gpio.h>
    #define GPIO0 DT_NODELABEL(gpio0)
    #define NUM_DEBUG_PINS 5
    gpio_pin_t debug_pins[NUM_DEBUG_PINS] = {19, 20, 22, 23, 24};
    const struct device *gpio_dev = DEVICE_DT_GET(GPIO0);

    static void gpio_debug_init() {
        for (int i = 0; i<NUM_DEBUG_PINS; i++) {
            gpio_pin_configure(gpio_dev, debug_pins[i], GPIO_OUTPUT_INACTIVE);
        }
    }

    void gpio_toggle(int pin) {
        gpio_pin_toggle(gpio_dev,debug_pins[pin]);
    }
#endif

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

// To enable development also on QEMU emulation we provide
//  the ability to use the less precise system clock. This flag is set

#ifndef LF_QEMU_EMULATION
// FIXME: timer1 is nrf52dk specific. For other boards we wanna support we have to do some
//  macro-lookups to get it working with all other boards
#define LF_TIMER DT_NODELABEL(timer1)
#define LF_TIMER_SLEEP_CHANNEL 0
#define LF_TIMER_ALARM_CHANNEL 0
#define LF_SLEEP_OVERHEAD_US 82
static struct counter_alarm_cfg _lf_alarm_cfg;
static struct counter_top_cfg _lf_timer_top_cfg;
const struct device *const _lf_counter_dev = DEVICE_DT_GET(LF_TIMER);   

// Timer overflow callback
static void  _lf_timer_overflow_callback(const struct device *dev, void *user_data) {
    _lf_time_cycles_high++;
}

// Global variable for storing the frequency of the clock. As well as macro for translating into nsec
static uint32_t _lf_timer_freq_hz;
static uint32_t _lf_timer_max_value_ticks;

static uint32_t _lf_ticks_to_nsec_divisor;
static uint32_t _lf_ticks_to_nsec_remainder;

// Convert ticks to nsec using precomputed divisor and remainder. 
//  If we translate ticks to nsec directly we lose a lot of precision unless we use floating point
static inline int64_t _lf_ticks_to_ns(uint64_t ticks) {
    int64_t res;
    res = ticks * _lf_ticks_to_nsec_divisor;
    res += (ticks * _lf_ticks_to_nsec_remainder)/_lf_timer_freq_hz;
    return res;
} 

#else
static inline int64_t _lf_ticks_to_ns(uint64_t ticks) {
    int64_t res;
    res = (SECOND(1)/CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)*ticks;
    return res;
}
#endif

// Forward declaration of local function to ack notified events
static int lf_ack_events();

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not
static volatile bool _lf_in_critical_section = false;
// Keep track of IRQ mask when entering critical section so we can enable again after
static volatile unsigned _lf_irq_mask = 0;

static volatile bool _lf_alarm_fired;

static void _lf_wakeup_alarm(const struct device *counter_dev,
				      uint8_t chan_id, uint32_t ticks,
				      void *user_data)
{
    _lf_alarm_fired=true;
}
#ifdef NUMBER_OF_WORKERS
lf_mutex_t mutex;
lf_cond_t event_q_changed;
#endif

/**
 * Initialize the LF clock
 */
void lf_initialize_clock() {
    #ifndef LF_QEMU_EMULATION
    LF_PRINT_LOG("Initializing zephyr HW timer");
	
    #ifdef GPIO_DEBUG
    gpio_debug_init();
    #endif

    // Verify that we have the device
    // FIXME: proper error handling here
    if (!device_is_ready(_lf_counter_dev)) {
		printk("ERROR: counter device not ready.\n");
        while(1) {};
    }

    // Verify that it is working as we think
    if(!counter_is_counting_up(_lf_counter_dev)) {
        printk("ERROR: Timer is counting down \n");
        while(1) {};
    }

    _lf_timer_freq_hz= counter_get_frequency(_lf_counter_dev);
    if (_lf_timer_freq_hz == 0) {
        printk("ERROR: Timer has 0Hz frequency\n");
        while(1) {};
    }
    _lf_ticks_to_nsec_divisor = SECOND(1) /_lf_timer_freq_hz;
    _lf_ticks_to_nsec_remainder = SECOND(1) % _lf_timer_freq_hz;
    
    _lf_timer_max_value_ticks = counter_get_max_top_value(_lf_counter_dev);
    _lf_timer_top_cfg.ticks = _lf_timer_max_value_ticks;
    _lf_timer_top_cfg.callback = _lf_timer_overflow_callback;
    
    _lf_alarm_cfg.flags = 0;
    _lf_alarm_cfg.ticks = 0;
    _lf_alarm_cfg.callback = _lf_wakeup_alarm;
    _lf_alarm_cfg.user_data = &_lf_alarm_cfg;

    int res = counter_set_top_value(_lf_counter_dev, &_lf_timer_top_cfg);
    if (res != 0) {
        printk("ERROR: Timer couldnt set top value\n");
        while(1) {};
    }

    printk("HW Clock has frequency of %u Hz and wraps every %u sec\n", _lf_timer_freq_hz, _lf_timer_max_value_ticks/_lf_timer_freq_hz);

    // Start counter
    counter_start(_lf_counter_dev);
    #else
    printk("Sys Clock has frequency of %u Hz\n", CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
    #endif
}   

/**
 * Return the current time in nanoseconds
 */
int lf_clock_gettime(instant_t* t) {
    
    if (t == NULL) {
        // The t argument address references invalid memory
        errno = EFAULT;
        return -1;
    }
    // Read value from HW counter
    #ifndef LF_QEMU_EMULATION
    uint32_t now_cycles;
    int res = counter_get_value(_lf_counter_dev, &now_cycles);

    if (res != 0) {
        return res;
    }
    #else
    uint32_t now_cycles = k_cycle_get_32();
    #endif

    uint64_t cycles_64 = COMBINE_HI_LO(_lf_time_cycles_high, now_cycles);
    *t = _lf_ticks_to_ns(cycles_64);

    return 0;
}

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
#ifndef LF_QEMU_EMULATION
int lf_sleep_until(instant_t wakeup) {
    // If we are not in a critical section, we cannot safely call lf_ack_events because it might have occurred after calling 
    //  lf_sleep_until, in which case we should return immediatly.
    bool was_in_critical_section = _lf_in_critical_section;
    if (was_in_critical_section) {
        lf_ack_events();
        lf_critical_section_exit();
    }

    _lf_alarm_fired = false;

    uint32_t now_cycles;
    int res = counter_get_value(_lf_counter_dev, &now_cycles);

    instant_t now;
    lf_clock_gettime(&now);
    
    interval_t sleep_for_us = (wakeup - now)/1000 - LF_SLEEP_OVERHEAD_US; 
    uint32_t sleep_duration_ticks = counter_us_to_ticks(_lf_counter_dev, sleep_for_us);

    _lf_alarm_cfg.ticks = sleep_duration_ticks;
	int err = counter_set_channel_alarm(_lf_counter_dev, LF_TIMER_ALARM_CHANNEL,  &_lf_alarm_cfg);
    if (err != 0) {
        lf_print_error_and_exit("Could not setup alarm");
    }

    while (!_lf_alarm_fired && !_lf_async_event) {}
    // gpio_toggle(1);

    if (was_in_critical_section) lf_critical_section_enter();

    if (_lf_async_event) {
        lf_ack_events();
        return -1;
    } else {
        return 0;
    }
}
#else
int lf_sleep_until(instant_t wakeup) {
    // If we are not in a critical section, we cannot safely call lf_ack_events because it might have occurred after calling 
    //  lf_sleep_until, in which case we should return immediatly.
    bool was_in_critical_section = _lf_in_critical_section;
    if (was_in_critical_section) {
        lf_ack_events();
        lf_critical_section_exit();
    }

    _lf_alarm_fired = false;
    do {
    instant_t now;
    lf_clock_gettime(&now);
    } while (!_lf_alarm_fired && !_lf_async_event);

    if (was_in_critical_section) lf_critical_section_enter();

    if (_lf_async_event) {
        lf_ack_events();
        return -1;
    } else {
        return 0;
    }
}
#endif

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
// FIXME: What is an appropriate stack size?
#define _LF_STACK_SIZE 1024
// FIXME: What is an appropriate thread prio?
#define _LF_THREAD_PRIORITY 5
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
 *
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    // Use static id to map each created thread to a 
    // FIXME: ARe we guaranteed to never have more threads? What about tracing?
    static int tid = 0;
    
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
    k_thread_join(thread, K_FOREVER);
    return 0;
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
int lf_cond_init(lf_cond_t* cond) {
    return k_condvar_init(cond);
}

/** 
 * Wake up all threads waiting for condition variable cond.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    k_condvar_broadcast(cond);
    return 0;
}

/** 
 * Wake up one thread waiting for condition variable cond.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond) {
    return k_condvar_signal(cond);
}

/** 
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond, lf_mutex_t* mutex) {
    return k_condvar_wait(cond, mutex, K_FOREVER);
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
    
    // lf_sleep_until(absolute_time_ns);
    // instant_t now;
    // lf_clock_gettime(&now);
    // return LF_TIMEOUT;
    instant_t now;
    lf_clock_gettime(&now);
    interval_t sleep_duration_ns = absolute_time_ns - now;
    k_timeout_t timeout = K_NSEC(sleep_duration_ns);
    int res = k_condvar_wait(cond, mutex, timeout);
    if (res == 0) {
        return 0;
    } else {
        return LF_TIMEOUT;
    }
}

// Atomics
//  Implemented by just entering a critical section and doing the arithmetic
//  FIXME: We are now restricted to atomic integer operations
int _zephyr_atomic_fetch_add(int *ptr, int value) {
    lf_critical_section_enter();
    int res = *ptr;
    *ptr += value;
    lf_critical_section_exit();
    return res;
}

int _zephyr_atomic_add_fetch(int *ptr, int value) {
    lf_critical_section_enter();
    int res = *ptr + value;
    *ptr = res;
    lf_critical_section_exit();
    return res;
}

// FIXME: Are you sure that it should return bool?
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

// FIXME: Are you sure that it should return bool?
bool _zephyr_val_compare_and_swap(int *ptr, int value, int newval) {
    lf_critical_section_enter();
    bool res = false;
    if (*ptr == value) {
        *ptr = newval;
        res = true;
    }
    lf_critical_section_exit();
    return res;
}

#endif // NUMBER_OF_WORKERS
