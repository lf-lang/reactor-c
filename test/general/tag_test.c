#include <stdio.h>
#include <stdlib.h>
#include "lf_types.h"

int main() {
  char* buf = malloc(sizeof(char) * 128);
  lf_readable_time(buf, 0);
  printf("%s", buf);
  return 0;
}
