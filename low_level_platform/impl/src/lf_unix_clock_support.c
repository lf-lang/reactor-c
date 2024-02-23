#if defined(PLATFORM_Linux) || defined(PLATFORM_Darwin)
#include <time.h>
#include <errno.h>

#include "low_level_platform.h"
// #include "util.h"
#include "logging.h"
#include "platform/lf_unix_clock_support.h"

#ifndef LF_CLOCK_SUPPORT_BILLION
#define LF_CLOCK_SUPPORT_BILLION ((instant_t) 1000000000LL)
#endif

instant_t convert_timespec_to_ns(struct timespec tp) {
    return ((instant_t) tp.tv_sec) * LF_CLOCK_SUPPORT_BILLION + tp.tv_nsec;
}

struct timespec convert_ns_to_timespec(instant_t t) {
    struct timespec tp;
    tp.tv_sec = t / LF_CLOCK_SUPPORT_BILLION;
    tp.tv_nsec = (t % LF_CLOCK_SUPPORT_BILLION);
    return tp;
}

void _lf_initialize_clock() {
    struct timespec res;
    int return_value = clock_getres(CLOCK_REALTIME, (struct timespec*) &res);
    if (return_value < 0) {
        lf_print_error_and_exit("Could not obtain resolution for CLOCK_REALTIME");
    }

    lf_print("---- System clock resolution: %ld nsec", res.tv_nsec);
}

/**
 * Fetch the value of CLOCK_REALTIME and store it in t.
 * @return 0 for success, or -1 for failure.
 */
int _lf_clock_gettime(instant_t* t) {
    if (t == NULL) return -1;
    struct timespec tp;
    if (clock_gettime(CLOCK_REALTIME, (struct timespec*) &tp) != 0) {
        return -1;
    }
    *t = convert_timespec_to_ns(tp);
    return 0;
}

#endif
