/**
 * @file version-abi.h
 * @author Peter Donovan (peter@xronos.com)
 * @brief API for runtime plugins to use to sanity-check compatibility with the
 * core.
 * @version 0.1
 * @date 2024-01-29
 *
 * @copyright Copyright (c) 2024
 */
#ifndef VERSION_ABI_H
#define VERSION_ABI_H

typedef enum {
  FALSE = 0,
  TRUE = 1,
  DOES_NOT_MATTER = 2,
} tribool_t;

typedef struct {
  tribool_t single_threaded;
  tribool_t build_type_is_debug;
  int log_level;
  char* core_sha;
} version_t;

#endif // VERSION_ABI_H
