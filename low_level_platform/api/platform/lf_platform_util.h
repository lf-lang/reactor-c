/**
 * @file lf_platform_util.h
 * @brief Utility functions for platform-specific operations in the Lingua Franca C runtime.
 *
 * This header file provides utility functions that are used across different
 * platform implementations to handle common platform-specific operations.
 */

#ifndef LF_PLATFORM_UTIL_H
#define LF_PLATFORM_UTIL_H

/**
 * @brief Maps a priority into a destination priority range.
 */
int map_priorities(int priority, int dest_min, int dest_max);

#endif