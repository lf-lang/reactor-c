/**
 * @file
 * @author Edward A. Lee
 * @author Ravi Akella
 *
 * @brief Standalone program to send a Lingua Franca trace file to InfluxDB.
 *
 * InfluxDB is a database server to which data can be posted using HTTP
 * or sent as a UDP datagram.
 *
 * ## Compiling this Program
 *
 * To compile this program, simply do this in this source directory:
 * ```
 *    sudo make install
 * ```
 * This will place an executable program `trace_to_influxdb` in the directory `usr/local/bin`.
 *
 * ## Setting up InfluxDB
 *
 * To set up InfluxDB, see:
 *
 * [https://docs.influxdata.com/influxdb/v2.0/get-started/](https://docs.influxdata.com/influxdb/v2.0/get-started/)
 *
 * If you have previously installed InfluxDB and you want a fresh start, do this:
 * ```shell
 *     rm -rf ~/.influxdbv2/
 *     ps aux | grep nflux
 * ```
 * The second command will report any InfluxDB processes that are running. Kill them with
 * ```shell
 *     kill -9 PID
 * ```
 * where 'PID' is replaced with whatever process ID(s) are reported by the `ps` command above.
 *
 * To start an InfluxDB server on localhost with port 8087:
 * ```shell
 *    influxd --http-bind-address :8087 --reporting-disabled
 * ```
 * The 'reporting-disabled' option simply disables notifications to the InfluxDB mother ship.
 *
 * You then need to set up at least one user, organization, and bucket. You can do this by pointing your browser to
 * ```
 *     http://localhost:8087
 * ```
 * The browser will walk you through the process of creating a user, password, organization, and initial bucket. E.g.:
 * ```
 *     User: eal superSecretPassword
 *     Organization: iCyPhy
 *     Bucket: test
 * ```
 * The UI in the browser will then give you the options Quick Start or Advanced, either of which you can select.
 * If you select "Data" on the left, you can browse Buckets to verify that your test bucket was created.
 *
 * ## Uploading Trace Data to InfluxDB
 *
 * First, generate a trace file by setting a target parameter in a Lingua Franca program:
 * ```
 *     target C {
 *         tracing: true
 *     };
 * ```
 * Then, when you run this program, a binary file with extension `.lft` will be created.
 *
 * In your browser, in the InfluxDB UI, select Data on the left, then select the Tokens tab.
 * Select a token and copy the token string to clipboard. It will looks something like this:
 * ```
 *     N1mK4b7z29YuWrWG_rBRJF3owaXjPA6gBVOgGG3eStS_zbESHTYJgfJWHB2JA_y3-BMYlMPVa05ccLVA1S770A==
 * ```
 * Then, invoke the conversion program as follows:
 * ```shell
 *     trace_to_influxdb Filename.lft \
 *         --token N1mK4b7z29YuWrWG_rBRJF3owaXjPA6gBVOgGG3eStS_zbESHTYJgfJWHB2JA_y3-BMYlMPVa05ccLVA1S770A==
 * ```
 * where 'Filename' and the token are replaced with your values.
 * This will upload the trace data to InfluxDB.
 *
 * You can also specify the following command-line options:
 * * -h, --host: The host name running InfluxDB. If not given, this defaults to "localhost".
 * * -p, --port: The port for accessing InfluxDB. This defaults to 8086. If you used 8087, as shown above, then you have
to give this option.
 *
 * The data can then be viewed in the InfluxDB browser, or you can configure an external
 * tool such as Grafana to visualize it (see https://grafana.com/docs/grafana/latest/datasources/influxdb/).
 */
#define LF_TRACE
#include <stdio.h>
#include "reactor.h"
#include "trace.h"
#include "trace_util.h"
#include "influxdb.h"

#define MAX_NUM_REACTIONS 64 // Maximum number of reactions reported in summary stats.
#define MAX_NUM_WORKERS 64

/** File containing the trace binary data. */
FILE* trace_file = NULL;

/** Struct identifying the influx client. */
influx_client_t influx_client;
influx_v2_client_t influx_v2_client;
/**
 * Print a usage message.
 */
void usage() {
  printf("\nUsage: trace_to_influxdb [options] trace_file [options]\n\n");
  printf("\nOptions: \n\n");
  printf("   -t, --token TOKEN\n");
  printf("   The token for access to InfluxDB (required argument).\n\n");
  printf("   -h, --host HOSTNAME\n");
  printf("   The host name for access to InfluxDB (default is 'localhost').\n\n");
  printf("   -p, --port PORT\n");
  printf("   The port for access to InfluxDB (default is 8086).\n\n");
  printf("   -o, --ort ORGANIZATION\n");
  printf("   The organization for access to InfluxDB (default is 'iCyPhy').\n\n");
  printf("   -b, --bucket BUCKET\n");
  printf("   The bucket into which to put the data (default is 'test').\n\n");
  printf("\n\n");
}

