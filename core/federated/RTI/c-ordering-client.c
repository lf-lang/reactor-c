#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

const char* LIBRARY_PATH_ENV_VAR = "C_ORDERING_CLIENT_LIBRARY_PATH";

#include "c-ordering-client.h"

OrderingClientApi* load_ordering_client_api() {
  char* library_path = getenv(LIBRARY_PATH_ENV_VAR);
  if (!library_path) {
    fprintf(stderr, "environment variable %s not set\n", LIBRARY_PATH_ENV_VAR);
    exit(1);
  }
  void* handle = dlopen(library_path, RTLD_LAZY);
  if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }
  OrderingClientApi* api = dlsym(handle, "ORDERING_CLIENT_API");
  if (!api) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }
  return api;
}
