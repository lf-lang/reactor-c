/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
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
 * Data structures and functions used and defined in federate.c.
 */

#ifndef FEDERATE_H
#define FEDERATE_H

#include <stdbool.h>

#include "tag.h"
#include "lf_types.h"
#include "environment.h"
#include "platform.h"

#ifndef ADVANCE_MESSAGE_INTERVAL
#define ADVANCE_MESSAGE_INTERVAL MSEC(10)
#endif

/**
 * Structure that a federate instance uses to keep track of its own state.
 */
typedef struct federate_instance_t {
    /**
     * The TCP socket descriptor for this federate to communicate with the RTI.
     * This is set by connect_to_rti(), which must be called before other
     * functions that communicate with the rti are called.
     */
    int socket_TCP_RTI;

    /**
     * Thread listening for incoming TCP messages from the RTI.
     */
    lf_thread_t RTI_socket_listener;

    /**
     * Thread responsible for setting ports to absent by an STAA offset if they
     * aren't already known.
     */
    #ifdef FEDERATED_DECENTRALIZED
    lf_thread_t staaSetter;
    #endif

    /**
     * Number of inbound physical connections to the federate.
     * This can be either physical connections, or logical connections
     * in the decentralized coordination, or both.
     */
    size_t number_of_inbound_p2p_connections;

    /**
     * Array of thread IDs for threads that listen for incoming messages.
     * This is NULL if there are none and otherwise has size given by
     * number_of_inbound_p2p_connections.
     */
    lf_thread_t *inbound_socket_listeners;

    /**
     * Number of outbound peer-to-peer connections from the federate.
     * This can be either physical connections, or logical connections
     * in the decentralized coordination, or both.
     */
    size_t number_of_outbound_p2p_connections;

    /**
     * An array that holds the socket descriptors for inbound
     * connections from each federate. The index will be the federate
     * ID of the remote sending federate. This is initialized at startup
     * to -1 and is set to a socket ID by handle_p2p_connections_from_federates()
     * when the socket is opened.
     *
     * @note There will not be an inbound socket unless a physical connection
     * or a p2p logical connection (by setting the coordination target property
     * to "distributed") is specified in the Lingua Franca program where this
     * federate is the destination. Multiple incoming p2p connections from the
     * same remote federate will use the same socket.
     */
    int sockets_for_inbound_p2p_connections[NUMBER_OF_FEDERATES];

    /**
     * An array that holds the socket descriptors for outbound direct
     * connections to each remote federate. The index will be the federate
     * ID of the remote receiving federate. This is initialized at startup
     * to -1 and is set to a socket ID by connect_to_federate()
     * when the socket is opened.
     *
     * @note This federate will not open an outbound socket unless a physical
     * connection or a p2p logical connection (by setting the coordination target
     * property to "distributed") is specified in the Lingua Franca
     * program where this federate acts as the source. Multiple outgoing p2p
     * connections to the same remote federate will use the same socket.
     */
    int sockets_for_outbound_p2p_connections[NUMBER_OF_FEDERATES];

    /**
     * Thread ID for a thread that accepts sockets and then supervises
     * listening to those sockets for incoming P2P (physical) connections.
     */
    lf_thread_t inbound_p2p_handling_thread_id;

    /**
     * A socket descriptor for the socket server of the federate.
     * This is assigned in create_server().
     * This socket is used to listen to incoming physical connections from
     * remote federates. Once an incoming connection is accepted, the
     * opened socket will be stored in
     * federate_sockets_for_inbound_p2p_connections.
     */
    int server_socket;

    /**
     * The port used for the server socket
     * to listen for messages from other federates.
     * The federate informs the RTI of this port once
     * it has created its socket server by sending
     * an ADDRESS_AD message (@see rti.h).
     */
    int server_port;

    /**
     * Most recent TIME_ADVANCE_GRANT received from the RTI, or NEVER if none
     * has been received.
     * This is used to communicate between the listen_to_rti_TCP thread and the
     * main federate thread.
     * This variable should only be accessed while holding the mutex lock.
     */
    tag_t last_TAG;

    /**
     * Indicates whether the last TAG received is provisional or an ordinary
     * TAG.
     * If the last TAG has been provisional, network port absent reactions must be inserted.
     * This variable should only be accessed while holding the mutex lock.
     */
    bool is_last_TAG_provisional;

    /**
     * Indicator of whether a NET has been sent to the RTI and no TAG
     * yet received in reply.
     * This variable should only be accessed while holding the mutex lock.
     */
    bool waiting_for_TAG;

    /**
     * Indicator of whether this federate has upstream federates.
     * The default value of false may be overridden in _lf_initialize_trigger_objects.
     */
    bool has_upstream;

    /**
     * Indicator of whether this federate has downstream federates.
     * The default value of false may be overridden in _lf_initialize_trigger_objects.
     */
    bool has_downstream;

    /**
     * Used to prevent the federate from sending a REQUEST_STOP
     * message if it has already received a stop request from the RTI.
     * This variable should only be accessed while holding a mutex lock.
     */
    bool received_stop_request_from_rti;

    /**
     * A record of the most recently sent LTC (logical tag complete) message.
     * In some situations, federates can send logical_tag_complete for
     * the same tag twice or more in-a-row to the RTI. For example, when
     * _lf_next() returns without advancing tag. To prevent overwhelming
     * the RTI with extra messages, record the last sent logical tag
     * complete message and check against it in
     * _lf_logical_tag_complete().
     *
     * @note Here, the underlying assumption is that the TCP stack will
     *  deliver the Logical TAG Complete message to the RTI eventually
     *  if it is deliverable
     */
    tag_t last_sent_LTC;

    /**
     * A record of the most recently sent NET (next event tag) message.
     */
    tag_t last_sent_NET;

    /**
     * For use in federates with centralized coordination, the minimum
     * time delay between a physical action within this federate and an
     * output from this federate.  This is NEVER if there is causal
     * path from a physical action to any output.
     */
    instant_t min_delay_from_physical_action_to_federate_output;

    // Trace object
    trace_t* trace;
} federate_instance_t;

