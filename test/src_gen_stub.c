#include <stdbool.h>
#include "tag.h"
#include "environment.h"

environment_t _env;

void _lf_initialize_trigger_objects() {}
void terminate_execution() {}
void _lf_set_default_command_line_options() {}
void _lf_initialize_watchdog_mutexes() {}
void logical_tag_complete(tag_t tag_to_send) {}
int _lf_get_environments(environment_t ** envs) {
  *envs = &_env;
  return 1;
}