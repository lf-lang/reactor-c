/**
 * @file lf_unix_clock_support.h
 * @brief Unix clock support functions for the Lingua Franca C runtime.
 *
 * This header file provides utility functions for converting between Unix
 * timespec structures and Lingua Franca time representations. These functions
 * are used by platform implementations that rely on Unix clock mechanisms.
 */

#include <time.h>
#include <errno.h>

/**
 * @brief Convert a _lf_time_spec_t ('tp') to an instant_t representation in
 * nanoseconds.
 *
 * @return nanoseconds (long long).
 */
instant_t convert_timespec_to_ns(struct timespec tp);

/**
 * @brief Convert an instant_t ('t') representation in nanoseconds to a
 * _lf_time_spec_t.
 *
 * @return _lf_time_spec_t representation of 't'.
 */
struct timespec convert_ns_to_timespec(instant_t t);
