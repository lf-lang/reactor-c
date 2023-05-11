/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Declarations for runtime infrastructure (RTI) for distributed Lingua Franca programs.
 *
 */

#ifndef RTI_LIB_H
#define RTI_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>      // Defines perror(), errno
#include <sys/socket.h>
#include <sys/types.h>  // Provides select() function to read from multiple sockets.
#include <netinet/in.h> // Defines struct sockaddr_in
#include <arpa/inet.h>  // inet_ntop & inet_pton
#include <unistd.h>     // Defines read(), write(), and close()
#include <netdb.h>      // Defines gethostbyname().
#include <strings.h>    // Defines bzero().
#include <assert.h>
#include <sys/wait.h>   // Defines wait() for process to change state.
#include <pthread.h>

#include "platform.h"   // Platform-specific types and functions
#include "util.h" // Defines print functions (e.g., lf_print).
#include "net_util.h"   // Defines network functions.
#include "net_common.h" // Defines message types, etc. Includes <pthread.h> and "reactor.h".
#include "tag.h"        // Time-related types and functions.
#include "trace.h"      // Tracing related functions

#ifdef __RTI_AUTH__
#include <openssl/rand.h> // For secure random number generation.
#include <openssl/hmac.h> // For HMAC authentication.
#endif

#include "lf_types.h"
#include "message_record/message_record.h"

/////////////////////////////////////////////
//// Data structures

typedef enum socket_type_t {
    TCP,
    UDP
} socket_type_t;

/** Mode of execution of a federate. */
typedef enum execution_mode_t {
    FAST,
    REALTIME
} execution_mode_t;

/** State of a federate during execution. */
typedef enum fed_state_t {
    NOT_CONNECTED,  // The federate has not connected.
    GRANTED,        // Most recent MSG_TYPE_NEXT_EVENT_TAG has been granted.
    PENDING         // Waiting for upstream federates.
} fed_state_t;

/**
 * Information about a federate known to the RTI, including its runtime state,
 * mode of execution, and connectivity with other federates.
 * The list of upstream and downstream federates does not include
 * those that are connected via a "physical" connection (one
 * denoted with ~>) because those connections do not impose
 * any scheduling constraints.
 */
typedef struct federate_t {
    uint16_t id;            // ID of this federate.
    pthread_t thread_id;    // The ID of the thread handling communication with this federate.
    int socket;             // The TCP socket descriptor for communicating with this federate.
    struct sockaddr_in UDP_addr;           // The UDP address for the federate.
    bool clock_synchronization_enabled;    // Indicates the status of clock synchronization
                                           // for this federate. Enabled by default.
    tag_t completed;        // The largest logical tag completed by the federate (or NEVER if no LTC has been received).
    tag_t last_granted;     // The maximum TAG that has been granted so far (or NEVER if none granted)
    tag_t last_provisionally_granted;      // The maximum PTAG that has been provisionally granted (or NEVER if none granted)
    tag_t next_event;       // Most recent NET received from the federate (or NEVER if none received).
    in_transit_message_record_q_t* in_transit_message_tags; // Record of in-transit messages to this federate that are not
                                                            // yet processed. This record is ordered based on the time
                                                            // value of each message for a more efficient access.
    fed_state_t state;      // State of the federate.
    int* upstream;          // Array of upstream federate ids.
    interval_t* upstream_delay;    // Minimum delay on connections from upstream federates.
    							   // Here, NEVER encodes no delay. 0LL is a microstep delay.
    int num_upstream;              // Size of the array of upstream federates and delays.
    int* downstream;        // Array of downstream federate ids.
    int num_downstream;     // Size of the array of downstream federates.
    execution_mode_t mode;  // FAST or REALTIME.
    char server_hostname[INET_ADDRSTRLEN]; // Human-readable IP address and
    int32_t server_port;    // port number of the socket server of the federate
                            // if it has any incoming direct connections from other federates.
                            // The port number will be -1 if there is no server or if the
                            // RTI has not been informed of the port number.
    struct in_addr server_ip_addr; // Information about the IP address of the socket
                                // server of the federate.
    bool requested_stop;    // Indicates that the federate has requested stop or has replied
                            // to a request for stop from the RTI. Used to prevent double-counting
                            // a federate when handling lf_request_stop().
} federate_t;

/**
 * The status of clock synchronization.
 */
typedef enum clock_sync_stat {
    clock_sync_off,
    clock_sync_init,
    clock_sync_on
} clock_sync_stat;

/**
 * Structure that an RTI instance uses to keep track of its own and its
 * corresponding federates' state.
 */
