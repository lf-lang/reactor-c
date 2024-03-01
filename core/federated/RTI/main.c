#if defined STANDALONE_RTI
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

#include "rti_remote.h"
#include "net_util.h"
#include <signal.h>     // To trap ctrl-c and invoke a clean stop to save the trace file, if needed.
#include <string.h>


/**
 * The tracing mechanism uses the number of workers variable `_lf_number_of_workers`.
 * For RTI tracing, the number of workers is set as the number of federates.
 */
unsigned int _lf_number_of_workers = 0u;

// The global RTI object. It is static, and can only be referenced in this file.
// A pointer is passed during initialization to rti_remote.c
static rti_remote_t rti;

/**
 * RTI trace file name
 */
const char *rti_trace_file_name = "rti.lft";

/** Indicator that normal termination of the RTI has occurred. */
bool normal_termination = false;

/**
 * Send a failed signal to the specified federate.
 */
static void send_failed_signal(federate_info_t* fed) {
    size_t bytes_to_write = 1;
    unsigned char buffer[bytes_to_write];
    buffer[0] = MSG_TYPE_FAILED;
    if (rti.base.tracing_enabled) {
        tracepoint_rti_to_federate(send_FAILED, fed->enclave.id, NULL);
    }
    int failed = write_to_socket(fed->socket, bytes_to_write, &(buffer[0]));
    if (failed == 0) {
        LF_PRINT_LOG("RTI has sent failed signal to federate %d due to abnormal termination.", fed->enclave.id);
    } else {
        lf_print_error("RTI failed to send failed signal to federate %d on socket ID %d.", fed->enclave.id, fed->socket);
    }
}

/**
 * @brief Function to run upon termination.
 * 
 * This function will be invoked both after main() returns and when a signal
 * that results in terminating the process, such as SIGINT.  In the former
 * case, it should do nothing.  In the latter case, it will send a MSG_TYPE_FAILED
 * signal to each federate and attempt to write the trace file, but without
 * acquiring a mutex lock, so the resulting files may be incomplete or even
 * corrupted.  But this is better than just failing to write the data we have
 * collected so far.
 */
void termination() {
    if (!normal_termination) {
        for (int i = 0; i < rti.base.number_of_scheduling_nodes; i++) {
            federate_info_t *f = (federate_info_t*)rti.base.scheduling_nodes[i];
            if (!f || f->enclave.state == NOT_CONNECTED) continue;
            send_failed_signal(f);
        }
        if (rti.base.tracing_enabled) {
            lf_tracing_global_shutdown();
            lf_print("RTI trace file saved.");
        }
        lf_print("RTI is exiting abnormally.");
    }   
}

void usage(int argc, const char* argv[]) {
    lf_print("\nCommand-line arguments: \n");
    lf_print("  -i, --id <n>");
    lf_print("   The ID of the federation that this RTI will control.\n");
    lf_print("  -n, --number_of_federates <n>");
    lf_print("   The number of federates in the federation that this RTI will control.\n");
    lf_print("  -p, --port <n>");
    lf_print("   The port number to use for the RTI. Must be larger than 0 and smaller than %d. Default is %d.\n", UINT16_MAX, DEFAULT_PORT);
    lf_print("  -c, --clock_sync [off|init|on] [period <n>] [exchanges-per-interval <n>]");
    lf_print("   The status of clock synchronization for this federate.");
    lf_print("       - off: Clock synchronization is off.");
    lf_print("       - init (default): Clock synchronization is done only during startup.");
    lf_print("       - on: Clock synchronization is done both at startup and during the execution.");
    lf_print("   Relevant parameters that can be set: ");
    lf_print("       - period <n>(in nanoseconds): Controls how often a clock synchronization attempt is made");
    lf_print("          (period in nanoseconds, default is 5 msec). Only applies to 'on'.");
    lf_print("       - exchanges-per-interval <n>: Controls the number of messages that are exchanged for each");
    lf_print("          clock sync attempt (default is 10). Applies to 'init' and 'on'.\n");
    lf_print("  -a, --auth Turn on HMAC authentication options.\n");
    lf_print("  -t, --tracing Turn on tracing.\n");

    lf_print("Command given:");
    for (int i = 0; i < argc; i++) {
        lf_print("%s ", argv[i]);
    }
    lf_print("\n");
}

