/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief Utility functions for clock synchronization.
 */

#ifdef FEDERATED
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "low_level_platform.h"
#include "clock-sync.h"
#include "net_common.h"
#include "net_util.h"
#include "socket_common.h"
#include "util.h"

/** Offset calculated by the clock synchronization algorithm. */
interval_t _lf_clock_sync_offset = NSEC(0);
/** Offset used to test clock synchronization (clock sync should largely remove this offset). */
interval_t _lf_clock_sync_constant_bias = NSEC(0);

/**
 * Keep a record of connection statistics
 * and the remote physical clock of the RTI.
 */
socket_stat_t _lf_rti_socket_stat = {.remote_physical_clock_snapshot_T1 = NEVER,
                                     .local_physical_clock_snapshot_T2 = NEVER,
                                     .local_delay = 0LL,
                                     .received_T4_messages_in_current_sync_window = 0,
                                     .history = 0LL,
                                     .network_stat_round_trip_delay_max = 0LL,
                                     .network_stat_sample_index = 0,
                                     .clock_synchronization_error_bound = 0LL};

/**
 * Records the physical time at which the clock of this federate was
 * synchronized with the RTI. Used to calculate the drift.
 */
instant_t _lf_last_clock_sync_instant = 0LL;

/**
 * The UDP socket descriptor for this federate to communicate with the RTI.
 * This is set by setup_clock_synchronization_with_rti() in lf_connect_to_rti()
 * in federate.c, which must be called before other
 * functions that communicate with the rti are called.
 */
int _lf_rti_socket_UDP = -1;

/**
 * Atomically add an adjustment to the clock sync offset.
 * This needs to be atomic to be thread safe, particularly on 32-bit platforms.
 */
static void adjust_lf_clock_sync_offset(interval_t adjustment) {
  lf_atomic_fetch_add64(&_lf_clock_sync_offset, adjustment);
}

#ifdef _LF_CLOCK_SYNC_COLLECT_STATS

#include <math.h> // For sqrtl()

/**
 * Update statistic on the socket based on the newly calculated network delay
 * and clock synchronization error
 *
 * @param socket_stat The socket_stat_t struct that  keeps track of stats for a given connection
 * @param network_round_trip_delay The newly calculated round trip delay to the remote federate/RTI
 * @param clock_synchronization_error The newly calculated clock synchronization error relative to
 *  the remote federate/RTI
 */
static void update_socket_stat(socket_stat_t* socket_stat, long long network_round_trip_delay,
                               long long clock_synchronization_error) {
  // Add the data point
  socket_stat->network_stat_samples[socket_stat->network_stat_sample_index] = network_round_trip_delay;
  socket_stat->network_stat_sample_index++;

  // Calculate maximums
  if (socket_stat->network_stat_round_trip_delay_max < network_round_trip_delay) {
    socket_stat->network_stat_round_trip_delay_max = network_round_trip_delay;
  }

  if (socket_stat->clock_synchronization_error_bound < clock_synchronization_error) {
    socket_stat->clock_synchronization_error_bound = clock_synchronization_error;
  }
}

/**
 * Calculate statistics of the socket.
 * The releavent information is returned as a lf_stat struct.
 *
 * @param socket_stat The socket_stat_t struct that  keeps track of stats for a given connection
 */
static lf_stat_ll calculate_socket_stat(struct socket_stat_t* socket_stat) {
  // Initialize the stat struct
  lf_stat_ll stats = {0, 0, 0, 0};
  // Calculate the average and max
  for (int i = 0; i < socket_stat->network_stat_sample_index; i++) {
    if (socket_stat->network_stat_samples[i] > stats.max) {
      stats.max = socket_stat->network_stat_samples[i];
    }
    stats.average += socket_stat->network_stat_samples[i] / socket_stat->network_stat_sample_index;
  }
  for (int i = 0; i < socket_stat->network_stat_sample_index; i++) {
    long long delta = socket_stat->network_stat_samples[i] - stats.average;
    stats.variance += powl(delta, 2);
  }
  stats.variance /= socket_stat->network_stat_sample_index;
  stats.standard_deviation = sqrtl(stats.variance);

  return stats;
}
#endif // _LF_CLOCK_SYNC_COLLECT_STATS

void reset_socket_stat(struct socket_stat_t* socket_stat) {
  socket_stat->received_T4_messages_in_current_sync_window = 0;
  socket_stat->history = 0LL;
  socket_stat->network_stat_sample_index = 0;
}

