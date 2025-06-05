/**
 * @file clock-sync.h
 * @brief Utility functions for clock synchronization in federated Lingua Franca programs.
 * @ingroup Federated
 *
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni (soroush@utdallas.edu)
 *
 * This file provides utility functions and data structures for synchronizing physical clocks
 * between federates and the RTI in federated Lingua Franca programs. It implements a variant
 * of the Precision Time Protocol (PTP) for clock synchronization, with support for both
 * initial synchronization and runtime synchronization. The file includes functions for
 * handling clock synchronization messages, managing synchronization statistics, and adjusting
 * clock offsets.
 */

#ifndef CLOCK_SYNC_H
#define CLOCK_SYNC_H

#include "low_level_platform.h"

#ifndef LF_CLOCK_SYNC
/**
 * @brief Clock synchronization mode.
 * @ingroup Federated
 *
 * This is one of LF_CLOCK_SYNC_OFF, LF_CLOCK_SYNC_INIT, or LF_CLOCK_SYNC_ON.
 * The default is LF_CLOCK_SYNC_INIT, which indicates that clock synchronization
 * is performed only at initialization.
 * @note This is a compile-time option.
 */
#define LF_CLOCK_SYNC LF_CLOCK_SYNC_INIT
#endif

/**
 * @brief Indicator for clock synchronization to be turned off altogether.
 * @ingroup Federated
 */
#define LF_CLOCK_SYNC_OFF 1

/**
 * @brief Indicator for clock synchronization to be turned on at initialization.
 * @ingroup Federated
 */
#define LF_CLOCK_SYNC_INIT 2

/**
 * @brief Indicator for clock synchronization to be turned on at initialization and runtime.
 * @ingroup Federated
 */
#define LF_CLOCK_SYNC_ON 3

/**
 * @brief Number of required clock sync T4 messages per synchronization interval.
 * @ingroup Federated
 *
 * The offset to the clock will not be adjusted until this number of T4 clock
 * synchronization messages have been received.
 */
#ifndef _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL
#define _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL 10
#endif

/**
 * @brief Runtime clock offset updates will be divided by this number.
 * @ingroup Federated
 */
#ifndef _LF_CLOCK_SYNC_ATTENUATION
#define _LF_CLOCK_SYNC_ATTENUATION 10
#endif

/**
 * @brief By default, collect statistics on clock synchronization.
 * @ingroup Federated
 */
#ifndef _LF_CLOCK_SYNC_COLLECT_STATS
#define _LF_CLOCK_SYNC_COLLECT_STATS true
#endif

/**
 * @brief Define a guard band to filter clock synchronization messages based on
 * discrepancies in the network delay.
 * @ingroup Federated
 *
 * @see Coded probes in Geng, Yilong, et al.
 * "Exploiting a natural network effect for scalable, fine-grained clock
 * synchronization."
 */
#define CLOCK_SYNC_GUARD_BAND USEC(100)

/**
 * @brief Statistics and state for clock synchronization over a socket connection.
 * @ingroup Federated
 *
 * This struct maintains the state and statistics needed for clock synchronization
 * between a federate and the RTI using a variant of the Precision Time Protocol (PTP).
 * The synchronization process involves four timestamps (T1-T4) to estimate network
 * delays and clock offsets:
 *
 * 1. T1: RTI's physical time when sending sync message
 * 2. T2: Federate's physical time when receiving T1
 * 3. T3: Federate's physical time when sending reply
 * 4. T4: RTI's physical time when receiving reply
 *
 * The round trip delay is estimated as: (T4 - T1) - (T3 - T2)
 * The clock offset can be estimated as: ((T2 - T1) + (T3 - T4)) / 2
 */
typedef struct socket_stat_t {
  /**
   * @brief Remote (RTI) physical time when sending sync message (T1).
   *
   * This is the first timestamp in the PTP exchange, recorded by the RTI
   * when it initiates a clock synchronization round. Used to calculate
   * network delays and clock offsets.
   */
  instant_t remote_physical_clock_snapshot_T1;

  /**
   * @brief Local (federate) physical time when receiving T1 (T2).
   *
   * Recorded by the federate when it receives the RTI's sync message.
   * Used in conjunction with T1 to estimate the one-way network delay
   * and clock offset.
   */
  instant_t local_physical_clock_snapshot_T2;

  /**
   * @brief Estimated local processing delay (T3 - T2).
   *
   * Measures the time taken by the federate to process the sync message
   * and prepare a reply. This is subtracted from the total round-trip
   * time to get a more accurate network delay estimate.
   */
  interval_t local_delay;

  /**
   * @brief Counter for T4 messages received in current sync window.
   *
   * Tracks the number of T4 messages received in the current synchronization
   * interval. Must be reset to 0 when it reaches _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL.
   * Used to determine when enough samples have been collected to update
   * the clock offset.
   */
  int received_T4_messages_in_current_sync_window;

  /**
   * @brief Running history of clock synchronization data.
   *
   * Maintains a history of clock synchronization measurements.
   * For the AVG strategy, this stores a partially computed average
   * of the clock offset measurements.
   */
  interval_t history;

  /**
   * @brief Maximum observed round-trip network delay.
   *
   * Tracks the highest estimated delay between the local socket and
   * the remote socket. Used to establish bounds on clock synchronization
   * accuracy and to detect network anomalies.
   */
  interval_t network_stat_round_trip_delay_max;

  /**
   * @brief Current index in the network statistics samples array.
   *
   * Points to the next position in network_stat_samples where a new
   * measurement should be stored. Used to maintain a circular buffer
   * of recent network delay measurements.
   */
  int network_stat_sample_index;

  /**
   * @brief Bound on clock synchronization error.
   *
   * Represents the maximum expected difference between this federate's
   * clock and the remote clock. This bound is used to ensure the
   * reliability of clock synchronization and to detect potential
   * synchronization failures.
   */
  interval_t clock_synchronization_error_bound;

  /**
   * @brief Array of network delay samples.
   *
   * Stores the most recent _LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL
   * network delay measurements. These samples are used to calculate
   * statistics about network performance and to detect anomalies
   * in clock synchronization.
   *
   * @note This array must be the last field in the struct due to
   * C++ restrictions on designated initializers.
   */
  interval_t network_stat_samples[_LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL];
} socket_stat_t;