#ifdef FEDERATED_DECENTRALIZED
typedef struct staa {
    lf_action_base_t** actions;
    size_t STAA;
    size_t numActions;
} staa_t;
#endif

typedef struct federation_metadata_t {
    const char* federation_id;
    char* rti_host;
    int rti_port;
    char* rti_user;
} federation_metadata_t;

extern lf_mutex_t outbound_socket_mutex;
extern lf_cond_t port_status_changed;
extern lf_cond_t logical_time_changed;

/**
* Generated function that sends information about connections between this federate and
* other federates where messages are routed through the RTI. Currently, this
* only includes logical connections when the coordination is centralized. This
* information is needed for the RTI to perform the centralized coordination.
* @see MSG_TYPE_NEIGHBOR_STRUCTURE in net_common.h
*/
void send_neighbor_structure_to_RTI(int);

/**
 * @brief Spawns a thread to iterate through STAA structs, setting its associated ports absent
 * at an offset if the port is not present with a value by a certain physical time.
 * 
 */
#ifdef FEDERATED_DECENTRALIZED
void spawn_staa_thread(void);
#endif

/**
 * Connect to the federate with the specified id. This established
 * connection will then be used in functions such as send_timed_message()
 * to send messages directly to the specified federate.
 * This function first sends an MSG_TYPE_ADDRESS_QUERY message to the RTI to obtain
 * the IP address and port number of the specified federate. It then attempts
 * to establish a socket connection to the specified federate.
 * If this fails, the program exits. If it succeeds, it sets element [id] of
 * the _fed.sockets_for_outbound_p2p_connections global array to
 * refer to the socket for communicating directly with the federate.
 * @param remote_federate_id The ID of the remote federate.
 */
