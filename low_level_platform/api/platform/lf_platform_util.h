#ifndef LF_PLATFORM_UTIL_H
#define LF_PLATFORM_UTIL_H
/**
 * @brief Maps a value from source range   into a destination priority range.
 */
int map_value(int value, int src_min, int src_max, int dest_min, int dest_max);

#endif
