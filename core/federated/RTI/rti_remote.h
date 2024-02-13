#if defined STANDALONE_RTI
/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @author Erling Jellum (erling.r.jellum@ntnu.no)
 * @author Chadlia Jerad (chadlia.jerad@ensi-uma.tn)
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Declarations for runtime infrastructure (RTI) for distributed Lingua Franca programs.
 * This file extends enclave.h with RTI features that are specific to federations and are not
 * used by scheduling enclaves.
 */

#ifndef RTI_REMOTE_H
#define RTI_REMOTE_H

#include <sys/socket.h>
#include <sys/types.h>  // Provides select() function to read from multiple sockets.
#include <netinet/in.h> // Defines struct sockaddr_in
#include <arpa/inet.h>  // inet_ntop & inet_pton
#include <unistd.h>     // Defines read(), write(), and close()
#include <strings.h>    // Defines bzero().

#include "rti_common.h"

#ifdef __RTI_AUTH__
#include <openssl/rand.h> // For secure random number generation.
#include <openssl/hmac.h> // For HMAC authentication.
#endif

#include "lf_types.h"
#include "pqueue_tag.h"

/** Time allowed for federates to reply to stop request. */
#define MAX_TIME_FOR_REPLY_TO_STOP_REQUEST SEC(30)

/////////////////////////////////////////////
//// Data structures

typedef enum socket_type_t {
    TCP,
    UDP
} socket_type_t;

/**
 * Information about a federate known to the RTI, including its runtime state,
 * mode of execution, and connectivity with other federates.
 * The list of upstream and downstream federates does not include
 * those that are connected via a "physical" connection (one
 * denoted with ~>) because those connections do not impose
 * any scheduling constraints.
 */
typedef struct federate_info_t {
    scheduling_node_t enclave;
    bool requested_stop;    // Indicates that the federate has requested stop or has replied
                            // to a request for stop from the RTI. Used to prevent double-counting
                            // a federate when handling lf_request_stop().
    lf_thread_t thread_id;    // The ID of the thread handling communication with this federate.
    int socket;             // The TCP socket descriptor for communicating with this federate.
    struct sockaddr_in UDP_addr;           // The UDP address for the federate.
    bool clock_synchronization_enabled;    // Indicates the status of clock synchronization
                                           // for this federate. Enabled by default.
    pqueue_tag_t* in_transit_message_tags; // Record of in-transit messages to this federate that are not
                                           // yet processed. This record is ordered based on the time
                                           // value of each message for a more efficient access.
    char server_hostname[INET_ADDRSTRLEN]; // Human-readable IP address and
    int32_t server_port;    // port number of the socket server of the federate
                            // if it has any incoming direct connections from other federates.
                            // The port number will be -1 if there is no server or if the
                            // RTI has not been informed of the port number.
    struct in_addr server_ip_addr; // Information about the IP address of the socket
                                // server of the federate.
} federate_info_t;



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
 * It is a special case of `rti_common_t` (declared in enclave.h). Inheritence
 * is mimicked by having the first attributes to be the same as of rti_common_t,
 * except that scheduling_nodes attribute here is of type `federate_info_t**`, while it
 * is of type `scheduling_node_t**` in `rti_common_t`.
 *     // **************** IMPORTANT!!! ********************
 *     // **   If you make any change to this struct,     **
 *     // **   you MUST also change  rti_common_t in     **
 *     // ** (enclave.h)! The change must exactly match.  **
 *     // **************************************************
 */
typedef struct rti_remote_t {
    rti_common_t base;
    // Maximum start time seen so far from the federates.
    int64_t max_start_time;

    // Number of federates that have proposed start times.
    int num_feds_proposed_start;

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
    lf_thread_t clock_thread;

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
     * Boolean indicating that a stop request is already in progress.
     */
    bool stop_in_progress;
} rti_remote_t;

/**
 * Enter a critical section where logical time and the event queue are guaranteed
 * to not change unless they are changed within the critical section.
 * this can be implemented by disabling interrupts.
 * Users of this function must ensure that lf_init_critical_sections() is
 * called first and that lf_critical_section_exit() is called later.
 * @param env Ignored (present for compatibility).
 * @return 0 on success, platform-specific error number otherwise.
 */
