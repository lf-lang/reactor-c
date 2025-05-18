#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "trace.h"
#include "trace_types.h"

int is_alphanumeric(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

void to_camel_case(char* s) {
  int capitalize_next = 1; // Flag to indicate whether the next character should be capitalized
  int j = 0;
  for (int i = 0; s[i] != '\0'; ++i) {
    if (!is_alphanumeric(s[i])) {
      capitalize_next = 1; // Treat non-alphanumeric characters as whitespace
    } else {
      if (capitalize_next) {
        s[j] = toupper(s[i]);
        capitalize_next = 0; // Reset the flag
      } else {
        s[j] = tolower(s[i]); // Convert to lowercase if not capitalizing
      }
      j++;
    }
  }
  s[j] = '\0';
}

typedef void (*string_consumer_t)(int, const char*, const char*);

void print_enum_variant(int idx, const char* camel_case, const char* description) {
  printf("    %s = %d,\n", camel_case, idx);
}

void print_match_case(int idx, const char* camel_case, const char* description) {
  printf("            EventType::%s => write!(f, \"%s\"),\n", camel_case, description);
}

void print_from_int(int idx, const char* camel_case, const char* description) {
  printf("            %d => Ok(EventType::%s),\n", idx, camel_case);
}

void do_for_each_camelcase(string_consumer_t sc) {
  for (int i = 0; i < NUM_EVENT_TYPES; i++) {
    size_t length = strlen(trace_event_names[i]);

    // Allocate memory for the new string including the null terminator
    char* destination = (char*)malloc((length + 1) * sizeof(char));

    // Check if memory allocation was successful
    if (destination == NULL) {
      perror("Memory allocation failed");
      exit(1);
    }

    // Copy the source string to the newly allocated buffer
    strcpy(destination, trace_event_names[i]);
    to_camel_case(destination);
    sc(i, destination, trace_event_names[i]);
  }
}

void print_display_impl() {
  printf("%s\n", "impl std::fmt::Display for EventType {");
  printf("%s\n", "    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {");
  printf("%s\n", "        match self {");
  do_for_each_camelcase(print_match_case);
  printf("%s\n", "        }");
  printf("%s\n", "    }");
  printf("%s\n", "}");
}

void print_rs_enum() {
  printf("%s\n", "#[derive(Debug)]");
  printf("%s\n", "pub enum EventType {");
  do_for_each_camelcase(print_enum_variant);
  printf("}\n");
}

void print_warning() {
  printf("%s\n", "/// Do not edit. Code in this file is generated from");
  printf("%s\n", "/// reactor-c/util/tracing/codegen/src/tracepoint_to_rs.c");
}

void print_rs_from_int() {
  printf("%s\n", "impl EventType {");
  printf("%s\n", "    pub fn try_from_int(i: i32) -> Result<Self, &'static str> {");
  printf("%s\n", "        match i {");
  do_for_each_camelcase(print_from_int);
  printf("%s\n", "            _ => Err(\"invalid event type\"),");
  printf("%s\n", "        }");
  printf("%s\n", "    }");
  printf("%s\n", "}");
}

int main() {
  print_warning();
  printf("%s", "\n");
  print_rs_enum();
  printf("%s", "\n");
  print_display_impl();
  printf("%s", "\n");
  print_rs_from_int();
}
