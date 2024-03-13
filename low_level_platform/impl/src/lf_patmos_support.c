#include <time.h>
#include <errno.h>
#include <assert.h>
#include <lf_patmos_support.h>
#include "../platform.h"
#include <machine/rtc.h>
#include <machine/exceptions.h>
#include <stdio.h>

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not
static volatile int _lf_num_nested_critical_sections = 0;
/**
 * @brief Sleep until an absolute time.
 * TODO: For improved power consumption this should be implemented with a HW timer and interrupts.
 *
 * @param wakeup int64_t time of wakeup
 * @return int 0 if successful sleep, -1 if awoken by async event
 */

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup) {
    instant_t now;
    _lf_async_event = false;
    lf_enable_interrupts_nested();

    // Do busy sleep
    do {
        _lf_clock_gettime(&now);
    } while ((now < wakeup) && !_lf_async_event);

    lf_disable_interrupts_nested();

    if (_lf_async_event) {
        _lf_async_event = false;
        return -1;
    } else {
        return 0;
    }
}

/**
 * Patmos clock does not need initialization.
 */
void _lf_initialize_clock() {

}

/**
 * Write the current time in nanoseconds into the location given by the argument.
 * This returns 0 (it never fails, assuming the argument gives a valid memory location).
 * This has to be called at least once per 35 minutes to properly handle overflows of the 32-bit clock.
 * TODO: This is only addressable by setting up interrupts on a timer peripheral to occur at wrap.
 */

int _lf_clock_gettime(instant_t* t) {

    assert(t != NULL);

   *t = get_cpu_usecs() * 1000;
   
    return 0;
}

#if defined(LF_SINGLE_THREADED)

int lf_disable_interrupts_nested() {
    if (_lf_num_nested_critical_sections++ == 0) {
        intr_disable();
    }
    return 0;
}

int lf_enable_interrupts_nested() {
    if (_lf_num_nested_critical_sections <= 0) {
        return 1;
    }
    
    if (--_lf_num_nested_critical_sections == 0) {
        intr_enable();
    }
    return 0;
}

#endif