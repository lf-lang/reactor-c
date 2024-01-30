#ifndef API_TAG_H
#define API_TAG_H

#define NSEC(t) (t * 1LL)
#define NSECS(t) (t * 1LL)
#define USEC(t) (t * 1000LL)
#define USECS(t) (t * 1000LL)
#define MSEC(t) (t * 1000000LL)
#define MSECS(t) (t * 1000000LL)
#define SEC(t)  (t * 1000000000LL)
#define SECS(t) (t * 1000000000LL)
#define SECOND(t)  (t * 1000000000LL)
#define SECONDS(t) (t * 1000000000LL)
#define MINUTE(t)   (t * 60000000000LL)
#define MINUTES(t)  (t * 60000000000LL)
#define HOUR(t)  (t * 3600000000000LL)
#define HOURS(t) (t * 3600000000000LL)
#define DAY(t)   (t * 86400000000000LL)
#define DAYS(t)  (t * 86400000000000LL)
#define WEEK(t)  (t * 604800000000000LL)
#define WEEKS(t) (t * 604800000000000LL)

#define NEVER LLONG_MIN
#define NEVER_MICROSTEP 0u
#define FOREVER LLONG_MAX
#define FOREVER_MICROSTEP UINT_MAX
#define NEVER_TAG (tag_t) { .time = NEVER, .microstep = NEVER_MICROSTEP }
// Need a separate initializer expression to comply with some C compilers
#define NEVER_TAG_INITIALIZER { NEVER,  NEVER_MICROSTEP }
#define FOREVER_TAG (tag_t) { .time = FOREVER, .microstep = FOREVER_MICROSTEP }
// Need a separate initializer expression to comply with some C compilers
#define FOREVER_TAG_INITIALIZER { FOREVER,  FOREVER_MICROSTEP }
#define ZERO_TAG (tag_t) { .time = 0LL, .microstep = 0u }

// Convenience for converting times
#define BILLION 1000000000LL

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

////////////////  Type definitions

/**
 * Time instant. Both physical and logical times are represented
 * using this typedef.
 */
typedef int64_t instant_t;

/**
 * Interval of time.
 */
typedef int64_t interval_t;

/**
 * Microstep instant.
 */
typedef uint32_t microstep_t;

/**
 * A tag is a time, microstep pair.
 */
typedef struct {
    instant_t time;
    microstep_t microstep;
} tag_t;

#endif // API_TAG_H
