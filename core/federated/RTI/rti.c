/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni
 *
 * @section LICENSE
Copyright (c) 2020, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 * Runtime infrastructure for distributed Lingua Franca programs.
 *
 * This implementation creates one thread per federate so as to be able
 * to take advantage of multiple cores. It may be more efficient, however,
 * to use select() instead to read from the multiple socket connections
 * to each federate.
 *
 * This implementation sends messages in little endian order
 * because Intel, RISC V, and Arm processors are little endian.
 * This is not what is normally considered "network order",
 * but we control both ends, and hence, for commonly used
 * processors, this will be more efficient since it won't have
 * to swap bytes.
 *
 * This implementation of the RTI should be considered a reference
 * implementation. In the future it might be re-implemented in Java or Kotlin.
 * Or we could bootstrap and implement it using Lingua Franca.
 */

#include "rti_lib.h"
#include <signal.h>     // To trap ctrl-c and invoke a clean stop to save the trace file, if needed.

/**
 * References to the federation RTI and the enclave RTI.
 * They both point to the same enclaves stuctures. In the case of federation RTI,
 * however, enclaves are encapsulated in federates.    
 */
extern enclave_rti_t * _e_rti;
extern federation_rti_t* _f_rti;

/**
 * The tracing mechanism uses the number of workers variable `_lf_number_of_workers`.
 * For RTI tracing, the number of workers is set as the number of federates.
 */
unsigned int _lf_number_of_workers = 0u;

extern lf_mutex_t rti_mutex;
extern lf_cond_t received_start_times;
extern lf_cond_t sent_start_time;

/**
 * RTI trace file name
 */
const char *rti_trace_file_name = "rti.lft";

/**
 * @brief A clean termination of the RTI will write the trace file, if tracing is
 * enabled, before exiting.
 */
void termination() {
    if (_f_rti->tracing_enabled) {
        stop_trace(_f_rti->trace);
        trace_free(_f_rti->trace);
        lf_print("RTI trace file saved.");
    }   
    lf_print("RTI is exiting.");
}

int main(int argc, const char* argv[]) {

    initialize_RTI();

    lf_mutex_init(&rti_mutex);
    lf_cond_init(&received_start_times, &rti_mutex);
    lf_cond_init(&sent_start_time, &rti_mutex);

    // Catch the Ctrl-C signal, for a clean exit that does not lose the trace information
    signal(SIGINT, exit);
    if (atexit(termination) != 0) {
        lf_print_warning("Failed to register termination function!");
    }
    

    if (!process_args(argc, argv)) {
        // Processing command-line arguments failed.
        return -1;
    }
    if (_f_rti->tracing_enabled) {
        _lf_number_of_workers = _f_rti->number_of_enclaves;
        _f_rti->trace = trace_new(NULL, rti_trace_file_name);
        
        lf_assert(_f_rti->trace, "Out of memory");
        start_trace(_f_rti->trace);

        lf_print("Tracing the RTI execution in %s file.", rti_trace_file_name);
    }

    lf_print("Starting RTI for %d federates in federation ID %s.",  _f_rti->number_of_enclaves, _f_rti->federation_id);
    assert(_f_rti->number_of_enclaves < UINT16_MAX);
    
    // Allocate memory for the federates
    _f_rti->enclaves = (federate_t**)calloc(_f_rti->number_of_enclaves, sizeof(federate_t*));
    for (uint16_t i = 0; i < _f_rti->number_of_enclaves; i++) {
        _f_rti->enclaves[i] = (federate_t *)malloc(sizeof(federate_t));
        initialize_federate(_f_rti->enclaves[i], i);
    }

    // Initialize the RTI enclaves
    _e_rti = (enclave_rti_t*)_f_rti;

    int socket_descriptor = start_rti_server(_f_rti->user_specified_port);
    wait_for_federates(socket_descriptor);
    lf_print("RTI is exiting.");
    return 0;
}
