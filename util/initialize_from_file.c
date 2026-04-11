#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "logging/api/logging.h"
#include "reactor.h"
#include "initialize_from_file.h"

typedef enum { LF_TYPE_DOUBLE, LF_TYPE_INT, LF_TYPE_STRING } lf_field_type;

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

/**
 * Shared implementation for lf_initialize_double, lf_initialize_int, and _lf_initialize_string.
 * The `type` parameter selects the expected va_arg pointer type (double*, int*, or char**).
 * The `allocations` parameter is used only for LF_TYPE_STRING to record allocated memory.
 */
static int lf_initialize_fields(const char* filename, char delimiter, size_t row_number, lf_field_type type,
                                struct allocation_record_t** allocations, va_list ap) {
  size_t pointer_count = 0;
  FILE* f = fopen(filename, "r");
  if (!f) {
    lf_print_error("Could not open file \"%s\".", filename);
    return -1;
  }

  char line[SC_CSV_LINE_MAX];
  size_t row_count = 0;
  while (fgets(line, sizeof(line), f)) {
    // Detect truncation: fgets didn't capture a newline and we're not at EOF.
    size_t len = strlen(line);
    int truncated = (len > 0 && line[len - 1] != '\n' && !feof(f));

    if (row_count == row_number) {
      if (truncated) {
        lf_print_error("Row %zu exceeds maximum line length of %d in file \"%s\".", row_number, SC_CSV_LINE_MAX - 1,
                       filename);
        fclose(f);
        return -1;
      }
      char* row = &line[0];
      if (len >= 3 && (unsigned char)row[0] == 0xEF && (unsigned char)row[1] == 0xBB &&
          (unsigned char)row[2] == 0xBF) {
        row += 3;
      }
      row[strcspn(row, "\r\n")] = '\0';

      char* fields[SC_CSV_MAX_COLS];
      int field_count = sc_csv_split(row, delimiter, fields, SC_CSV_MAX_COLS);
      for (size_t i = 0; i < (size_t)field_count; i++) {
        sc_csv_trim(fields[i]);
        if (type == LF_TYPE_DOUBLE) {
          double* out = va_arg(ap, double*);
          if (out == NULL)
            break;
          pointer_count++;
          char* end = NULL;
          double parsed = strtod(fields[i], &end);
          if (end != fields[i] && *end == '\0') {
            *out = parsed;
          } else {
            lf_print_error("Failed to parse numeric value \"%s\" at row %zu, column %zu in \"%s\".", fields[i],
                           row_number, i, filename);
            fclose(f);
            return -1;
          }
        } else if (type == LF_TYPE_INT) {
          int* out = va_arg(ap, int*);
          if (out == NULL)
            break;
          pointer_count++;
          char* end = NULL;
          long parsed = strtol(fields[i], &end, 10);
          if (end != fields[i] && *end == '\0') {
            *out = (int)parsed;
          } else {
            lf_print_error("Failed to parse integer value \"%s\" at row %zu, column %zu in \"%s\".", fields[i],
                           row_number, i, filename);
            fclose(f);
            return -1;
          }
        } else {
          char** out = va_arg(ap, char**);
          if (out == NULL)
            break;
          pointer_count++;
          char* field = fields[i];
          size_t len = strlen(field);
          if (len >= 2 &&
              ((field[0] == '"' && field[len - 1] == '"') || (field[0] == '\'' && field[len - 1] == '\''))) {
            field++;
            len -= 2;
          }
          char* str = (char*)lf_allocate(len + 1, sizeof(char), allocations);
          memcpy(str, field, len);
          str[len] = '\0';
          *out = str;
        }
      }
      fclose(f);
      return (int)pointer_count;
    }
    if (truncated) {
      // Consume the rest of this over-long line so it counts as one row.
      while (fgets(line, sizeof(line), f)) {
        len = strlen(line);
        if ((len > 0 && line[len - 1] == '\n') || feof(f)) break;
      }
    }
    row_count++;
  }
  lf_print_error("Requested row %zu not found in file \"%s\".", row_number, filename);
  fclose(f);
  return -1;
}

int lf_initialize_double(const char* filename, char delimiter, size_t row_number, ...) {
  va_list ap;
  va_start(ap, row_number);
  int result = lf_initialize_fields(filename, delimiter, row_number, LF_TYPE_DOUBLE, NULL, ap);
  va_end(ap);
  return result;
}

int lf_initialize_int(const char* filename, char delimiter, size_t row_number, ...) {
  va_list ap;
  va_start(ap, row_number);
  int result = lf_initialize_fields(filename, delimiter, row_number, LF_TYPE_INT, NULL, ap);
  va_end(ap);
  return result;
}

int _lf_initialize_string(const char* filename, char delimiter, size_t row_number,
                          struct allocation_record_t** allocations, ...) {
  va_list ap;
  va_start(ap, allocations);
  int result = lf_initialize_fields(filename, delimiter, row_number, LF_TYPE_STRING, allocations, ap);
  va_end(ap);
  return result;
}