typedef struct RTI_instance_t {
    // The main mutex lock.
    pthread_mutex_t rti_mutex;

    // Condition variable used to signal receipt of all proposed start times.
    pthread_cond_t received_start_times;

    // Condition variable used to signal that a start time has been sent to a federate.
    pthread_cond_t sent_start_time;

    // RTI's decided stop tag for federates
    tag_t max_stop_tag;

    // Number of federates in the federation
    int32_t number_of_federates;

    // The federates.
    federate_t* federates;

    // Maximum start time seen so far from the federates.
    int64_t max_start_time;

    // Number of federates that have proposed start times.
    int num_feds_proposed_start;

    // Number of federates handling stop
    int num_feds_handling_stop;

    /**
     * Boolean indicating that all federates have exited.
     * This gets set to true exactly once before the program exits.
     * It is marked volatile because the write is not guarded by a mutex.
     * The main thread makes this true, then calls shutdown and close on
     * the socket, which will cause accept() to return with an error code
     * in respond_to_erroneous_connections().
     */
    volatile bool all_federates_exited;

    /**
     * The ID of the federation that this RTI will supervise.
     * This should be overridden with a command-line -i option to ensure
     * that each federate only joins its assigned federation.
     */
    const char* federation_id;

    /************* TCP server information *************/
    /** The desired port specified by the user on the command line. */
    uint16_t user_specified_port;

    /** The final port number that the TCP socket server ends up using. */
    uint16_t final_port_TCP;

    /** The TCP socket descriptor for the socket server. */
    int socket_descriptor_TCP;

    /************* UDP server information *************/
    /** The final port number that the UDP socket server ends up using. */
    uint16_t final_port_UDP;

    /** The UDP socket descriptor for the socket server. */
    int socket_descriptor_UDP;

    /************* Clock synchronization information *************/
    /* Thread performing PTP clock sync sessions periodically. */
    pthread_t clock_thread;

    /**
     * Indicates whether clock sync is globally on for the federation. Federates
     * can still selectively disable clock synchronization if they wanted to.
     */
    clock_sync_stat clock_sync_global_status;

    /**
     * Frequency (period in nanoseconds) between clock sync attempts.
     */
    uint64_t clock_sync_period_ns;

    /**
     * Number of messages exchanged for each clock sync attempt.
     */
    int32_t clock_sync_exchanges_per_interval;

    /**
     * Boolean indicating that authentication is enabled.
     */
    bool authentication_enabled;

    /**
     * Boolean indicating that tracing is enabled.
     */
    bool tracing_enabled;
} RTI_instance_t;

/**
 * Enter a critical section where logical time and the event queue are guaranteed
 * to not change unless they are changed within the critical section.
 * this can be implemented by disabling interrupts.
 * Users of this function must ensure that lf_init_critical_sections() is
 * called first and that lf_critical_section_exit() is called later.
 * @return 0 on success, platform-specific error number otherwise.
 */
extern int lf_critical_section_enter();

/**
 * Exit the critical section entered with lf_lock_time().
 * @return 0 on success, platform-specific error number otherwise.
 */
extern int lf_critical_section_exit();

/**
 * Create a server and enable listening for socket connections.
 *
 * @note This function is similar to create_server(...) in
 * federate.c. However, it contains logs that are specific
 * to the RTI.
 *
 * @param port The port number to use.
 * @param socket_type The type of the socket for the server (TCP or UDP).
 * @return The socket descriptor on which to accept connections.
 */
int create_server(int32_t specified_port, uint16_t port, socket_type_t socket_type);

/**
 * Send a tag advance grant (TAG) message to the specified federate.
 * Do not send it if a previously sent PTAG was greater or if a
 * previously sent TAG was greater or equal.
 *
 * This function will keep a record of this TAG in the federate's last_granted
 * field.
 *
 * This function assumes that the caller holds the mutex lock.
 *
 * @param fed The federate.
 * @param tag The tag to grant.
 */
void send_tag_advance_grant(federate_t* fed, tag_t tag);

/**
 * Find the earliest tag at which the specified federate may
 * experience its next event. This is the least next event tag (NET)
 * of the specified federate and (transitively) upstream federates
 * (with delays of the connections added). For upstream federates,
 * we assume (conservatively) that federate upstream of those
 * may also send an event. The result will never be less than
 * the completion time of the federate (which may be NEVER,
 * if the federate has not yet completed a logical time).
 *
 * FIXME: This could be made less conservative by building
 * at code generation time a causality interface table indicating
 * which outputs can be triggered by which inputs. For now, we
 * assume any output can be triggered by any input.
 *
 * @param fed The federate.
 * @param candidate A candidate tag (for the first invocation,
 *  this should be fed->next_event).
 * @param visited An array of booleans indicating which federates
 *  have been visited (for the first invocation, this should be
 *  an array of falses of size _RTI.number_of_federates).
 */
