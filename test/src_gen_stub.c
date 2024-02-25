#include <stdbool.h>
#include "tag.h"
#include "environment.h"

/**
 * This file enables unit tests to run without there having been an actual code generation
 * from a Lingua Franca program. It defines (mostly empty) functions that would normally be
 * code generated. Of course, this strategy will only work for tests that do not actually
 * need functional versions of these functions.
 */

environment_t _env;

void _lf_initialize_trigger_objects(void) {}
void lf_terminate_execution(void) {}
void lf_set_default_command_line_options(void) {}
void _lf_initialize_watchdogs(environment_t ** envs) {}
void logical_tag_complete(tag_t tag_to_send) {}
int _lf_get_environments(environment_t ** envs) {
  *envs = &_env;
  return 1;
}