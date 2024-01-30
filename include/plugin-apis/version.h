/**
 * @file version.h
 * @author Peter Donovan (peter@xronos.com)
 * @brief API for runtime plugins to use to sanity-check compatibility with the
 * core.
 * @version 0.1
 * @date 2024-01-29
 *
 * @copyright Copyright (c) 2024
 */

typedef enum {
  FALSE = 0,
  TRUE = 1,
  DOES_NOT_MATTER = 2,
} tribool_t;

#define NAMESPACE_BY_PLUGIN(NAME) NAME ## LF_CURRENT_PLUGIN_NAME

/**
 * @brief Return whether a threading library is required.
 */
tribool_t NAMESPACE_BY_PLUGIN(lf_version_single_threaded)();
tribool_t NAMESPACE_BY_PLUGIN(lf_version_build_type_is_debug)();
tribool_t NAMESPACE_BY_PLUGIN(lf_version_tracing_enabled)();
char* NAMESPACE_BY_PLUGIN(lf_version_log_level)();
char* NAMESPACE_BY_PLUGIN(lf_version_core_sha)();