tag_t transitive_next_event(federate_t* fed, tag_t candidate, bool visited[]);

/**
 * Send a provisional tag advance grant (PTAG) message to the specified federate.
 * Do not send it if a previously sent PTAG or TAG was greater or equal.
 *
 * This function will keep a record of this PTAG in the federate's last_provisionally_granted
 * field.
 *
 * This function assumes that the caller holds the mutex lock.
 *
 * @param fed The federate.
 * @param tag The tag to grant.
 */
void send_provisional_tag_advance_grant(federate_t* fed, tag_t tag);

/**
 * Determine whether the specified federate fed is eligible for a tag advance grant,
 * (TAG) and, if so, send it one. This is called upon receiving a LTC, NET
 * or resign from an upstream federate.
 *
 * This function calculates the minimum M over
 * all upstream federates of the "after" delay plus the most recently
 * received LTC from that federate. If M is greater than the
 * most recently sent TAG to fed or greater than or equal to the most
 * recently sent PTAG, then send a TAG(M) to fed and return.
 *
 * If the above conditions do not result in sending a TAG, then find the
 * minimum M of the earliest possible future message from upstream federates.
 * This is calculated by transitively looking at the most recently received
 * NET message from upstream federates.
 * If M is greater than the NET of the federate fed or the most recently
 * sent PTAG to that federate, then
 * send TAG to the federate with tag equal to the NET of fed or the PTAG.
 * If M is equal to the NET of the federate, then send PTAG(M).
 *
 * This should be called whenever an immediately upstream federate sends to
 * the RTI an LTC (Logical Tag Complete), or when a transitive upstream
 * federate sends a NET (Next Event Tag) message.
 * It is also called when an upstream federate resigns from the federation.
 *
 * This function assumes that the caller holds the mutex lock.
 *
 * @return True if the TAG message is sent and false otherwise.
 */
bool send_advance_grant_if_safe(federate_t* fed);

/**
 * For all federates downstream of the specified federate, determine
 * whether they should be sent a TAG or PTAG and send it if so.
 *
 * This assumes the caller holds the mutex.
 *
 * @param fed The upstream federate.
 * @param visited An array of booleans used to determine whether a federate has
 *  been visited (initially all false).
 */
void send_downstream_advance_grants_if_safe(federate_t* fed, bool visited[]);

/**
 * @brief Update the next event tag of federate `federate_id`.
 *
 * It will update the recorded next event tag of federate `federate_id` to the minimum of `next_event_tag` and the
 * minimum tag of in-transit messages (if any) to the federate.
 *
 * Will try to see if the RTI can grant new TAG or PTAG messages to any
 * downstream federates based on this new next event tag.
 *
 * This function assumes that the caller is holding the _RTI.rti_mutex.
 *
 * @param federate_id The id of the federate that needs to be updated.
 * @param next_event_tag The next event tag for `federate_id`.
 */
void update_federate_next_event_tag_locked(uint16_t federate_id, tag_t next_event_tag);

/**
 * Handle a port absent message being received rom a federate via the RIT.
 *
 * This function assumes the caller does not hold the mutex.
 */
void handle_port_absent_message(federate_t* sending_federate, unsigned char* buffer);

/**
 * Handle a timed message being received from a federate by the RTI to relay to another federate.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param sending_federate The sending federate.
 * @param buffer The buffer to read into (the first byte is already there).
 */
void handle_timed_message(federate_t* sending_federate, unsigned char* buffer);

/**
 * Handle a logical tag complete (LTC) message. @see
 * MSG_TYPE_LOGICAL_TAG_COMPLETE in rti.h.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate that has completed a logical tag.
 */
void handle_logical_tag_complete(federate_t* fed);

/**
 * Handle a next event tag (NET) message. @see MSG_TYPE_NEXT_EVENT_TAG in rti.h.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate sending a NET message.
 */
void handle_next_event_tag(federate_t* fed);

/////////////////// STOP functions ////////////////////
/**
 * Once the RTI has seen proposed tags from all connected federates,
 * it will broadcast a MSG_TYPE_STOP_GRANTED carrying the _RTI.max_stop_tag.
 * This function also checks the most recently received NET from
 * each federate and resets that be no greater than the _RTI.max_stop_tag.
 *
 * This function assumes the caller holds the _RTI.rti_mutex lock.
 */
