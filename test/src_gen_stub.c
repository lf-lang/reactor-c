#include <stdbool.h>
#include "tag.h"

void _lf_initialize_trigger_objects() {}
void terminate_execution() {}
bool _lf_trigger_shutdown_reactions() { return true; }
void _lf_set_default_command_line_options() {}
void _lf_trigger_startup_reactions() {}
void _lf_initialize_timers() {}
void _lf_initialize_watchdog_mutexes() {}
void logical_tag_complete(tag_t tag_to_send) {}
