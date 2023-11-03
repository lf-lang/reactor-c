#if defined(PLATFORM_Linux) || defined(PLATFORM_Darwin)
#include <time.h>
#include <errno.h>

#include "platform.h"
#include "util.h"
#include "lf_unix_clock_support.h"

/**
 * Offset to _LF_CLOCK that would convert it
 * to epoch time. This is applied to the physical clock
 * to get a more meaningful and universal time.
 *
 * For CLOCK_REALTIME, this offset is always zero.
 * For CLOCK_MONOTONIC, it is the difference between those
 * clocks at the start of the execution.
 */
interval_t _lf_time_epoch_offset = 0LL;

instant_t convert_timespec_to_ns(struct timespec tp) {
    return ((instant_t) tp.tv_sec) * BILLION + tp.tv_nsec;
}

struct timespec convert_ns_to_timespec(instant_t t) {
    struct timespec tp;
    tp.tv_sec = t / BILLION;
    tp.tv_nsec = (t % BILLION);
    return tp;
}

void calculate_epoch_offset(void) {
    if (_LF_CLOCK == CLOCK_REALTIME) {
        // Set the epoch offset to zero (see tag.h)
        _lf_time_epoch_offset = 0LL;
    } else {
        // Initialize _lf_time_epoch_offset to the difference between what is
        // reported by whatever clock LF is using (e.g. CLOCK_MONOTONIC) and
        // what is reported by CLOCK_REALTIME.
        struct timespec physical_clock_snapshot, real_time_start;

        clock_gettime(_LF_CLOCK, &physical_clock_snapshot);
        long long physical_clock_snapshot_ns = convert_timespec_to_ns(physical_clock_snapshot);


        clock_gettime(CLOCK_REALTIME, &real_time_start);
        long long real_time_start_ns = convert_timespec_to_ns(real_time_start);

        _lf_time_epoch_offset = real_time_start_ns - physical_clock_snapshot_ns;
    }
}

void _lf_initialize_clock() {
    calculate_epoch_offset();

    struct timespec res;
    int return_value = clock_getres(_LF_CLOCK, (struct timespec*) &res);
    if (return_value < 0) {
        lf_print_error_and_exit("Could not obtain resolution for _LF_CLOCK");
    }

    lf_print("---- System clock resolution: %ld nsec", res.tv_nsec);
}

/**
 * Fetch the value of _LF_CLOCK (see lf_linux_support.h) and store it in tp. The
 * timestamp value in 't' will always be epoch time, which is the number of
 * nanoseconds since January 1st, 1970.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_gettime`).
 */
int _lf_clock_now(instant_t* t) {
    struct timespec tp;
    // Adjust the clock by the epoch offset, so epoch time is always reported.
    int return_value = clock_gettime(_LF_CLOCK, (struct timespec*) &tp);
    if (return_value < 0) {
        return -1;
    }

    instant_t tp_in_ns = convert_timespec_to_ns(tp);

    // We need to apply the epoch offset if it is not zero
    if (_lf_time_epoch_offset != 0) {
        tp_in_ns += _lf_time_epoch_offset;
    }

    if (t == NULL) {
        // The t argument address references invalid memory
        errno = EFAULT;
        return -1;
    }

    *t = tp_in_ns;
    return return_value;
}
#endif