/** Largest timestamp seen. */
instant_t latest_time = 0LL;

/**
 * Read a trace in the trace_file and write it to InfluxDB.
 * @return The number of records read or 0 upon seeing an EOF.
 */
size_t read_and_write_trace() {
  int trace_length = read_trace();
  if (trace_length == 0)
    return 0;
  // Write each line.
  for (int i = 0; i < trace_length; i++) {

    // Ignore federated traces.
    if (trace[i].event_type > federated)
      continue;

    char* reaction_name = "none";
    if (trace[i].dst_id >= 0) {
      reaction_name = (char*)malloc(4);
      snprintf(reaction_name, 4, "%d", trace[i].dst_id);
    }
    // printf("DEBUG: reactor self struct pointer: %p\n", trace[i].pointer);
    int object_instance = -1;
    char* reactor_name = get_object_description(trace[i].pointer, &object_instance);
    if (reactor_name == NULL) {
      reactor_name = "NO REACTOR";
    }
    int trigger_instance = -1;
    char* trigger_name = get_trigger_name(trace[i].trigger, &trigger_instance);
    if (trigger_name == NULL) {
      trigger_name = "NO TRIGGER";
    }
    // FIXME: Treating physical time as the timestamp.
    // Do we want this to optionally be logical time?
    // FIXME: What is the difference between a TAG and F_STR (presumably, Field String)?
    // Presumably, the HTTP post is formatted as a "line protocol" command. See:
    // https://docs.influxdata.com/influxdb/v2.0/reference/syntax/line-protocol/
    int response_code =
        post_curl(&influx_v2_client, INFLUX_MEAS(trace_event_names[trace[i].event_type]),
                  INFLUX_TAG("Reactor", reactor_name), INFLUX_TAG("Reaction", reaction_name),
                  INFLUX_F_INT("Worker", trace[i].src_id), INFLUX_F_INT("Logical Time", trace[i].logical_time),
                  INFLUX_F_INT("Microstep", trace[i].microstep), INFLUX_F_STR("Trigger Name", trigger_name),
                  INFLUX_F_INT("Extra Delay", trace[i].extra_delay), INFLUX_TS(trace[i].physical_time), INFLUX_END);
    if (response_code != 0) {
      fprintf(stderr, "****** response code: %d\n", response_code);
      return 0;
    }
  }
  return trace_length;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    usage();
    exit(0);
  }
  // Defaults.
  influx_v2_client.token = NULL;
  influx_v2_client.host = "localhost";
  influx_v2_client.port = 8086;
  influx_v2_client.org = "iCyPhy";
  influx_v2_client.bucket = "test";

  char* filename = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp("-t", argv[i]) == 0 || strcmp("--token", argv[i]) == 0) {
      if (i++ == argc - 1) {
        usage();
        fprintf(stderr, "No token specified.\n");
        exit(1);
      }
      influx_v2_client.token = argv[i];
    } else if (strcmp("-h", argv[i]) == 0 || strcmp("--host", argv[i]) == 0) {
      if (i++ == argc - 1) {
        usage();
        fprintf(stderr, "No host specified.\n");
        exit(1);
      }
      influx_v2_client.host = argv[i];
    } else if (strcmp("-p", argv[i]) == 0 || strcmp("--port", argv[i]) == 0) {
      if (i++ == argc - 1) {
        usage();
        fprintf(stderr, "No port specified.\n");
        exit(1);
      }
      influx_v2_client.port = atoi(argv[i]);
      if (influx_v2_client.port == 0) {
        fprintf(stderr, "Invalid port: %s.\n", argv[i]);
      }
    } else if (strcmp("-o", argv[i]) == 0 || strcmp("--org", argv[i]) == 0) {
      if (i++ == argc - 1) {
        usage();
        fprintf(stderr, "No organization specified.\n");
        exit(1);
      }
      influx_v2_client.org = argv[i];
    } else if (strcmp("-b", argv[i]) == 0 || strcmp("--bucket", argv[i]) == 0) {
      if (i++ == argc - 1) {
        usage();
        fprintf(stderr, "No bucket specified.\n");
        exit(1);
      }
      influx_v2_client.bucket = argv[i];
    } else {
      // Must be the filename.
      filename = argv[i];
    }
  }
  if (influx_v2_client.token == NULL) {
    fprintf(stderr, "No token specified.\n");
    exit(1);
  }
  if (filename == NULL) {
    fprintf(stderr, "No trace file specified.\n");
    exit(1);
  }

  // Open the trace file.
  trace_file = open_file(filename, "r");

  if (read_header() >= 0) {
    size_t num_records = 0, result;
    while ((result = read_and_write_trace()) != 0) {
      num_records = result;
    };
    printf("***** %zu records written to InfluxDB.\n", num_records);
    // File closing is handled by termination function.
  }
}
