#ifndef PLATFORM_INTERNAL_H
#define PLATFORM_INTERNAL_H
/**
 * @brief Maps a priority into a destination priority range.
 */
int map_priorities(int priority, int dest_min, int dest_max);

#endif