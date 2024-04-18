
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"

int main(int argc, char* argv[]) {
  assert(map(5, 0, 10, 0, 100) == 50);
  assert(map(5, 0, 99, 0, 99) == 5);
  assert(map(3, 0, 10, 50, 0) == 35);
}