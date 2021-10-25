#include <stdio.h>
#include <stdlib.h>
#include "tag.h"
#include "reactor.h"

void _lf_initialize_timers() {}
void logical_tag_complete(tag_t tag_to_send) {}
void terminate_execution() {}
void _lf_trigger_startup_reactions() {}
void _lf_initialize_trigger_objects() {}
bool _lf_trigger_shutdown_reactions() { return true; }

int main(int argc, char **argv) {
  char* buf = malloc(sizeof(char) * 128);
  lf_readable_time(buf, 0);
  printf("%s", buf);
  return 0;
}
