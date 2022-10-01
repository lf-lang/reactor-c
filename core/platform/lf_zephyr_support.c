#include <time.h>
#include <errno.h>

#include "lf_zephyr_support.h"
#include "../platform.h"

#include <zephyr/kernel.h>

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not
static volatile bool _lf_in_critical_section = true;

static volatile unsigned _lf_irq_mask = 0;
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
static volatile uint32_t _lf_time_us_high = 0;
static volatile uint32_t _lf_time_us_low_last = 0;

// Forward declaration of local function to ack notified events
static int lf_ack_events();
/**
 * @brief Sleep until an absolute time.
 * FIXME: For improved power consumption this should be implemented with a HW timer and interrupts.
 * 
 * @param wakeup int64_t time of wakeup 
 * @return int 0 if successful sleep, -1 if awoken by async event
 */
int lf_sleep_until(instant_t wakeup) {
    // FIXME: I think we can always assume that we are in critical section
    //  and we can thus always ack an incoming event.
    instant_t now;
    interval_t duration = wakeup - now;
    bool was_in_critical_section = _lf_in_critical_section;
    if (was_in_critical_section) lf_critical_section_exit();


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
    // FIXME: Get proper timing
    int64_t now = k_uptime_get() * 1000000;
    *t = now;
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