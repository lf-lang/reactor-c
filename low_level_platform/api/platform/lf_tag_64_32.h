/**
 * @file lf_tag_64_32.h
 * @brief Format strings for printf for 64-bit times and 32-bit unsigned microsteps.
 *
 * Define PRINTF_TIME and PRINTF_MICROSTEP, which are the printf
 * codes (like the d in %d to print an int) for time and microsteps.
 * To use these, specify the printf as follows:
 *     printf("Time: " PRINTF_TIME "\n", time_value);
 * On most platforms, time is an signed 64-bit number (int64_t) and
 * the microstep is an unsigned 32-bit number (uint32_t).
 * Sadly, in C, there is no portable to print such numbers using
 * printf without getting a warning on some platforms.
 * On each platform, the code for printf if given by the macros
 * PRId64 and PRIu32 defined in inttypes.h.  Hence, here, we import
 * inttypes.h, then define PRINTF_TIME and PRINTF_MICROSTEP.
 * If you are targeting a platform that uses some other type
 * for time and microsteps, you can simply define
 * PRINTF_TIME and PRINTF_MICROSTEP directly in the same file that
 * defines the types instant_t, interval_t, and microstep_t.
 */

#include <inttypes.h>
#define PRINTF_TIME "%" PRId64
#define PRINTF_MICROSTEP "%" PRIu32

// For convenience, the following string can be inserted in a printf
// format for printing both time and microstep as follows:
//     printf("Tag is " PRINTF_TAG "\n", time_value, microstep);
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"