uint16_t setup_clock_synchronization_with_rti() {
  uint16_t port_to_return = UINT16_MAX; // Default if clock sync is off.
#if (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_ON)
  // Initialize the UDP socket
  _lf_rti_socket_UDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  // Initialize the necessary information for the UDP address
  struct sockaddr_in federate_UDP_addr;
  federate_UDP_addr.sin_family = AF_INET;
  federate_UDP_addr.sin_port = htons(0u); // Port 0 indicates to bind that
                                          // it can assign any port to this
                                          // socket. This is okay because
                                          // the port number is then sent
                                          // to the RTI.
  federate_UDP_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(_lf_rti_socket_UDP, (struct sockaddr*)&federate_UDP_addr, sizeof(federate_UDP_addr)) < 0) {
    lf_print_error_system_failure("Failed to bind its UDP socket.");
  }
  // Retrieve the port number that was assigned by the operating system
  socklen_t addr_length = sizeof(federate_UDP_addr);
  if (getsockname(_lf_rti_socket_UDP, (struct sockaddr*)&federate_UDP_addr, &addr_length) == -1) {
    // FIXME: Send 0 UDP_PORT message instead of exiting.
    // That will disable clock synchronization.
    lf_print_error_system_failure("Failed to retrieve UDP port.");
  }
  LF_PRINT_DEBUG("Assigned UDP port number %u to its socket.", ntohs(federate_UDP_addr.sin_port));

  port_to_return = ntohs(federate_UDP_addr.sin_port);

  // Set the option for this socket to reuse the same address
  int option_value = 1;
  if (setsockopt(_lf_rti_socket_UDP, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) < 0) {
    lf_print_error("Failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
  }
  // Set the timeout on the UDP socket so that read and write operations don't block for too long
  struct timeval timeout_time = {.tv_sec = UDP_TIMEOUT_TIME / BILLION, .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000};
  if (setsockopt(_lf_rti_socket_UDP, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
    lf_print_error("Failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
  }
  if (setsockopt(_lf_rti_socket_UDP, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
    lf_print_error("Failed to set SO_SNDTIMEO option on the socket: %s.", strerror(errno));
  }
#elif (LF_CLOCK_SYNC == LF_CLOCK_SYNC_INIT)
  port_to_return = 0u;
#endif // (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_ON)
  return port_to_return;
}

void synchronize_initial_physical_clock_with_rti(int* rti_socket_TCP) {
  LF_PRINT_DEBUG("Waiting for initial clock synchronization messages from the RTI.");

  size_t message_size = 1 + sizeof(instant_t);
  unsigned char buffer[message_size];

  for (int i = 0; i < _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL; i++) {
    // The first message expected from the RTI is MSG_TYPE_CLOCK_SYNC_T1
    read_from_socket_fail_on_error(rti_socket_TCP, message_size, buffer, NULL,
                                   "Federate %d did not get the initial clock synchronization message T1 from the RTI.",
                                   _lf_my_fed_id);

    // Get local physical time before doing anything else.
    instant_t receive_time = lf_time_physical();

    // Check that this is the T1 message.
    if (buffer[0] != MSG_TYPE_CLOCK_SYNC_T1) {
      lf_print_error_and_exit("Initial clock sync: Expected T1 message from RTI. Got %x.", buffer[0]);
    }
    // Handle the message and send a reply T3 message.
    // NOTE: No need to acquire the mutex lock during initialization because only
    // one thread is running.
    if (handle_T1_clock_sync_message(buffer, *rti_socket_TCP, receive_time) != 0) {
      lf_print_error_and_exit("Initial clock sync: Failed to send T3 reply to RTI.");
    }

    // Next message from the RTI is required to be MSG_TYPE_CLOCK_SYNC_T4
    read_from_socket_fail_on_error(rti_socket_TCP, message_size, buffer, NULL,
                                   "Federate %d did not get the clock synchronization message T4 from the RTI.",
                                   _lf_my_fed_id);

    // Check that this is the T4 message.
    if (buffer[0] != MSG_TYPE_CLOCK_SYNC_T4) {
      lf_print_error_and_exit("Federate %d expected T4 message from RTI. Got %x.", _lf_my_fed_id, buffer[0]);
    }

    // Handle the message.
    handle_T4_clock_sync_message(buffer, *rti_socket_TCP, receive_time);
  }

  LF_PRINT_LOG("Finished initial clock synchronization with the RTI.");
}

int handle_T1_clock_sync_message(unsigned char* buffer, int socket, instant_t t2) {
  // Extract the payload
  instant_t t1 = extract_int64(&(buffer[1]));

  LF_PRINT_DEBUG("Received T1 message with time payload " PRINTF_TIME " from RTI at local time " PRINTF_TIME ".", t1,
                 t2);

  // Store snapshots of remote (master) and local physical clock
  _lf_rti_socket_stat.remote_physical_clock_snapshot_T1 = t1;
  _lf_rti_socket_stat.local_physical_clock_snapshot_T2 = t2;
  // Send a message to the RTI and calculate the local delay
  // T3-T2 between receiving the T1 message and replying.

  // Reply will have the federate ID as a payload.
  unsigned char reply_buffer[1 + sizeof(uint16_t)];
  reply_buffer[0] = MSG_TYPE_CLOCK_SYNC_T3;
  encode_uint16(_lf_my_fed_id, &(reply_buffer[1]));

  // Write the reply to the socket.
  LF_PRINT_DEBUG("Sending T3 message to RTI.");
  if (write_to_socket(socket, 1 + sizeof(uint16_t), reply_buffer)) {
    lf_print_error("Clock sync: Failed to send T3 message to RTI.");
    return -1;
  }

  // Measure the time _after_ the write on the assumption that the read
  // from the socket, which occurs before this function is called, takes
  // about the same amount of time as the write of the reply.
  _lf_rti_socket_stat.local_delay = lf_time_physical() - t2;
  return 0;
}

void handle_T4_clock_sync_message(unsigned char* buffer, int socket, instant_t r4) {
  // Increment the number of received T4 messages
  _lf_rti_socket_stat.received_T4_messages_in_current_sync_window++;

  // Extract the payload
  instant_t t4 = extract_int64(&(buffer[1]));

  LF_PRINT_DEBUG("Clock sync: Received T4 message with time payload " PRINTF_TIME " from RTI at local time " PRINTF_TIME
                 ". "
                 "(difference " PRINTF_TIME ")",
                 t4, r4, r4 - t4);

  // Calculate the round trip delay from T1 to T4:
  // (T4 - T1) - (T3 - T2)
  interval_t network_round_trip_delay =
      (t4 - _lf_rti_socket_stat.remote_physical_clock_snapshot_T1) - _lf_rti_socket_stat.local_delay;

  // Estimate the clock synchronization error based on the assumption
  // that the channel delay is symmetric:
  // one_way_channel_delay - (T2 - T1).
  // This number is positive if the clock at the federate (T2) is
  // behind the clock at the RTI (T1).
  interval_t estimated_clock_error =
      network_round_trip_delay / 2 -
      (_lf_rti_socket_stat.local_physical_clock_snapshot_T2 - _lf_rti_socket_stat.remote_physical_clock_snapshot_T1);
  LF_PRINT_DEBUG("Clock sync: Estimated clock error: " PRINTF_TIME ".", estimated_clock_error);

  // The adjustment to the clock offset (to be calculated)
  interval_t adjustment = 0;
  // If the socket is _lf_rti_socket_UDP, then
  // after sending T4, the RTI sends a "coded probe" message,
  // which can be used to filter out noise.
  if (socket == _lf_rti_socket_UDP) {
    // Read the coded probe message.
    // We can reuse the same buffer.
    int read_failed = read_from_socket(socket, 1 + sizeof(instant_t), buffer);

    instant_t r5 = lf_time_physical();

    if (read_failed || buffer[0] != MSG_TYPE_CLOCK_SYNC_CODED_PROBE) {
      lf_print_warning("Clock sync: Did not get the expected coded probe message from the RTI. "
                       "Skipping clock synchronization round.");
      return;
    }
    // Filter out noise.
    instant_t t5 = extract_int64(&(buffer[1])); // Time at the RTI of sending the coded probe.

    // Compare the difference in time at the RTI between sending T4 and the coded probe
    // against the difference in time at this federate of receiving these two message.
    interval_t coded_probe_distance = llabs((r5 - r4) - (t5 - t4));

    LF_PRINT_DEBUG("Clock sync: Received code probe that reveals a time discrepancy between "
                   "messages of " PRINTF_TIME ".",
                   coded_probe_distance);

    // Check against the guard band.
    if (coded_probe_distance >= CLOCK_SYNC_GUARD_BAND) {
      // Discard this clock sync cycle
      LF_PRINT_LOG("Clock sync: Skipping the current clock synchronization cycle "
                   "due to impure coded probes.");
      LF_PRINT_LOG("Clock sync: Coded probe packet stats: "
                   "Distance: " PRINTF_TIME ". r5 - r4 = " PRINTF_TIME ". t5 - t4 = " PRINTF_TIME ".",
                   coded_probe_distance, r5 - r4, t5 - t4);
      _lf_rti_socket_stat.received_T4_messages_in_current_sync_window--;
      return;
    }
    // Apply a jitter attenuator to the estimated clock error to prevent
    // large jumps in the underlying clock.
    // Note that estimated_clock_error is calculated using lf_time_physical() which includes
    // the clock sync adjustment.
    adjustment = estimated_clock_error / _LF_CLOCK_SYNC_ATTENUATION;
  } else {
    // Use of TCP socket means we are in the startup phase, so
    // rather than adjust the clock offset, we simply set it to the
    // estimated error.
    adjustment = estimated_clock_error;
  }

#ifdef _LF_CLOCK_SYNC_COLLECT_STATS // Enabled by default
  // Update RTI's socket stats
  update_socket_stat(&_lf_rti_socket_stat, network_round_trip_delay, estimated_clock_error);
#endif // _LF_CLOCK_SYNC_COLLECT_STATS

  // FIXME: Enable alternative regression mechanism here.
  LF_PRINT_DEBUG("Clock sync: Adjusting clock offset running average by " PRINTF_TIME ".",
                 adjustment / _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL);
  // Calculate the running average
  _lf_rti_socket_stat.history += adjustment / _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL;

  if (_lf_rti_socket_stat.received_T4_messages_in_current_sync_window >= _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL) {

    lf_stat_ll stats = {0, 0, 0, 0};
#ifdef _LF_CLOCK_SYNC_COLLECT_STATS // Enabled by default
    stats = calculate_socket_stat(&_lf_rti_socket_stat);
    // Issue a warning if standard deviation is high in data
    if (stats.standard_deviation >= CLOCK_SYNC_GUARD_BAND) {
      // Reset the stats
      LF_PRINT_LOG("Clock sync: Large standard deviation detected in network delays (" PRINTF_TIME
                   ") for the current period."
                   " Clock synchronization offset might not be accurate.",
                   stats.standard_deviation);
      reset_socket_stat(&_lf_rti_socket_stat);
      return;
    }
#endif // _LF_CLOCK_SYNC_COLLECT_STATS
    // The number of received T4 messages has reached _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL
    // which means we can now adjust the clock offset.
    // For the AVG algorithm, history is a running average and can be directly
    // applied.
    adjust_lf_clock_sync_offset(_lf_rti_socket_stat.history);
    // @note AVG and SD will be zero if _LF_CLOCK_SYNC_COLLECT_STATS is set to false
    LF_PRINT_LOG("Clock sync:"
                 " New offset: " PRINTF_TIME "."
                 " Round trip delay to RTI (now): " PRINTF_TIME "."
                 " (AVG): " PRINTF_TIME "."
                 " (SD): " PRINTF_TIME "."
                 " Local round trip delay: " PRINTF_TIME ".",
                 _lf_clock_sync_offset, network_round_trip_delay, stats.average, stats.standard_deviation,
                 _lf_rti_socket_stat.local_delay);
    // Reset the stats
    reset_socket_stat(&_lf_rti_socket_stat);
    // Set the last instant at which the clocks were synchronized
    _lf_last_clock_sync_instant = r4;
  }
}

#if (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_ON)
/**
 * Thread that listens for UDP inputs from the RTI.
 */
static void* listen_to_rti_UDP_thread(void* args) {
  (void)args;
  initialize_lf_thread_id();
  // Listen for UDP messages from the RTI.
  // The only expected messages are T1 and T4, which have
  // a payload of a time value.
  size_t message_size = 1 + sizeof(instant_t);
  unsigned char buffer[message_size];
  // This thread will be either waiting for T1 or waiting
  // for T4. Track the mode with this variable:
  bool waiting_for_T1 = true;
  // Even though UDP messages are connectionless, we need to call connect()
  // at least once to record the address of the RTI's UDP port. The RTI
  // uses bind() to reserve that address, so recording it once is sufficient.
  bool connected = false;
  while (1) {
    struct sockaddr_in RTI_UDP_addr;
    socklen_t RTI_UDP_addr_length = sizeof(RTI_UDP_addr);
    ssize_t bytes_read = 0;
    // Read from the UDP socket
    do {
      ssize_t bytes = recvfrom(_lf_rti_socket_UDP,                // The UDP socket
                               &buffer[bytes_read],               // The buffer to read into
                               message_size - (size_t)bytes_read, // Number of bytes to read
                               MSG_WAITALL,                       // Read the entire datagram
                               (struct sockaddr*)&RTI_UDP_addr,   // Record the RTI's address
                               &RTI_UDP_addr_length);             // The RTI's address length
      // Try reading again if errno indicates the need to try again and there are more
      // bytes to read.
      if (bytes > 0) {
        bytes_read += bytes;
      }
    } while ((errno == EAGAIN || errno == EWOULDBLOCK) && bytes_read < (ssize_t)message_size);

    // Get local physical time before doing anything else.
    instant_t receive_time = lf_time_physical();

    if (bytes_read < (ssize_t)message_size) {
      // Either the socket has closed or the RTI has sent EOF.
      // Exit the thread to halt clock synchronization.
      lf_print_error("Clock sync: UDP socket to RTI is broken: %s. Clock sync is now disabled.", strerror(errno));
      break;
    }
    LF_PRINT_DEBUG("Clock sync: Received UDP message %u from RTI on port %u.", buffer[0], ntohs(RTI_UDP_addr.sin_port));

    // Handle the message
    if (waiting_for_T1) {
      if (buffer[0] == MSG_TYPE_CLOCK_SYNC_T1) {
        waiting_for_T1 = false;
        // The reply (or return) address is given in RTI_UDP_addr.
        // We utilize the connect() function to set the default address
        // of the _lf_rti_socket_UDP socket to RTI_UDP_addr. This is convenient
        // because subsequent calls to write_to_socket do not need this address.
        // Note that this only needs to be done for handle_T1_clock_sync_message()
        // because it is the only function that needs to reply to the RTI.
        if (!connected && connect(_lf_rti_socket_UDP, (struct sockaddr*)&RTI_UDP_addr, RTI_UDP_addr_length) < 0) {
          lf_print_error("Clock sync: Federate %d failed to register RTI's UDP reply address. "
                         "Clock synchronization has stopped.",
                         _lf_my_fed_id);
          break;
        }
        connected = true;
        if (handle_T1_clock_sync_message(buffer, _lf_rti_socket_UDP, receive_time) != 0) {
          // Failed to send T3 reply. Wait for the next T1.
          waiting_for_T1 = true;
          continue;
        }
      } else {
        // Waiting for a T1 message, but received something else. Discard message.
        lf_print_warning("Clock sync: Received %u message from RTI, but waiting for %u (T1). "
                         "Discarding the message.",
                         buffer[0], MSG_TYPE_CLOCK_SYNC_T1);
        continue;
      }
    } else if (buffer[0] == MSG_TYPE_CLOCK_SYNC_T4) {
      handle_T4_clock_sync_message(buffer, _lf_rti_socket_UDP, receive_time);
      waiting_for_T1 = true;
    } else {
      lf_print_warning("Clock sync: Received from RTI an unexpected UDP message type: %u. "
                       "Discarding the message and skipping this round.",
                       buffer[0]);
      // Ignore further clock sync messages until we get a T1.
      waiting_for_T1 = true;
    }
  }
  return NULL;
}
#endif // (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_ON)

// If clock synchronization is enabled, provide implementations. If not
// just empty implementations that should be optimized away.
#if (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_INIT)
void clock_sync_add_offset(instant_t* t) {
  *t = lf_time_add(*t, (_lf_clock_sync_offset + _lf_clock_sync_constant_bias));
}

void clock_sync_subtract_offset(instant_t* t) {
  *t = lf_time_add(*t, -(_lf_clock_sync_offset + _lf_clock_sync_constant_bias));
}

void clock_sync_set_constant_bias(interval_t offset) { _lf_clock_sync_constant_bias = offset; }
#else  // i.e. (LF_CLOCK_SYNC < LF_CLOCK_SYNC_INIT)
void clock_sync_add_offset(instant_t* t) { (void)t; }
void clock_sync_subtract_offset(instant_t* t) { (void)t; }
void clock_sync_set_constant_bias(interval_t offset) { (void)offset; }
#endif // (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_INIT)

int create_clock_sync_thread(lf_thread_t* thread_id) {
#if (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_ON)
  // One for UDP messages if clock synchronization is enabled for this federate
  return lf_thread_create(thread_id, listen_to_rti_UDP_thread, NULL);
#else  // i.e. (LF_CLOCK_SYNC < LF_CLOCK_SYNC_ON)
  (void)thread_id; // Suppress unused parameter warning.
#endif // (LF_CLOCK_SYNC >= LF_CLOCK_SYNC_ON)
  return 0;
}

#endif // FEDERATED