void connect_to_federate(uint16_t);

/**
 * Send a logical tag complete (LTC) message to the RTI
 * unless an equal or later LTC has previously been sent.
 * This function assumes the caller holds the mutex lock.
 *
 * @param tag_to_send The tag to send.
 */
void _lf_logical_tag_complete(tag_t);

/**
 * Connect to the RTI at the specified host and port and return
 * the socket descriptor for the connection. If this fails, the
 * program exits. If it succeeds, it sets the _fed.socket_TCP_RTI global
 * variable to refer to the socket for communicating with the RTI.
 * @param hostname A hostname, such as "localhost".
 * @param port_number A port number.
 */
void connect_to_rti(const char*, int);

/**
 * Thread that listens for inputs from other federates.
 * This thread listens for messages of type MSG_TYPE_P2P_MESSAGE,
 * MSG_TYPE_P2P_TAGGED_MESSAGE, or MSG_TYPE_PORT_ABSENT (@see net_common.h) from the specified
 * peer federate and calls the appropriate handling function for
 * each message type. If an error occurs or an EOF is received
 * from the peer, then this procedure sets the corresponding
 * socket in _fed.sockets_for_inbound_p2p_connections
 * to -1 and returns, terminating the thread.
 * @param fed_id_ptr A pointer to a uint16_t containing federate ID being listened to.
 *  This procedure frees the memory pointed to before returning.
 */
void* listen_to_federates(void*);

/**
 * Create a server to listen to incoming physical
 * connections from remote federates. This function
 * only handles the creation of the server socket.
 * The reserved port for the server socket is then
 * sent to the RTI by sending an MSG_TYPE_ADDRESS_ADVERTISEMENT message
 * (@see net_common.h). This function expects no response
 * from the RTI.
 *
 * If a port is specified by the user, that will be used
 * as the only possibility for the server. This function
 * will fail if that port is not available. If a port is not
 * specified, the STARTING_PORT (@see net_common.h) will be used.
 * The function will keep incrementing the port in this case
 * until the number of tries reaches PORT_RANGE_LIMIT.
 *
 * @note This function is similar to create_server(...) in rti.c.
 * However, it contains specific log messages for the peer to
 * peer connections between federates. It also additionally
 * sends an address advertisement (MSG_TYPE_ADDRESS_ADVERTISEMENT) message to the
 * RTI informing it of the port.
 *
 * @param specified_port The specified port by the user.
 */
void create_server(int specified_port);

/**
 * Thread to accept connections from other federates that send this federate
 * messages directly (not through the RTI). This thread starts a thread for
 * each accepted socket connection and, once it has opened all expected
 * sockets, exits.
 * @param ignored No argument needed for this thread.
 */
void* handle_p2p_connections_from_federates(void*);

/**
 * Send a port absent message to federate with fed_ID, informing the
 * remote federate that the current federate will not produce an event
 * on this network port at the current logical time.
 *
 * @param env The environment in which we are executing
 * @param additional_delay The offset applied to the timestamp
 *  using after. The additional delay will be greater or equal to zero
 *  if an after is used on the connection. If no after is given in the
 *  program, -1 is passed.
 * @param port_ID The ID of the receiving port.
 * @param fed_ID The fed ID of the receiving federate.
 */
void send_port_absent_to_federate(environment_t* env, interval_t, unsigned short, unsigned short);

/**
 * Enqueue port absent reactions that will send a PORT_ABSENT
 * message to downstream federates if a given network output port is not present.
 */
void enqueue_port_absent_reactions(environment_t* env);

/**
 * @brief Prevent the advancement to the next level of the reaction queue until the
 * level we try to advance to is known to be under the max level allowed to advance.
 *
 * @param next_reaction_level
 */
void stall_advance_level_federation(environment_t*, size_t);