/**
 * @brief Holds generic statistical data
 * @ingroup Federated
 */
typedef struct lf_stat_ll {
  int64_t average;
  int64_t standard_deviation;
  int64_t variance;
  int64_t max;
} lf_stat_ll;

/**
 * @brief Reset statistics on the socket.
 * @ingroup Federated
 *
 * @param socket_stat The socket_stat_t struct that  keeps track of stats for a given connection
 */
void reset_socket_stat(struct socket_stat_t* socket_stat);

/**
 * @brief Setup necessary functionalities to synchronize clock with the RTI.
 * @ingroup Federated
 *
 * @return port number to be sent to the RTI.
 *  If clock synchronization is off compeltely, USHRT_MAX is returned.
 *  If clock synchronization is set to initial, 0 is sent.
 *  If clock synchronization is set to on, a reserved UDP port number will be sent.
 */
uint16_t setup_clock_synchronization_with_rti(void);

/**
 * @brief Synchronize the initial physical clock with the RTI.
 * @ingroup Federated
 *
 * A call to this function is inserted into the startup
 * sequence by the code generator if initial clock synchronization
 * is required.
 *
 * This is a blocking function that expects
 * to read a MSG_TYPE_CLOCK_SYNC_T1 from the RTI TCP socket.
 * It will then follow the PTP protocol to synchronize the local
 * physical clock with the RTI.
 * Failing to complete this protocol is treated as a catastrophic
 * error that causes the federate to exit.
 *
 * @param rti_socket_TCP Pointer to the RTI's socket
 */
void synchronize_initial_physical_clock_with_rti(int* rti_socket_TCP);

/**
 * @brief Handle a clock synchroninzation message T1 coming from the RTI.
 * @ingroup Federated
 *
 * T1 is the first message in a PTP exchange.
 * This replies to the RTI with a T3 message.
 * It also measures the time it takes between when the method is
 * called and the reply has been sent.
 * @param buffer The buffer containing the message, including the message type.
 * @param socket The socket (either _lf_rti_socket_TCP or _lf_rti_socket_UDP).
 * @param t2 The physical time at which the T1 message was received.
 * @return 0 if T3 reply is successfully sent, -1 otherwise.
 */
int handle_T1_clock_sync_message(unsigned char* buffer, int socket, instant_t t2);

/**
 * @brief Handle a clock synchronization message T4 coming from the RTI.
 * @ingroup Federated
 *
 * If the socket is _lf_rti_socket_TCP, then assume we are in the
 * initial clock synchronization phase and set the clock offset
 * based on the estimated clock synchronization error.
 * Otherwise, if the socket is _lf_rti_socket_UDP, then this looks also for a
 * subsequent "coded probe" message on the socket. If the delay between
 * the T4 and the coded probe message is not as expected, then reject
 * this clock synchronization round. If it is not rejected, then make
 * an adjustment to the clock offset based on the estimated error.
 * This function does not acquire the socket_mutex lock.
 * The caller should acquire it unless it is sure there is only one thread running.
 * @param buffer The buffer containing the message, including the message type.
 * @param socket The socket (either _lf_rti_socket_TCP or _lf_rti_socket_UDP).
 * @param r4 The physical time at which this T4 message was received.
 */
void handle_T4_clock_sync_message(unsigned char* buffer, int socket, instant_t r4);

/**
 * @brief Create the thread responsible for handling clock synchronization
 * with the RTI if (runtime) clock synchronization is on.
 * @ingroup Federated
 *
 * Otherwise, do nothing and return 0.
 *
 * @return On success, returns 0; On error, it returns an error number.
 */
int create_clock_sync_thread(lf_thread_t* thread_id);

/**
 * @brief Add the current clock synchronization offset to a specified timestamp.
 * @ingroup Federated
 *
 * @param t Pointer to the timestamp to which to add the offset.
 */
void clock_sync_add_offset(instant_t* t);

/**
 * @brief Subtract the clock synchronization offset from a timestamp.
 * @ingroup Federated
 *
 * @param t The timestamp from which to subtract the current clock sync offset.
 */
void clock_sync_subtract_offset(instant_t* t);

/**
 * @brief Set a fixed offset to the physical clock.
 * @ingroup Federated
 *
 * After calling this, the value returned by lf_time_physical(void)
 * and get_elpased_physical_time(void) will have this specified offset
 * added to what it would have returned before the call.
 */
void clock_sync_set_constant_bias(interval_t offset);

#endif // CLOCK_SYNC_H
