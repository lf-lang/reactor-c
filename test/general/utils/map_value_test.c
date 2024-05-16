#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "platform/lf_platform_util.h"

int main() {
  int res;

  // Simple
  res = map_value(50, 0, 100, 0, 10);
  if (res != 5) {
    return -1;
  }

  // To a bigger range
  res = map_value(50, 0, 100, 0, 200);
  if (res != 100) {
    return -1;
  }

  // To an inverted range
  res = map_value(8, 0, 10, 10, 0);
  if (res != 2) {
    return -1;
  }

  // TO a range involving positive and negative
  res = map_value(8, 0, 10, 20, -20);
  if (res != -12) {
    return -1;
  }

  return 0;
}
