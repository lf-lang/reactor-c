#include <stdio.h>
#include <stdlib.h>
#include "../core/reactor.h"

void _lf_initialize_trigger_objects() {}
void terminate_execution() {}
bool _lf_trigger_shutdown_reactions() { return true; }
void _lf_set_default_command_line_options() {}
void _lf_trigger_startup_reactions() {}
void _lf_initialize_timers() {}


int main(int argc, char **argv) {
  char* buf = malloc(sizeof(char) * 128);
  lf_readable_time(buf, 0);
  printf("%s", buf);
  return 0;
}
