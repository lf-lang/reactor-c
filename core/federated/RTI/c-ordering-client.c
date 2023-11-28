#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

const char* LIBRARY_PATH_ENV_VAR = "C_ORDERING_CLIENT_LIBRARY_PATH";

#include "c-ordering-client.h"

ClientAndJoinHandle start_client_nop(int federate_id) {
  return (ClientAndJoinHandle) { .client = NULL, .join_handle = NULL };
}
void finish_nop(ClientAndJoinHandle) {}
void tracepoint_maybe_wait_nop(void* client, char* hook_id, int federate_id, int sequence_number) {}
void tracepoint_maybe_notify_nop(void* client, char* hook_id, int federate_id, int sequence_number) {}
void tracepoint_maybe_do_nop(void* client, char* hook_id, int federate_id, int sequence_number) {}

OrderingClientApi* load_ordering_client_api() {
  char* library_path = getenv(LIBRARY_PATH_ENV_VAR);
  if (!library_path) {
    fprintf(stderr, "environment variable %s not set\n", LIBRARY_PATH_ENV_VAR);
    OrderingClientApi* api = malloc(sizeof(OrderingClientApi));
    api->start_client = start_client_nop;
    api->finish = finish_nop;
    api->tracepoint_maybe_wait = tracepoint_maybe_wait_nop;
    api->tracepoint_maybe_notify = tracepoint_maybe_notify_nop;
    api->tracepoint_maybe_do = tracepoint_maybe_do_nop;
    return api;
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
