#include <stdio.h>  // debugging only

#include "trace.h"

void tracepoint(
        void* trace_void,
        int worker,
        trace_record_nodeps_t* tr
) {
    printf("default tracepoint called in worker %d\n", worker);
    // trace_t* trace = (trace_t*) trace_void;

    // environment_t *env = trace->env;
    // // Worker argument determines which buffer to write to.
    // int index = (worker >= 0) ? worker : 0;

    // // Flush the buffer if it is full.
    // if (trace->_lf_trace_buffer_size[index] >= TRACE_BUFFER_CAPACITY) {
    //     // No more room in the buffer. Write the buffer to the file.
    //     flush_trace(trace, index);
    // }
    // // The above flush_trace resets the write pointer.
    // int i = trace->_lf_trace_buffer_size[index];
    // // Write to memory buffer.
    // // Get the correct time of the event

    // trace->_lf_trace_buffer[index][i] = *tr;
    // trace->_lf_trace_buffer_size[index]++;
}

void lf_tracing_global_init(int process_id) {
    // Do nothing.
    printf("default lf_tracing_global_init called\n");
}
void lf_tracing_global_shutdown() {
    // Do nothing.
    printf("default lf_tracing_global_shutdown called\n");
}
