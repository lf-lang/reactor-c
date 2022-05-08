#include "../include/ctarget/time.h"

/**
 * Return the current logical time in nanoseconds since January 1, 1970.
 */
instant_t lf_time_logical() {
    return _lf_time(LF_LOGICAL);
}
/**
 * @deprecated version of "lf_time_logical"
 */
instant_t get_logical_time() { return lf_time_logical(); }

/**
 * Return the elapsed logical time in nanoseconds since the start of execution.
 */
interval_t lf_time_logical_elapsed() {
    return _lf_time(LF_ELAPSED_LOGICAL);
}
/**
 * @deprecated version of "lf_time_logical_elapsed"
 */
interval_t get_elapsed_logical_time() { return lf_time_logical_elapsed(); }


/**
 * Return the current physical time in nanoseconds since January 1, 1970,
 * adjusted by the global physical time offset.
 */
instant_t lf_time_physical() {
    return _lf_time(LF_PHYSICAL);
}
/**
 * @deprecated version of "lf_time_physical"
 */
instant_t get_physical_time() { return lf_time_physical(); }

/**
 * Return the elapsed physical time in nanoseconds.
 * This is the time returned by get_physical_time() minus the
 * physical start time as measured by get_physical_time() when
 * the program was started.
 */
instant_t lf_time_physical_elapsed() {
    return _lf_time(LF_ELAPSED_PHYSICAL);
}
/**
 * @deprecated version of "lf_time_physical_elapsed"
 */
instant_t get_elapsed_physical_time() { return lf_time_physical_elapsed(); }

/**
 * Return the physical time of the start of execution in nanoseconds. * 
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent. * 
 * @return A time instant.
 */
instant_t lf_time_start(void) {
    return _lf_time(LF_START);
}
/**
 * @deprecated version of "lf_time_start"
 */
instant_t get_start_time() { return lf_time_start(); }


/**
 * Set a fixed offset to the physical clock.
 * After calling this, the value returned by get_physical_time()
 * and get_elpased_physical_time() will have this specified offset
 * added to what it would have returned before the call.
 */
void lf_set_physical_clock_offset(interval_t offset) {
    _lf_set_physical_clock_offset(offset);
}

/**
 * @deprecated version of 'lf_set_physical_clock_offset'
 */
void set_physical_clock_offset(interval_t offset) {
    lf_set_physical_clock_offset(offset);
}