extern int lf_critical_section_enter(environment_t* env);

/**
 * Exit the critical section entered with lf_lock_time().
 * @param env Ignored (present for compatibility).
 * @return 0 on success, platform-specific error number otherwise.
 */
extern int lf_critical_section_exit(environment_t* env);

/**
 * Indicator that one or more federates have reported an error on resigning.
 */
extern bool _lf_federate_reports_error;

/**
 * @brief Update the next event tag of federate `federate_id`.
 *
 * It will update the recorded next event tag of federate `federate_id` to the minimum of `next_event_tag` and the
 * minimum tag of in-transit messages (if any) to the federate.
 *
 * Will try to see if the RTI can grant new TAG or PTAG messages to any
 * downstream federates based on this new next event tag.
 *
 * This function assumes that the caller is holding the _RTI.mutex.
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
void handle_port_absent_message(federate_info_t* sending_federate, unsigned char* buffer);

/**
 * Handle a timed message being received from a federate by the RTI to relay to another federate.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param sending_federate The sending federate.
 * @param buffer The buffer to read into (the first byte is already there).
 */
void handle_timed_message(federate_info_t* sending_federate, unsigned char* buffer);

/**
 * Handle a latest tag complete (LTC) message. @see
 * MSG_TYPE_LATEST_TAG_COMPLETE in rti.h.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate that has completed a logical tag.
 */
void handle_latest_tag_complete(federate_info_t* fed);

/**
 * Handle a next event tag (NET) message. @see MSG_TYPE_NEXT_EVENT_TAG in rti.h.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate sending a NET message.
 */
void handle_next_event_tag(federate_info_t* fed);

/////////////////// STOP functions ////////////////////

/**
 * Handle a MSG_TYPE_STOP_REQUEST message.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate sending a MSG_TYPE_STOP_REQUEST message.
 */
void handle_stop_request_message(federate_info_t* fed);

/**
 * Handle a MSG_TYPE_STOP_REQUEST_REPLY message.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate replying the MSG_TYPE_STOP_REQUEST
 */
void handle_stop_request_reply(federate_info_t* fed);

//////////////////////////////////////////////////

/**
 * Handle address query messages.
 * This function reads the body of a MSG_TYPE_ADDRESS_QUERY (@see net_common.h) message
 * which is the requested destination federate ID and replies with the stored
 * port value for the socket server of that federate. The port values
 * are initialized to -1. If no MSG_TYPE_ADDRESS_ADVERTISEMENT message has been received from
 * the destination federate, the RTI will simply reply with -1 for the port.
 * The sending federate is responsible for checking back with the RTI after a
 * period of time.
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
 * in lf_connect_to_federates() upon accepting the socket
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
void handle_timestamp(federate_info_t *my_fed);

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
void send_physical_clock(unsigned char message_type, federate_info_t* fed, socket_type_t socket_type);

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
void handle_physical_clock_sync_message(federate_info_t* my_fed, socket_type_t socket_type);

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
 * Thread handling TCP communication with a federate.
 * @param fed A pointer to the federate's struct that has the
 *  socket descriptor for the federate.
 */
void* federate_info_thread_TCP(void* fed);

/**
 * Send a MSG_TYPE_REJECT message to the specified socket and close the socket.
 * @param socket_id Pointer to the socket ID.
 * @param error_code An error code.
 */
void send_reject(int* socket_id, unsigned char error_code);

/**
 * Wait for one incoming connection request from each federate,
 * and upon receiving it, create a thread to communicate with
 * that federate. Return when all federates have connected.
 * @param socket_descriptor The socket on which to accept connections.
 */
void lf_connect_to_federates(int socket_descriptor);

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
void initialize_federate(federate_info_t* fed, uint16_t id);

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

/**
 * Initialize the _RTI instance.
 */
void initialize_RTI(rti_remote_t *rti);

#endif // RTI_REMOTE_H
#endif // STANDALONE_RTI