int process_clock_sync_args(int argc, const char* argv[]) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "off") == 0) {
            rti.clock_sync_global_status = clock_sync_off;
            lf_print("RTI: Clock sync: off");
        } else if (strcmp(argv[i], "init") == 0 || strcmp(argv[i], "initial") == 0) {
            rti.clock_sync_global_status = clock_sync_init;
            lf_print("RTI: Clock sync: init");
        } else if (strcmp(argv[i], "on") == 0) {
            rti.clock_sync_global_status = clock_sync_on;
            lf_print("RTI: Clock sync: on");
        } else if (strcmp(argv[i], "period") == 0) {
            if (rti.clock_sync_global_status != clock_sync_on) {
                lf_print_error("clock sync period can only be set if --clock-sync is set to on.");
                usage(argc, argv);
                i++;
                continue; // Try to parse the rest of the arguments as clock sync args.
            } else if (argc < i + 2) {
                lf_print_error("clock sync period needs a time (in nanoseconds) argument.");
                usage(argc, argv);
                continue;
            }
            i++;
            long long period_ns = strtoll(argv[i], NULL, 10);
            if (period_ns == 0LL || period_ns == LLONG_MAX || period_ns == LLONG_MIN) {
                lf_print_error("clock sync period value is invalid.");
                continue; // Try to parse the rest of the arguments as clock sync args.
            }
            rti.clock_sync_period_ns = (int64_t)period_ns;
            lf_print("RTI: Clock sync period: %lld", (long long int)rti.clock_sync_period_ns);
        } else if (strcmp(argv[i], "exchanges-per-interval") == 0) {
            if (rti.clock_sync_global_status != clock_sync_on && rti.clock_sync_global_status != clock_sync_init) {
                lf_print_error("clock sync exchanges-per-interval can only be set if\n"
                               "--clock-sync is set to on or init.");
                usage(argc, argv);
                continue; // Try to parse the rest of the arguments as clock sync args.
            } else if (argc < i + 2) {
                lf_print_error("clock sync exchanges-per-interval needs an integer argument.");
                usage(argc, argv);
                continue; // Try to parse the rest of the arguments as clock sync args.
            }
            i++;
            long exchanges = (long)strtol(argv[i], NULL, 10);
            if (exchanges == 0L || exchanges == LONG_MAX ||  exchanges == LONG_MIN) {
                 lf_print_error("clock sync exchanges-per-interval value is invalid.");
                 continue; // Try to parse the rest of the arguments as clock sync args.
             }
            rti.clock_sync_exchanges_per_interval = (int32_t)exchanges; // FIXME: Loses numbers on 64-bit machines
            lf_print("RTI: Clock sync exchanges per interval: %d", rti.clock_sync_exchanges_per_interval);
        } else if (strcmp(argv[i], " ") == 0) {
            // Tolerate spaces
            continue;
        } else {
            // Either done with the clock sync args or there is an invalid
            // character. In  either case, let the parent function deal with
            // the rest of the characters;
            return i;
        }
    }
    return argc;
}

