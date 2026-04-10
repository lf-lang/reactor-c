#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "logging/api/logging.h"
#include "initialize_from_file.h"

/** Remove leading and trailing whitespace from the string. */
static void sc_csv_trim(char* s) {
  char* p = s;
  while (*p && isspace((unsigned char)*p)) {
    p++;
  }
  if (p != s) {
    memmove(s, p, strlen(p) + 1);
  }
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) {
    s[--n] = '\0';
  }
}

/** Replace the specified delimiter with a null character and return the number of fields. */
static int sc_csv_split(char* line, char delimiter, char** fields, int max_fields) {
  int n = 0;
  char* cur = line;
  while (n < max_fields) {
    fields[n++] = cur;
    char* sep = strchr(cur, delimiter);
    if (!sep) {
      break;
    }
    *sep = '\0';
    cur = sep + 1;
  }
  return n;
}

int lf_initialize_double(const char* filename, char delimiter, size_t row_number, ...) {
  va_list ap;
  size_t pointer_count = 0;
  FILE* f = fopen(filename, "r");
  if (!f) {
    lf_print_error("Could not open file \"%s\".", filename);
    return -1;
  }

  char line[SC_CSV_LINE_MAX];
  size_t row_count = 0;
  while (fgets(line, sizeof(line), f)) {
    if (row_count == row_number) {
      char* row = &line[0];
      // Remove the UTF-8 BOM (byte-order mark) if it exists.
      if ((unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
        row += 3;
      }
      // Terminate the last item in the line with a null character.
      row[strcspn(row, "\r\n")] = '\0';

      char* fields[SC_CSV_MAX_COLS];
      int field_count = sc_csv_split(row, delimiter, fields, SC_CSV_MAX_COLS);
      va_start(ap, row_number);
      for (size_t i = 0; i < (size_t)field_count; i++) {
        double* out = va_arg(ap, double*);
        if (out == NULL) {
          break;
        }
        pointer_count++;
        sc_csv_trim(fields[i]);
        char* end = NULL;
        double parsed = strtod(fields[i], &end);
        if (end != fields[i] && *end == '\0') {
          *out = parsed;
        } else {
          lf_print_error(
              "Failed to parse numeric value \"%s\" at row %zu, column %zu in \"%s\".",
              fields[i], row_number, i, filename);
        }
      }
      va_end(ap);
      fclose(f);
      return (int)pointer_count;
    }
    row_count++;
  }
  lf_print_error("Requested row %zu not found in file \"%s\".", row_number, filename);
  fclose(f);
  return -1;
}