/**
 * @brief Attempts to update the max level the reaction queue is allowed to advance to
 * for the current logical timestep.
 *
 * @param tag The latest TAG received by this federate.
 * @param is_provisional Whether the latest tag was provisional
 *
 * This function assumes that the caller holds the mutex.
 */
void update_max_level(tag_t, bool);

/**
 * Send a message to another federate directly or via the RTI.
 * This method assumes that the caller does not hold the outbound_socket_mutex lock,
 * which it acquires to perform the send.
 *
 * If the socket connection to the remote federate or the RTI has been broken,
 * then this returns 0 without sending. Otherwise, it returns 1.
 *
 * @note This function is similar to send_timed_message() except that it
 *  does not deal with time and timed_messages.
 *
 * @param message_type The type of the message being sent.
 *  Currently can be MSG_TYPE_TAGGED_MESSAGE for messages sent via
 *  RTI or MSG_TYPE_P2P_TAGGED_MESSAGE for messages sent between
 *  federates.
 * @param port The ID of the destination port.
 * @param federate The ID of the destination federate.
 * @param next_destination_str The name of the next destination in string format
 * @param length The message length.
 * @param message The message.
 * @return 1 if the message has been sent, 0 otherwise.
 */
int send_message(int message_type,
                  unsigned short port,
                  unsigned short federate,
                  const char* next_destination_str,
                  size_t length,
                  unsigned char* message);

/**
 * Send the specified timestamped message to the specified port in the
 * specified federate via the RTI or directly to a federate depending on
 * the given socket. The timestamp is calculated as current_logical_time +
 * additional delay which is greater than or equal to zero.
 * The port should be an input port of a reactor in
 * the destination federate. This version does include the timestamp
 * in the message. The caller can reuse or free the memory after this returns.
 *
 * If the socket connection to the remote federate or the RTI has been broken,
 * then this returns 0 without sending. Otherwise, it returns 1.
 *
 * This method assumes that the caller does not hold the outbound_socket_mutex lock,
 * which it acquires to perform the send.
 *
 * @note This function is similar to send_message() except that it
 *   sends timed messages and also contains logics related to time.
 *
 * @param env The environment in which we are executing
 * @param additional_delay The offset applied to the timestamp
 *  using after. The additional delay will be greater or equal to zero
 *  if an after is used on the connection. If no after is given in the
 *  program, -1 is passed.
 * @param message_type The type of the message being sent.
 *  Currently can be MSG_TYPE_TAGGED_MESSAGE for messages sent via
 *  RTI or MSG_TYPE_P2P_TAGGED_MESSAGE for messages sent between
 *  federates.
 * @param port The ID of the destination port.
 * @param federate The ID of the destination federate.
 * @param next_destination_str The next destination in string format (RTI or federate)
 *  (used for reporting errors).
 * @param length The message length.
 * @param message The message.
 * @return 1 if the message has been sent, 0 otherwise.
 */
int send_timed_message(environment_t*,
                        interval_t,
                        int,
                        unsigned short,
                        unsigned short,
                        const char*,
                        size_t,
                        unsigned char*);

/**
 * Synchronize the start with other federates via the RTI.
 * This assumes that a connection to the RTI is already made
 * and _lf_rti_socket_TCP is valid. It then sends the current logical
 * time to the RTI and waits for the RTI to respond with a specified
 * time. It starts a thread to listen for messages from the RTI.
 */
void synchronize_with_other_federates();

/**
 * Wait until the status of network port "port_ID" is known.
 *
 * In decentralized coordination mode, the wait time is capped by STAA + STA,
 * after which the status of the port is presumed to be absent.
 *
 * This function assumes the holder does not hold a mutex.
 *
 * @param env The environment in which we are executing
 * @param port_ID The ID of the network port
 * @param STAA The safe-to-assume-absent threshold for the port
 */
void wait_until_port_status_known(environment_t* env, int portID, interval_t STAA);

#endif // FEDERATE_H