int process_args(int argc, const char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--id") == 0) {
            if (argc < i + 2) {
                lf_print_error("--id needs a string argument.");
                usage(argc, argv);
                return 0;
            }
            i++;
            lf_print("RTI: Federation ID: %s", argv[i]);
            rti.federation_id = argv[i];
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--number_of_federates") == 0) {
            if (argc < i + 2) {
                lf_print_error("--number_of_federates needs an integer argument.");
                usage(argc, argv);
                return 0;
            }
            i++;
            long num_federates = strtol(argv[i], NULL, 10);
            if (num_federates == 0L || num_federates == LONG_MAX ||  num_federates == LONG_MIN) {
                lf_print_error("--number_of_federates needs a valid positive integer argument.");
                usage(argc, argv);
                return 0;
            }
            rti.base.number_of_scheduling_nodes = (int32_t)num_federates; // FIXME: Loses numbers on 64-bit machines
            lf_print("RTI: Number of federates: %d", rti.base.number_of_scheduling_nodes);
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (argc < i + 2) {
                lf_print_error(
                    "--port needs a short unsigned integer argument ( > 0 and < %d).",
                    UINT16_MAX
                );
                usage(argc, argv);
                return 0;
            }
            i++;
            uint32_t RTI_port = (uint32_t)strtoul(argv[i], NULL, 10);
            if (RTI_port <= 0 || RTI_port >= UINT16_MAX) {
                lf_print_error(
                    "--port needs a short unsigned integer argument ( > 0 and < %d).",
                    UINT16_MAX
                );
                usage(argc, argv);
                return 0;
            }
            rti.user_specified_port = (uint16_t)RTI_port;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--clock_sync") == 0) {
            if (argc < i + 2) {
               lf_print_error("--clock-sync needs off|init|on.");
               usage(argc, argv);
               return 0;
           }
           i++;
           i += process_clock_sync_args((argc-i), &argv[i]);
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--auth") == 0) {
            #ifndef __RTI_AUTH__
            lf_print_error("--auth requires the RTI to be built with the -DAUTH=ON option.");
            usage(argc, argv);
            return 0;
            #endif
            rti.authentication_enabled = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tracing") == 0) {
            rti.base.tracing_enabled = true;
        } else if (strcmp(argv[i], " ") == 0) {
            // Tolerate spaces
            continue;
        }  else {
           lf_print_error("Unrecognized command-line argument: %s", argv[i]);
           usage(argc, argv);
           return 0;
       }
    }
    if (rti.base.number_of_scheduling_nodes == 0) {
        lf_print_error("--number_of_federates needs a valid positive integer argument.");
        usage(argc, argv);
        return 0;
    }
    return 1;
}
int main(int argc, const char* argv[]) {
    initialize_lf_thread_id();
    initialize_RTI(&rti);

    // Catch the Ctrl-C signal, for a clean exit that does not lose the trace information
    signal(SIGINT, exit);
#ifdef SIGPIPE
    // Ignore SIGPIPE errors, which terminate the entire application if
    // socket write() fails because the reader has closed the socket.
    // Instead, cause an EPIPE error to be set when write() fails.
    // NOTE: The reason for a broken socket causing a SIGPIPE signal
    // instead of just having write() return an error is to robutly
    // a foo | bar pipeline where bar crashes. The default behavior
    // is for foo to also exit.
    signal(SIGPIPE, SIG_IGN);
#endif // SIGPIPE
    if (atexit(termination) != 0) {
        lf_print_warning("Failed to register termination function!");
    }

    if (!process_args(argc, argv)) {
        // Processing command-line arguments failed.
        return -1;
    }

    if (rti.base.tracing_enabled) {
        _lf_number_of_workers = rti.base.number_of_scheduling_nodes;
        // One thread communicating to each federate. Add 1 for 1 ephemeral
        // timeout thread for each federate (this should be created only once
        // per federate because shutdown only occurs once). Add 1 for the clock
        // sync thread. Add 1 for the thread that responds to erroneous
        // connections attempted after initialization phase has completed. Add 1
        // for the main thread.
        lf_tracing_global_init("rti", -1, _lf_number_of_workers * 2 + 3);
        lf_print("Tracing the RTI execution in %s file.", rti_trace_file_name);
    }

    lf_print("Starting RTI for %d federates in federation ID %s.",  rti.base.number_of_scheduling_nodes, rti.federation_id);
    assert(rti.base.number_of_scheduling_nodes < UINT16_MAX);
    
    // Allocate memory for the federates
    rti.base.scheduling_nodes = (scheduling_node_t**)calloc(rti.base.number_of_scheduling_nodes, sizeof(scheduling_node_t*));
    for (uint16_t i = 0; i < rti.base.number_of_scheduling_nodes; i++) {
        federate_info_t *fed_info = (federate_info_t *) calloc(1, sizeof(federate_info_t));
        initialize_federate(fed_info, i);
        rti.base.scheduling_nodes[i] = (scheduling_node_t *) fed_info;
    }

    int socket_descriptor = start_rti_server(rti.user_specified_port);
    if (socket_descriptor >= 0) {
        wait_for_federates(socket_descriptor);
        normal_termination = true;
        if (rti.base.tracing_enabled) {
            // No need for a mutex lock because all threads have exited.
            lf_tracing_global_shutdown();
            lf_print("RTI trace file saved.");
        }
    }

    lf_print("RTI is exiting."); // Do this before freeing scheduling nodes.
    free_scheduling_nodes(rti.base.scheduling_nodes, rti.base.number_of_scheduling_nodes);

    // Even if the RTI is exiting normally, it should report an error code if one of the
    // federates has reported an error.
    return (int)_lf_federate_reports_error;
}
#endif // STANDALONE_RTI

