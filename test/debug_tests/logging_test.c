#include "logging_macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <regex.h>

#define TIMEED_DEBUG_CHAR_LEN 30
#define SOME_EXTRA_SPACE 10
/**
 * @brief unit for LF_PRINT_DEBUG macro
 * LOG_LEVEL must be in LOG_LEVEL_DEBUG
 */
void test_logging_macro(const char* expected, int st_len) {

  FILE* tmp = tmpfile(); // auto-deletes when closed
  char* buffer;
  char pattern[256];
  regex_t re;
  int result;

  // Computing the buffer size based on strlen("DEBUG: ") + \0 + \n + extra space
  int buffer_size = st_len + TIMEED_DEBUG_CHAR_LEN + SOME_EXTRA_SPACE;

  if (!tmp) {
    perror("tmpfile");
    exit(1);
  }

  // Redirect stdout -> tmp
  fflush(stdout);
  int fd = fileno(tmp);
  if (fd == -1 || dup2(fd, STDOUT_FILENO) == -1) {
    perror("redirect stdout");
    exit(1);
  }

  // Call code under test
  LF_PRINT_DEBUG("%s", expected);

  fflush(stdout); // flush so data goes into tmp
  rewind(tmp);    // reset read position

  // Read back
  buffer = (char*)malloc(buffer_size);
  size_t n = fread(buffer, 1, buffer_size - 1, tmp);
  buffer[n] = '\0';

  // Regex to check format: DEBUG: [number]expected\n
  snprintf(pattern, sizeof(pattern), "^DEBUG: \\[-?[0-9]+\\]%s\\n?$", expected);

  if (regcomp(&re, pattern, REG_EXTENDED | REG_NEWLINE) != 0) {
    perror("regcomp");
    exit(1);
  }

  result = regexec(&re, buffer, 0, NULL, 0);
  regfree(&re);

  assert(result == 0); // match succeeded

  fclose(tmp); // deletes the file
  free(buffer);
}

int main() {
  char* str_test = "Hello World";
  test_logging_macro(str_test, strlen(str_test));
  return 0;
}
