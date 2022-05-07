#ifndef CTARGET_TIME
#define CTARGET_TIME

#include "../core/reactor.h"

/**
 * Return the current logical time in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 * 
 * @return A time instant.
 */
instant_t lf_time_logical(void);
DEPRECATED(instant_t get_logical_time(void));

/**
 * Return the elapsed logical time in nanoseconds
 * since the start of execution.
 * @return A time interval.
 */
interval_t lf_time_logical_elapsed(void);
DEPRECATED(interval_t get_elapsed_logical_time(void));

/**
 * Return the current physical time in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 * @return A time instant.
 */
instant_t lf_time_physical(void);
DEPRECATED(instant_t get_physical_time(void));

/**
 * Return the elapsed physical time in nanoseconds.
 * This is the time returned by get_physical_time(void) minus the
 * physical start time as measured by get_physical_time(void) when
 * the program was started.
 */
instant_t lf_time_physical_elapsed(void);
DEPRECATED(instant_t get_elapsed_physical_time(void));

/**
 * Return the physical and logical time of the start of execution in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent. 
 * @return A time instant.
 */
instant_t lf_time_start(void);
DEPRECATED(instant_t get_start_time(void));

/**
 * Set a fixed offset to the physical clock.
 * After calling this, the value returned by get_physical_time(void)
 * and get_elpased_physical_time(void) will have this specified offset
 * added to what it would have returned before the call.
 */
void lf_set_physical_clock_offset(interval_t offset);
DEPRECATED(void set_physical_clock_offset(interval_t offset));


#endif // CTARGET_TIME
