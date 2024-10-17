/**
 * @file lf_core_version.h
 * @author Peter Donovan (peter@xronos.com)
 * @brief API for runtime plugins to use to sanity-check compatibility with the
 * core. Plugins APIs can include a function to get information about the
 * version of the plugin, and the core can use that information to determine if
 * the plugin is compatible with the core.
 * @version 0.1
 * @date 2024-01-29
 *
 * @copyright Copyright (c) 2024
 */
#ifndef VERSION_H
#define VERSION_H

typedef enum {
  TRIBOOL_FALSE = 0,
  TRIBOOL_TRUE = 1,
  TRIBOOL_DOES_NOT_MATTER = 2,
} tribool_t;

typedef struct {
  tribool_t single_threaded;
  tribool_t build_type_is_debug;
  int log_level;
} build_config_t;

typedef struct {
  const build_config_t build_config;
  const char* core_version_name;
} version_t;

#endif // VERSION_H