void _lf_rti_broadcast_stop_time_to_federates_already_locked();

/**
 * Mark a federate requesting stop.
 *
 * If the number of federates handling stop reaches the
 * NUM_OF_FEDERATES, broadcast MSG_TYPE_STOP_GRANTED to every federate.
 *
 * This function assumes the _RTI.rti_mutex is already locked.
 *
 * @param fed The federate that has requested a stop or has suddenly
 *  stopped (disconnected).
 */
void mark_federate_requesting_stop(federate_t* fed);

/**
 * Handle a MSG_TYPE_STOP_REQUEST message.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate sending a MSG_TYPE_STOP_REQUEST message.
 */
void handle_stop_request_message(federate_t* fed);

/**
 * Handle a MSG_TYPE_STOP_REQUEST_REPLY message.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate replying the MSG_TYPE_STOP_REQUEST
 */
void handle_stop_request_reply(federate_t* fed);

//////////////////////////////////////////////////

/**
 * Handle address query messages.
 * This function reads the body of a MSG_TYPE_ADDRESS_QUERY (@see net_common.h) message
 * which is the requested destination federate ID and replies with the stored
 * port value for the socket server of that federate. The port values
 * are initialized to -1. If no MSG_TYPE_ADDRESS_ADVERTISEMENT message has been received from
 * the destination federate, the RTI will simply reply with -1 for the port.
 * The sending federate is responsible for checking back with the RTI after a
 * period of time. @see connect_to_federate() in federate.c. *
 * @param fed_id The federate sending a MSG_TYPE_ADDRESS_QUERY message.
 */
void handle_address_query(uint16_t fed_id);

/**
 * Handle address advertisement messages (@see MSG_TYPE_ADDRESS_ADVERTISEMENT in net_common.h).
 * The federate is expected to send its server port number as the next
 * byte. The RTI will keep a record of this number in the .server_port
 * field of the _RTI.federates[federate_id] array of structs.
 *
 * The server_hostname and server_ip_addr fields are assigned
 * in connect_to_federates() upon accepting the socket
 * from the remote federate.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param federate_id The id of the remote federate that is
 *  sending the address advertisement.
 */
void handle_address_ad(uint16_t federate_id);

/**
 * A function to handle timestamp messages.
 * This function assumes the caller does not hold the mutex.
 */
void handle_timestamp(federate_t *my_fed);

/**
 * Take a snapshot of the physical clock time and send
 * it to federate fed_id.
 *
 * This version assumes the caller holds the mutex lock.
 *
 * @param message_type The type of the clock sync message (see net_common.h).
 * @param fed The federate to send the physical time to.
 * @param socket_type The socket type (TCP or UDP).
 */
void send_physical_clock(unsigned char message_type, federate_t* fed, socket_type_t socket_type);

/**
 * Handle clock synchronization T3 messages from federates.
 * These will come in on the TCP channel during initialization
 * and on the UDP channel subsequently. In both cases, this
 * function will reply with a T4 message. If the channel is
 * the UDP channel, then it will follow the T4 message
 * immediately with a "coded probe" message, which will be
 * used by the federate to decide whether to discard this
 * clock synchronization round.
 *
 * @param my_fed The sending federate.
 * @param socket_type The RTI's socket type used for the communication (TCP or UDP)
 */
void handle_physical_clock_sync_message(federate_t* my_fed, socket_type_t socket_type);

/**
 * A (quasi-)periodic thread that performs clock synchronization with each
 * federate. It starts by waiting a time given by _RTI.clock_sync_period_ns
 * and then iterates over the federates, performing a complete clock synchronization
 * interaction with each federate before proceeding to the next federate.
 * The interaction starts with this RTI sending a snapshot of its physical clock
 * to the federate (message T1). It then waits for a reply and then sends another
 * snapshot of its physical clock (message T4).  It then follows that T4 message
 * with a coded probe message that the federate can use to discard the session if
 * the network is congested.
 */
void* clock_synchronization_thread(void* noargs);

