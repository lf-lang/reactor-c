#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int event_type;
    void* pointer;
    int src_id;
    int dst_id;
    int64_t logical_time;
    int64_t microstep;
    int64_t physical_time;
    void* trigger;
    int64_t extra_delay;
} trace_record_nodeps_t;
void tracepoint(
    void* trace_void,
    int worker,
    trace_record_nodeps_t* tr
);
void lf_tracing_global_init(int process_id);
void lf_tracing_global_shutdown();
