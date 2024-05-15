#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 *  * @file  * @author Hokeun Kim (hokeun@asu.edu)  * @author Chanhee Lee (chanheel@asu.edu)  * @copyright (c) 2023, Arizona State University  * License in [BSD 2-clause](..)  * @brief ..
 */
#define STARTING_PORT 15045

/**
 *  * Size of the buffer used for messages sent between federates.  * This is used by both the federates and the rti, so message lengths  * should generally match.
 */
// #define FED_COM_BUFFER_SIZE 256

/**
 *  * Delay the start of all federates by this amount.  * FIXME: More.  * FIXME: Should use the latency estimates that were  * acquired during initial clock synchronization.
 */
// #define DELAY_START 1

#define MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE 9

// #define NEVER INT64_MIN

// #define FOREVER INT64_MAX

// #define FOREVER_MICROSTEP UINT32_MAX

void run_rust_rti(void);
