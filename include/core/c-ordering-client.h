#ifndef C_ORDERING_CLIENT_H
#define C_ORDERING_CLIENT_H

typedef struct {
    void *client;
    void *join_handle;
} ClientAndJoinHandle;

typedef ClientAndJoinHandle (*start_client_t) (int federate_id);
typedef void (*drop_join_handle_t) (void* join_handle);
typedef void tracepoint_maybe_wait_t(void* client, char* hook_id, int federate_id, int sequence_number);
typedef void tracepoint_maybe_notify_t(void* client, char* hook_id, int federate_id, int sequence_number);
typedef void tracepoint_maybe_do_t(void* client, char* hook_id, int federate_id, int sequence_number);

typedef struct {
    start_client_t start_client;
    drop_join_handle_t drop_join_handle;
    tracepoint_maybe_wait_t* tracepoint_maybe_wait;
    tracepoint_maybe_notify_t* tracepoint_maybe_notify;
    tracepoint_maybe_do_t* tracepoint_maybe_do;
} OrderingClientApi;

OrderingClientApi* load_ordering_client_api();

#endif // C_ORDERING_CLIENT_H