/**
 * A function to handle messages labeled
 * as MSG_TYPE_RESIGN sent by a federate. This
 * message is sent at the time of termination
 * after all shutdown events are processed
 * on the federate.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @note At this point, the RTI might have
 * outgoing messages to the federate. This
 * function thus first performs a shutdown
 * on the socket which sends an EOF. It then
 * waits for the remote socket to be closed
 * before closing the socket itself.
 *
 * Assumptions:
 * - We assume that the other side (the federates)
 *  are in charge of closing the socket (by calling
 *  close() on the socket), and then wait for the RTI
 *  to shutdown the socket.
 * - We assume that calling shutdown() follows the same
 *  shutdown procedure as stated in the TCP/IP specification.
 *
 * @param my_fed The federate sending a MSG_TYPE_RESIGN message.
 **/
void handle_federate_resign(federate_t *my_fed);

/**
 * Thread handling TCP communication with a federate.
 * @param fed A pointer to the federate's struct that has the
 *  socket descriptor for the federate.
 */
void* federate_thread_TCP(void* fed);

/**
 * Send a MSG_TYPE_REJECT message to the specified socket and close the socket.
 * @param socket_id The socket.
 * @param error_code An error code.
 */
void send_reject(int socket_id, unsigned char error_code);

/**
 * Listen for a MSG_TYPE_FED_IDS message, which includes as a payload
 * a federate ID and a federation ID. If the federation ID
 * matches this federation, send an MSG_TYPE_ACK and otherwise send
 * a MSG_TYPE_REJECT message. Return 1 if the federate is accepted to
 * the federation and 0 otherwise.
 * @param socket_id The socket on which to listen.
 * @param client_fd The socket address.
 * @return The federate ID for success or -1 for failure.
 */
int32_t receive_and_check_fed_id_message(int socket_id, struct sockaddr_in* client_fd);

/**
 * Listen for a MSG_TYPE_NEIGHBOR_STRUCTURE message, and upon receiving it, fill
 * out the relevant information in the federate's struct.
 */
int receive_connection_information(int socket_id, uint16_t fed_id);

/**
 * Listen for a MSG_TYPE_UDP_PORT message, and upon receiving it, set up
 * clock synchronization and perform the initial clock synchronization.
 * Initial clock synchronization is performed only if the MSG_TYPE_UDP_PORT message
 * payload is not UINT16_MAX. If it is also not 0, then this function sets
 * up to perform runtime clock synchronization using the UDP port number
 * specified in the payload to communicate with the federate's clock
 * synchronization logic.
 * @param socket_id The socket on which to listen.
 * @param fed_id The federate ID.
 * @return 1 for success, 0 for failure.
 */
int receive_udp_message_and_set_up_clock_sync(int socket_id, uint16_t fed_id);

#ifdef __RTI_AUTH__
/**
 * Authenticate incoming federate by performing HMAC-based authentication.
 * 
 * @param socket Socket for the incoming federate tryting to authenticate.
 * @return True if authentication is successful and false otherwise.
 */
bool authenticate_federate(int socket);
#endif

/**
 * Wait for one incoming connection request from each federate,
 * and upon receiving it, create a thread to communicate with
 * that federate. Return when all federates have connected.
 * @param socket_descriptor The socket on which to accept connections.
 */
void connect_to_federates(int socket_descriptor);

/**
 * Thread to respond to new connections, which could be federates of other
 * federations who are attempting to join the wrong federation.
 * @param nothing Nothing needed here.
 */
void* respond_to_erroneous_connections(void* nothing);

/** 
 * Initialize the federate with the specified ID.
 * @param id The federate ID.
 */
void initialize_federate(uint16_t id);

/**
 * Start the socket server for the runtime infrastructure (RTI) and
 * return the socket descriptor.
 * @param num_feds Number of federates.
 * @param port The port on which to listen for socket connections, or
 *  0 to use the default port range.
 */
int32_t start_rti_server(uint16_t port);

/**
 * Start the runtime infrastructure (RTI) interaction with the federates
 * and wait for the federates to exit.
 * @param socket_descriptor The socket descriptor returned by start_rti_server().
 */
void wait_for_federates(int socket_descriptor);

/**
 * Print a usage message.
 */
void usage(int argc, const char* argv[]);

/**
 * Process command-line arguments related to clock synchronization. Will return
 * the last read position of argv if all related arguments are parsed or an
 * invalid argument is read.
 *
 * @param argc: Number of arguments in the list
 * @param argv: The list of arguments as a string
 * @return Current position (head) of argv;
 */
int process_clock_sync_args(int argc, const char* argv[]);

/**
 * Process the command-line arguments. If the command line arguments are not
 * understood, then print a usage message and return 0. Otherwise, return 1.
 * @return 1 if the arguments processed successfully, 0 otherwise.
 */
int process_args(int argc, const char* argv[]);



#endif // RTI_LIB_H