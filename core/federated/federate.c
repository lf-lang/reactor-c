/**
 * @file
 * @author Soroush Bateni
 * @author Peter Donovan
 * @author Edward A. Lee
 * @author Anirudh Rengarajsm
 * @copyright (c) 2020-2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Utility functions for a federate in a federated execution.
 */

#ifdef FEDERATED
#if !defined(PLATFORM_Linux) && !defined(PLATFORM_Darwin)
#error No support for federated execution on this platform.
#endif

#include <arpa/inet.h>  // inet_ntop & inet_pton
#include <netdb.h>      // Defines getaddrinfo(), freeaddrinfo() and struct addrinfo.
#include <netinet/in.h> // Defines struct sockaddr_in
#include <sys/socket.h>
#include <unistd.h>     // Defines read(), write(), and close()
#include <string.h>     // Defines memset(), strnlen(), strncmp(), strncpy()
#include <stdio.h>      // Defines strerror()

#include <assert.h>
#include <errno.h>      // Defined perror(), errno
#include <strings.h>    // Defines bzero().

#include "clock-sync.h"
#include "federate.h"
#include "net_common.h"
#include "net_util.h"
#include "reactor.h"
#include "reactor_common.h"
#include "reactor_threaded.h"
#include "api/schedule.h"
#include "scheduler.h"
#include "tracepoint.h"

#ifdef FEDERATED_AUTHENTICATED
#include <openssl/rand.h> // For secure random number generation.
#include <openssl/hmac.h> // For HMAC-based authentication of federates.
#endif

// Global variables defined in tag.c:
extern instant_t start_time;

// Global variable defined in reactor_common.c:
extern bool _lf_termination_executed;

// Global variables references in federate.h
lf_mutex_t lf_outbound_socket_mutex;
lf_cond_t lf_port_status_changed;
lf_cond_t lf_current_tag_changed;

/**
 * The max level allowed to advance (MLAA) is a variable that tracks how far in the reaction
 * queue we can go until we need to wait for more network port statuses to be known.
 * Specifically, when an input port status is unknown at a tag (we don't know whether the upstream
 * federate has sent or will send a message at that tag), then the downstream federate must
 * pause before executing any reaction that depends on that port. A "level" is assigned to that
 * port by the code generator based on the overall topology of the federation. Reactions that
 * depend on the port have higher levels, whereas those with no dependence on that port have
 * lower levels.  The MLAA is a level at which the federate must block until the MLAA is
 * incremented.  It will be incremented as port statuses become known, and when all are known,
 * it will become INT_MAX and all reactions will be unblocked. In decentralized execution, the
 * MLAA is incremented by a background thread that monitors the local physical clock and
 * increments the MLAA when it is safe to assume that the port is absent, if it has not already
 * been incremented by the arrival of a message.  In centralized execution, the MLAA is used
 * only for ports that are involved in a zero-delay cycle (ZDC), and it is incremented when
 * either a message or an absent message arrives.
 */
int max_level_allowed_to_advance;

/**
 * The state of this federate instance. Each executable has exactly one federate instance,
 * and the _fed global variable refers to that instance.
 */
federate_instance_t _fed = {
        .socket_TCP_RTI = -1,
        .number_of_inbound_p2p_connections = 0,
        .inbound_socket_listeners = NULL,
        .number_of_outbound_p2p_connections = 0,
        .inbound_p2p_handling_thread_id = 0,
        .server_socket = -1,
        .server_port = -1,
        .last_TAG = {.time = NEVER, .microstep = 0u},
        .is_last_TAG_provisional = false,
        .has_upstream = false,
        .has_downstream = false,
        .received_stop_request_from_rti = false,
        .last_sent_LTC = (tag_t) {.time = NEVER, .microstep = 0u},
        .last_sent_NET = (tag_t) {.time = NEVER, .microstep = 0u},
        .min_delay_from_physical_action_to_federate_output = NEVER
};

federation_metadata_t federation_metadata = {
    .federation_id =  "Unidentified Federation",
    .rti_host = NULL,
    .rti_port = -1,
    .rti_user = NULL
};

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Static functions (used only internally)

/**
 * Send a time to the RTI. This acquires the lf_outbound_socket_mutex.
 * @param type The message type (MSG_TYPE_TIMESTAMP).
 * @param time The time.
 */
static void send_time(unsigned char type, instant_t time) {
    LF_PRINT_DEBUG("Sending time " PRINTF_TIME " to the RTI.", time);
    size_t bytes_to_write = 1 + sizeof(instant_t);
    unsigned char buffer[bytes_to_write];
    buffer[0] = type;
    encode_int64(time, &(buffer[1]));

    // Trace the event when tracing is enabled
    tag_t tag = {.time = time, .microstep = 0};
    tracepoint_federate_to_rti(send_TIMESTAMP, _lf_my_fed_id, &tag);

    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    write_to_socket_fail_on_error(&_fed.socket_TCP_RTI, bytes_to_write, buffer, &lf_outbound_socket_mutex,
        "Failed to send time " PRINTF_TIME " to the RTI.", time - start_time);
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
}

/**
 * Send a tag to the RTI.
 * This function acquires the lf_outbound_socket_mutex.
 * @param type The message type (MSG_TYPE_NEXT_EVENT_TAG or MSG_TYPE_LATEST_TAG_COMPLETE).
 * @param tag The tag.
 */
static void send_tag(unsigned char type, tag_t tag) {
    LF_PRINT_DEBUG("Sending tag " PRINTF_TAG " to the RTI.", tag.time - start_time, tag.microstep);
    size_t bytes_to_write = 1 + sizeof(instant_t) + sizeof(microstep_t);
    unsigned char buffer[bytes_to_write];
    buffer[0] = type;
    encode_tag(&(buffer[1]), tag);

    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    if (_fed.socket_TCP_RTI < 0) {
        lf_print_warning("Socket is no longer connected. Dropping message.");
        LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
        return;
    }
    trace_event_t event_type = (type == MSG_TYPE_NEXT_EVENT_TAG) ? send_NET : send_LTC;
    // Trace the event when tracing is enabled
    tracepoint_federate_to_rti(event_type, _lf_my_fed_id, &tag);
    write_to_socket_fail_on_error(
            &_fed.socket_TCP_RTI, bytes_to_write, buffer, &lf_outbound_socket_mutex,
            "Failed to send tag " PRINTF_TAG " to the RTI.", tag.time - start_time, tag.microstep);
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
}

/**
 * Return true if either the socket to the RTI is broken or the socket is
 * alive and the first unread byte on the socket's queue is MSG_TYPE_FAILED.
 */
static bool rti_failed() {
    unsigned char first_byte;
    ssize_t bytes = peek_from_socket(_fed.socket_TCP_RTI, &first_byte);
    if (bytes < 0 || (bytes == 1 && first_byte == MSG_TYPE_FAILED)) return true;
    else return false;
}

//////////////////////////////// Port Status Handling ///////////////////////////////////////

extern lf_action_base_t* _lf_action_table[];
extern interval_t _lf_action_delay_table[];
extern size_t _lf_action_table_size;
extern lf_action_base_t* _lf_zero_delay_cycle_action_table[];
extern size_t _lf_zero_delay_cycle_action_table_size;
extern reaction_t* network_input_reactions[];
extern size_t num_network_input_reactions;
extern reaction_t* port_absent_reaction[];
extern size_t num_port_absent_reactions;
#ifdef FEDERATED_DECENTRALIZED
extern staa_t* staa_lst[];
extern size_t staa_lst_size;
#endif

/**
 * Return a pointer to the action struct for the action
 * corresponding to the specified port ID.
 * @param port_id The port ID.
 * @return A pointer to an action struct or null if the ID is out of range.
 */
static lf_action_base_t* action_for_port(int port_id) {
    if (port_id >= 0 && port_id < _lf_action_table_size) {
        return _lf_action_table[port_id];
    }
    lf_print_error_and_exit("Invalid port ID: %d", port_id);
    return NULL;
}

/**
 * Update the last known status tag of all network input ports
 * to the value of `tag`, unless that the provided `tag` is less
 * than the last_known_status_tag of the port. This is called when
 * a TAG signal is received from the RTI in centralized coordination.
 * If any update occurs, then this broadcasts on `lf_port_status_changed`.
 *
 * This assumes the caller holds the mutex.
 *
 * @param tag The tag on which the latest status of all network input
 *  ports is known.
 */
static void update_last_known_status_on_input_ports(tag_t tag) {
    LF_PRINT_DEBUG("In update_last_known_status_on_input ports.");
    bool notify = false;
    for (int i = 0; i < _lf_action_table_size; i++) {
        lf_action_base_t* input_port_action = _lf_action_table[i];
        // This is called when a TAG is received.
        // But it is possible for an input port to have received already
        // a message with a larger tag (if there is an after delay on the
        // connection), in which case, the last known status tag of the port
        // is in the future and should not be rolled back. So in that case,
        // we do not update the last known status tag.
        if (lf_tag_compare(tag,
                input_port_action->trigger->last_known_status_tag) >= 0) {
            LF_PRINT_DEBUG(
                "Updating the last known status tag of port %d from " PRINTF_TAG " to " PRINTF_TAG ".",
                i,
                input_port_action->trigger->last_known_status_tag.time - lf_time_start(),
                input_port_action->trigger->last_known_status_tag.microstep,
                tag.time - lf_time_start(),
                tag.microstep
            );
            input_port_action->trigger->last_known_status_tag = tag;
            notify = true;
        }
    }
    // FIXME: We could put a condition variable into the trigger_t
    // struct for each network input port, in which case this won't
    // be a broadcast but rather a targetted signal.
    if (notify && lf_update_max_level(tag, false)) {
        // Notify network input reactions
        lf_cond_broadcast(&lf_port_status_changed);
    }
}

/**
 * @brief Update the last known status tag of a network input port.
 * 
 * First, if the specified tag is less than the current_tag of the top-level
 * environment, then ignore the specified tag and use the current_tag. This
 * situation can arise if a message has arrived late (an STP violation has occurred).
 * 
 * If the specified tag is greater than the previous last_known_status_tag
 * of the port, then update the last_known_status_tag to the new tag.
 * 
 * If the tag is equal to the previous last_known_status_tag, then
 * increment the microstep of the last_known_status_tag. This situation can
 * occur if a sequence of late messages (STP violations) are occurring all at
 * once during an execution of a logical tag.
 * 
 * This function is called when a message or absent message arrives. For decentralized
 * coordination, it is also called by the background thread update_ports_from_staa_offsets
 * which uses physical time to determine when an input port can be assumed to be absent
 * if a message has not been received.
 *
 * This function assumes the caller holds the mutex on the top-level environment,
 * and, if the tag actually increases, it broadcasts on `lf_port_status_changed`.
 *
 * @param env The top-level environment, whose mutex is assumed to be held.
 * @param tag The tag on which the latest status of the specified network input port is known.
 * @param portID The port ID.
 */
static void update_last_known_status_on_input_port(environment_t* env, tag_t tag, int port_id) {
    if (lf_tag_compare(tag, env->current_tag) < 0) tag = env->current_tag;
    trigger_t* input_port_action = action_for_port(port_id)->trigger;
    int comparison = lf_tag_compare(tag, input_port_action->last_known_status_tag);
    if (comparison == 0) tag.microstep++;
    if (comparison >= 0) {
        LF_PRINT_LOG(
            "Updating the last known status tag of port %d from " PRINTF_TAG " to " PRINTF_TAG ".",
            port_id,
            input_port_action->last_known_status_tag.time - lf_time_start(),
            input_port_action->last_known_status_tag.microstep,
            tag.time - lf_time_start(),
            tag.microstep
        );
        input_port_action->last_known_status_tag = tag;

        // Check whether this port update implies a change to MLAA, which may unblock reactions.
        // For decentralized coordination, the first argument is NEVER, so it has no effect.
        // For centralized, the arguments probably also have no effect, but the port update may.
        // Note that it would not be correct to pass `tag` as the first argument because
        // there is no guarantee that there is either a TAG or a PTAG for this time.
        // The message that triggered this to be called could be from an upstream
        // federate that is far ahead of other upstream federates in logical time.
        lf_update_max_level(_fed.last_TAG, _fed.is_last_TAG_provisional);
        lf_cond_broadcast(&lf_port_status_changed);
    } else {
        // Message arrivals should be monotonic, so this should not occur.
        lf_print_warning("Attempt to update the last known status tag "
               "of network input port %d to an earlier tag was ignored.", port_id);
    }
}

/**
 * Set the status of network port with id portID.
 *
 * @param portID The network port ID
 * @param status The network port status (port_status_t)
 */
static void set_network_port_status(int portID, port_status_t status) {
    lf_action_base_t* network_input_port_action = action_for_port(portID);
    network_input_port_action->trigger->status = status;
}

/**
 * Version of schedule_value() similar to that in reactor_common.c
 * except that it does not acquire the mutex lock and has a special
 * behavior during startup where it can inject reactions to the reaction
 * queue if execution has not started yet.
 * It is also responsible for setting the intended tag of the
 * network message based on the calculated delay.
 * This function assumes that the caller holds the mutex lock.
 *
 * This is used for handling incoming timed messages to a federate.
 *
 * @param env The environment of the federate
 * @param action The action or timer to be triggered.
 * @param tag The tag of the message received over the network.
 * @param value Dynamically allocated memory containing the value to send.
 * @param length The length of the array, if it is an array, or 1 for a
 *  scalar and 0 for no payload.
 * @return A handle to the event, or 0 if no event was scheduled, or -1 for error.
 */
static trigger_handle_t schedule_message_received_from_network_locked(
        environment_t* env,
        trigger_t* trigger,
        tag_t tag,
        lf_token_t* token) {
    assert(env != GLOBAL_ENVIRONMENT);

    // Return value of the function
    trigger_handle_t return_value = 0;

    // Indicates whether or not the intended tag
    // of the message (timestamp, microstep) is
    // in the future relative to the tag of this
    // federate. By default, assume it is not.
    bool message_tag_is_in_the_future = lf_tag_compare(tag, env->current_tag) > 0;
    // Assign the intended tag temporarily to restore later.
    tag_t previous_intended_tag = trigger->intended_tag;
    trigger->intended_tag = tag;

    // Calculate the extra_delay required to be passed
    // to the schedule function.
    interval_t extra_delay = tag.time - env->current_tag.time;
    if (!message_tag_is_in_the_future && env->execution_started) {
#ifdef FEDERATED_CENTRALIZED
        // If the coordination is centralized, receiving a message
        // that does not carry a timestamp that is in the future
        // would indicate a critical condition, showing that the
        // time advance mechanism is not working correctly.
        LF_MUTEX_UNLOCK(&env->mutex);
        lf_print_error_and_exit(
                "Received a message at tag " PRINTF_TAG " that has a tag " PRINTF_TAG
                " that has violated the STP offset. "
                "Centralized coordination should not have these types of messages.",
                env->current_tag.time - start_time, env->current_tag.microstep,
                tag.time - start_time, tag.microstep);
#else
        // Set the delay back to 0
        extra_delay = 0LL;
        LF_PRINT_LOG("Calling schedule with 0 delay and intended tag " PRINTF_TAG ".",
                    trigger->intended_tag.time - start_time,
                    trigger->intended_tag.microstep);
        return_value = lf_schedule_trigger(env, trigger, extra_delay, token);
#endif
    } else {
        // In case the message is in the future, call
        // _lf_schedule_at_tag() so that the microstep is respected.
        LF_PRINT_LOG("Received a message that is (" PRINTF_TIME " nanoseconds, " PRINTF_MICROSTEP " microsteps) "
                "in the future.", extra_delay, tag.microstep - env->current_tag.microstep);
        return_value = _lf_schedule_at_tag(env, trigger, tag, token);
    }
    trigger->intended_tag = previous_intended_tag;
    // Notify the main thread in case it is waiting for physical time to elapse.
    LF_PRINT_DEBUG("Broadcasting notification that event queue changed.");
    lf_cond_broadcast(&env->event_q_changed);
    return return_value;
}

/**
 * Close the socket that receives incoming messages from the
 * specified federate ID. This function should be called when a read
 * of incoming socket fails or when an EOF is received.
 * It can also be called when the receiving end wants to stop communication,
 * in which case, flag should be 1.
 *
 * @param fed_id The ID of the peer federate sending messages to this
 *  federate.
 * @param flag 0 if an EOF was received, -1 if a socket error occurred, 1 otherwise.
 */
static void close_inbound_socket(int fed_id, int flag) {
    LF_MUTEX_LOCK(&socket_mutex);
    if (_fed.sockets_for_inbound_p2p_connections[fed_id] >= 0) {
        if (flag >= 0) {
            if (flag > 0) {
                shutdown(_fed.sockets_for_inbound_p2p_connections[fed_id], SHUT_RDWR);
            } else {
                // Have received EOF from the other end. Send EOF to the other end.
                shutdown(_fed.sockets_for_inbound_p2p_connections[fed_id], SHUT_WR);
            }
        }
        close(_fed.sockets_for_inbound_p2p_connections[fed_id]);
        _fed.sockets_for_inbound_p2p_connections[fed_id] = -1;
    }
    LF_MUTEX_UNLOCK(&socket_mutex);
}

/**
 * Return true if reactions need to be inserted directly into the reaction queue and
 * false if a call to schedule is needed (the normal case). This function handles zero-delay
 * cycles, where processing at a tag must be able to begin before all messages have arrived
 * at that tag. This returns true if the following conditions are all true:
 *
 * 1. the first reaction triggered has a level >= MLAA (a port is or will be blocked on this trigger);
 * 2. the intended_tag is equal to the current tag of the environment;
 * 3. the intended_tag is greater than the last_tag of the trigger;
 * 4. the intended_tag is greater than the last_known_status_tag of the trigger;
 * 5. the execution has started (the event queue has been examined);
 * 6. the trigger is not physical;
 *
 * The comparison against the MLAA (condition 1), if true, means that there is a blocking port
 * waiting for this trigger (or possibly an earlier blocking port). For condition (2), tardy
 * messages are not scheduled now (they are already late), so if a reaction is blocked on
 * unknown status of this port, it will be unblocked with an absent. The comparison against the
 * last_tag of the trigger (condition 3) ensures that if the message is tardy but there is
 * already an earlier tardy message that has been handled (or is being handled), then we
 * don't try to handle two messages in the same tag, which is not allowed. For example, there
 * could be a case where current tag is 10 with a port absent reaction waiting, and a message
 * has arrived with intended_tag 8. This message will eventually cause the port absent reaction
 * to exit, but before that, a message with intended_tag of 9 could arrive before the port absent
 * reaction has had a chance to exit. The port status is on the other hand changed in this thread,
 * and thus, can be checked in this scenario without this race condition. The message with
 * intended_tag of 9 in this case needs to wait one microstep to be processed. The check with
 * last_known_status_tag (condition 4) deals with messages arriving with identical intended
 * tags (which should not happen). This one will be handled late (one microstep later than
 * the current tag if 1 and 2 are true).
  *
 * This function assumes the mutex is held on the environment.
 *
 * @param env The environment.
 * @param trigger The trigger.
 * @param intended_tag The intended tag.
 */
static bool handle_message_now(environment_t* env, trigger_t* trigger, tag_t intended_tag) {
    return trigger->reactions[0]->index >= max_level_allowed_to_advance
            && lf_tag_compare(intended_tag, lf_tag(env)) == 0
            && lf_tag_compare(intended_tag, trigger->last_tag) > 0
            && lf_tag_compare(intended_tag, trigger->last_known_status_tag) > 0
            && env->execution_started
            && !trigger->is_physical;
}

/**
 * Handle a message being received from a remote federate.
 *
 * This function assumes the caller does not hold the mutex lock.
 * @param socket Pointer to the socket to read the message from.
 * @param fed_id The sending federate ID or -1 if the centralized coordination.
 * @return 0 for success, -1 for failure.
 */
static int handle_message(int* socket, int fed_id) {
    // Read the header.
    size_t bytes_to_read = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t);
    unsigned char buffer[bytes_to_read];
    if (read_from_socket_close_on_error(socket, bytes_to_read, buffer)) {
        // Read failed, which means the socket has been closed between reading the
        // message ID byte and here.
        return -1;
    }

    // Extract the header information.
    unsigned short port_id;
    unsigned short federate_id;
    size_t length;
    extract_header(buffer, &port_id, &federate_id, &length);
    // Check if the message is intended for this federate
    assert(_lf_my_fed_id == federate_id);
    LF_PRINT_DEBUG("Receiving message to port %d of length %zu.", port_id, length);

    // Get the triggering action for the corresponding port
    lf_action_base_t* action = action_for_port(port_id);

    // Read the payload.
    // Allocate memory for the message contents.
    unsigned char* message_contents = (unsigned char*)malloc(length);
    if (read_from_socket_close_on_error(socket, length, message_contents)) {
        return -1;
    }
    // Trace the event when tracing is enabled
    tracepoint_federate_from_federate(receive_P2P_MSG, _lf_my_fed_id, federate_id, NULL);
    LF_PRINT_LOG("Message received by federate: %s. Length: %zu.", message_contents, length);

    LF_PRINT_DEBUG("Calling schedule for message received on a physical connection.");
    lf_schedule_value(action, 0, message_contents, length);
    return 0;
}

/**
 * Handle a tagged message being received from a remote federate via the RTI
 * or directly from other federates.
 * This will read the tag encoded in the header
 * and calculate an offset to pass to the schedule function.
 * This function assumes the caller does not hold the mutex lock.
 * Instead of holding the mutex lock, this function calls
 * _lf_increment_tag_barrier with the tag carried in
 * the message header as an argument. This ensures that the current tag
 * will not advance to the tag of the message if it is in the future, or
 * the tag will not advance at all if the tag of the message is
 * now or in the past.
 * @param socket Pointer to the socket to read the message from.
 * @param fed_id The sending federate ID or -1 if the centralized coordination.
 * @return 0 on successfully reading the message, -1 on failure (e.g. due to socket closed).
 */
static int handle_tagged_message(int* socket, int fed_id) {
    // Environment is always the one corresponding to the top-level scheduling enclave.
    environment_t *env;
    _lf_get_environments(&env);

    // Read the header which contains the timestamp.
    size_t bytes_to_read = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t)
            + sizeof(instant_t) + sizeof(microstep_t);
    unsigned char buffer[bytes_to_read];
    if (read_from_socket_close_on_error(socket, bytes_to_read, buffer)) {
        return -1;  // Read failed.
    }

    // Extract the header information.
    unsigned short port_id;
    unsigned short federate_id;
    size_t length;
    tag_t intended_tag;
    extract_timed_header(buffer, &port_id, &federate_id, &length, &intended_tag);
    // Trace the event when tracing is enabled
    if (fed_id == -1) {
        tracepoint_federate_from_rti(receive_TAGGED_MSG, _lf_my_fed_id, &intended_tag);
    } else {
        tracepoint_federate_from_federate(receive_P2P_TAGGED_MSG, _lf_my_fed_id, fed_id, &intended_tag);
    }
    // Check if the message is intended for this federate
    assert(_lf_my_fed_id == federate_id);
    LF_PRINT_DEBUG("Receiving message to port %d of length %zu.", port_id, length);

    // Get the triggering action for the corresponding port
    lf_action_base_t* action = action_for_port(port_id);

    // Record the physical time of arrival of the message
    instant_t time_of_arrival = lf_time_physical();

    if (action->trigger->is_physical) {
        // Messages sent on physical connections should be handled via handle_message().
        lf_print_error_and_exit("Received a tagged message on a physical connection.");
    }

#ifdef FEDERATED_DECENTRALIZED
    // Only applicable for federated programs with decentralized coordination:
    // For logical connections in decentralized coordination,
    // increment the barrier to prevent advancement of tag beyond
    // the received tag if possible. The following function call
    // suggests that the tag barrier be raised to the tag provided
    // by the message. If this tag is in the past, the function will cause
    // the tag to freeze at the current level.
    // If something happens, make sure to release the barrier.
    _lf_increment_tag_barrier(env, intended_tag);
#endif
    LF_PRINT_LOG("Received message on port %d with intended tag: " PRINTF_TAG ", Current tag: " PRINTF_TAG ".",
            port_id, intended_tag.time - start_time, intended_tag.microstep,
            lf_time_logical_elapsed(env), env->current_tag.microstep);

    // Read the payload.
    // Allocate memory for the message contents.
    unsigned char* message_contents = (unsigned char*)malloc(length);
    if (read_from_socket_close_on_error(socket, length, message_contents)) {
#ifdef FEDERATED_DECENTRALIZED 
        _lf_decrement_tag_barrier_locked(env);
#endif
        return -1; // Read failed.
    }

    // The following is only valid for string messages.
    // LF_PRINT_DEBUG("Message received: %s.", message_contents);

    LF_MUTEX_LOCK(&env->mutex);

    action->trigger->physical_time_of_arrival = time_of_arrival;

    // Create a token for the message
    lf_token_t* message_token = _lf_new_token((token_type_t*)action, message_contents, length);

    if (handle_message_now(env, action->trigger, intended_tag)) {
        // Since the message is intended for the current tag and a port absent reaction
        // was waiting for the message, trigger the corresponding reactions for this message.

        update_last_known_status_on_input_port(env, intended_tag, port_id);

        LF_PRINT_LOG(
            "Inserting reactions directly at tag " PRINTF_TAG ". "
            "Intended tag: " PRINTF_TAG ".",
            env->current_tag.time - lf_time_start(),
            env->current_tag.microstep, 
            intended_tag.time - lf_time_start(), 
            intended_tag.microstep
        );
        // Only set the intended tag of the trigger if it is being executed now
        // because otherwise this may preempt the intended_tag of a previous activation
        // of the trigger.
        action->trigger->intended_tag = intended_tag;

        // This will mark the STP violation in the reaction if the message is tardy.
        _lf_insert_reactions_for_trigger(env, action->trigger, message_token);

        // Set the status of the port as present here to inform the network input
        // port absent reactions know that they no longer need to block. The reason for
        // that is because the network receiver reaction is now in the reaction queue
        // keeping the precedence order intact.
        set_network_port_status(port_id, present);
    } else {
        // If no port absent reaction is waiting for this message, or if the intended
        // tag is in the future, or the message is tardy, use schedule functions to process the message.

        tag_t actual_tag = intended_tag;
#ifdef FEDERATED_DECENTRALIZED 
        // For tardy messages in decentralized coordination, we need to figure out what the actual tag will be.
        // (Centralized coordination errors out with tardy messages).
        if (lf_tag_compare(intended_tag, env->current_tag) <= 0) {
            // Message is tardy.
            actual_tag = env->current_tag;
            actual_tag.microstep++;
            // Check that this is greater than any previously scheduled event for this port.
            trigger_t* input_port_action = action_for_port(port_id)->trigger;
            if (lf_tag_compare(actual_tag, input_port_action->last_known_status_tag) <= 0) {
                actual_tag = input_port_action->last_known_status_tag;
                actual_tag.microstep++;
            }
        }
#endif // FEDERATED_DECENTRALIZED
        // The following will update the input_port_action->last_known_status_tag.
        // For decentralized coordination, this is needed for the thread implementing STAA.
        update_last_known_status_on_input_port(env, actual_tag, port_id);

        // If the current time >= stop time, discard the message.
        // But only if the stop time is not equal to the start time!
        if (lf_tag_compare(env->current_tag, env->stop_tag) >= 0 && env->execution_started) {
            lf_print_error("Received message too late. Already at stop tag.\n"
            		"    Current tag is " PRINTF_TAG " and intended tag is " PRINTF_TAG ".\n"
            		"    Discarding message and closing the socket.",
					env->current_tag.time - start_time, env->current_tag.microstep,
					intended_tag.time - start_time, intended_tag.microstep);
            // Close socket, reading any incoming data and discarding it.
            close_inbound_socket(fed_id, 1);
        } else {
            // Need to use intended_tag here, not actual_tag, so that STP violations are detected.
            // It will become actual_tag (that is when the reactions will be invoked).
            schedule_message_received_from_network_locked(env, action->trigger, intended_tag, message_token);
        }
    }

#ifdef FEDERATED_DECENTRALIZED 
    // Only applicable for federated programs with decentralized coordination
    // Finally, decrement the barrier to allow the execution to continue
    // past the raised barrier
    _lf_decrement_tag_barrier_locked(env);
#endif

    // The mutex is unlocked here after the barrier on
    // logical time has been removed to avoid
    // the need for unecessary lock and unlock
    // operations.
    LF_MUTEX_UNLOCK(&env->mutex);

    return 0;
}

/**
 * Handle a port absent message received from a remote federate.
 * This just sets the last known status tag of the port specified
 * in the message.
 *
 * @param socket Pointer to the socket to read the message from
 * @param fed_id The sending federate ID or -1 if the centralized coordination.
 * @return 0 for success, -1 for failure to complete the read.
 */
static int handle_port_absent_message(int* socket, int fed_id) {
    size_t bytes_to_read = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(instant_t) + sizeof(microstep_t);
    unsigned char buffer[bytes_to_read];
    if (read_from_socket_close_on_error(socket, bytes_to_read, buffer)) {
        return -1;
    }

    // Extract the header information.
    unsigned short port_id = extract_uint16(buffer);
    // The next part of the message is the federate_id, but we don't need it.
    // unsigned short federate_id = extract_uint16(&(buffer[sizeof(uint16_t)]));
    tag_t intended_tag = extract_tag(&(buffer[sizeof(uint16_t)+sizeof(uint16_t)]));

    // Trace the event when tracing is enabled
    if (fed_id == -1) {
        tracepoint_federate_from_rti(receive_PORT_ABS, _lf_my_fed_id, &intended_tag);
    } else {
        tracepoint_federate_from_federate(receive_PORT_ABS, _lf_my_fed_id, fed_id, &intended_tag);
    }
    LF_PRINT_LOG("Handling port absent for tag " PRINTF_TAG " for port %hu of fed %d.",
            intended_tag.time - lf_time_start(),
            intended_tag.microstep,
            port_id,
            fed_id
    );

    // Environment is always the one corresponding to the top-level scheduling enclave.
    environment_t *env;
    _lf_get_environments(&env);

    LF_MUTEX_LOCK(&env->mutex);
    update_last_known_status_on_input_port(env, intended_tag, port_id);
    LF_MUTEX_UNLOCK(&env->mutex);

    return 0;
}

/**
 * Thread that listens for inputs from other federates.
 * This thread listens for messages of type MSG_TYPE_P2P_MESSAGE,
 * MSG_TYPE_P2P_TAGGED_MESSAGE, or MSG_TYPE_PORT_ABSENT (@see net_common.h) from the specified
 * peer federate and calls the appropriate handling function for
 * each message type. If an error occurs or an EOF is received
 * from the peer, then this procedure sets the corresponding
 * socket in _fed.sockets_for_inbound_p2p_connections
 * to -1 and returns, terminating the thread.
 * @param _args The remote federate ID (cast to void*).
 * @param fed_id_ptr A pointer to a uint16_t containing federate ID being listened to.
 *  This procedure frees the memory pointed to before returning.
 */
static void* listen_to_federates(void* _args) {
    initialize_lf_thread_id();
    uint16_t fed_id = (uint16_t)(uintptr_t)_args;

    LF_PRINT_LOG("Listening to federate %d.", fed_id);

    int* socket_id = &_fed.sockets_for_inbound_p2p_connections[fed_id];

    // Buffer for incoming messages.
    // This does not constrain the message size
    // because the message will be put into malloc'd memory.
    unsigned char buffer[FED_COM_BUFFER_SIZE];

    // Listen for messages from the federate.
    while (1) {
        bool socket_closed = false;
        // Read one byte to get the message type.
        LF_PRINT_DEBUG("Waiting for a P2P message on socket %d.", *socket_id);
        if (read_from_socket_close_on_error(socket_id, 1, buffer)) {
            // Socket has been closed.
            lf_print("Socket from federate %d is closed.", fed_id);
            // Stop listening to this federate.
            socket_closed = true;
            break;
        }
        LF_PRINT_DEBUG("Received a P2P message on socket %d of type %d.",
                *socket_id, buffer[0]);
        bool bad_message = false;
        switch (buffer[0]) {
            case MSG_TYPE_P2P_MESSAGE:
                LF_PRINT_LOG("Received untimed message from federate %d.", fed_id);
                if (handle_message(socket_id, fed_id)) {
                    // Failed to complete the reading of a message on a physical connection.
                    lf_print_warning("Failed to complete reading of message on physical connection.");
                    socket_closed = true;
                }
                break;
            case MSG_TYPE_P2P_TAGGED_MESSAGE:
                LF_PRINT_LOG("Received tagged message from federate %d.", fed_id);
                if (handle_tagged_message(socket_id, fed_id)) {
                    // P2P tagged messages are only used in decentralized coordination, and
                    // it is not a fatal error if the socket is closed before the whole message is read.
                    // But this thread should exit.
                    lf_print_warning("Failed to complete reading of tagged message.");
                    socket_closed = true;
                }
                break;
            case MSG_TYPE_PORT_ABSENT:
                LF_PRINT_LOG("Received port absent message from federate %d.", fed_id);
                if (handle_port_absent_message(socket_id, fed_id)) {
                    // P2P tagged messages are only used in decentralized coordination, and
                    // it is not a fatal error if the socket is closed before the whole message is read.
                    // But this thread should exit.
                    lf_print_warning("Failed to complete reading of tagged message.");
                    socket_closed = true;
                }
                break;
            default:
                bad_message = true;
        }
        if (bad_message) {
            lf_print_error("Received erroneous message type: %d. Closing the socket.", buffer[0]);
            // Trace the event when tracing is enabled
            tracepoint_federate_from_federate(receive_UNIDENTIFIED, _lf_my_fed_id, fed_id, NULL);
            break; // while loop
        }
        if (socket_closed) {
            // NOTE: For decentralized execution, once this socket is closed, we could
            // update last known tags of all ports connected to the specified federate to FOREVER_TAG,
            // which would eliminate the need to wait for STAA to assume an input is absent.
            // However, at this time, we don't know which ports correspond to which upstream federates.
            // The code generator would have to encode this information. Once that is done,
            // we could call update_last_known_status_on_input_port with FOREVER_TAG.

            break; // while loop
        }
    }
    return NULL;
}

/**
 * Close the socket that sends outgoing messages to the
 * specified federate ID. This function acquires the lf_outbound_socket_mutex mutex lock
 * if _lf_normal_termination is true and otherwise proceeds without the lock.
 * @param fed_id The ID of the peer federate receiving messages from this
 *  federate, or -1 if the RTI (centralized coordination).
 * @param flag 0 if the socket has received EOF, 1 if not, -1 if abnormal termination.
 */
static void close_outbound_socket(int fed_id, int flag) {
    assert (fed_id >= 0 && fed_id < NUMBER_OF_FEDERATES);
    if (_lf_normal_termination) {
        LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    }
    if (_fed.sockets_for_outbound_p2p_connections[fed_id] >= 0) {
        // Close the socket by sending a FIN packet indicating that no further writes
        // are expected.  Then read until we get an EOF indication.
        if (flag >= 0) {
            // SHUT_WR indicates no further outgoing messages.
            shutdown(_fed.sockets_for_outbound_p2p_connections[fed_id], SHUT_WR);
            if (flag > 0) {
                // Have not received EOF yet. read until we get an EOF or error indication.
                // This compensates for delayed ACKs and disabling of Nagles algorithm
                // by delaying exiting until the shutdown is complete.
                unsigned char message[32];
                while (read(_fed.sockets_for_outbound_p2p_connections[fed_id], &message, 32) > 0);
            }
        }
        close(_fed.sockets_for_outbound_p2p_connections[fed_id]);
        _fed.sockets_for_outbound_p2p_connections[fed_id] = -1;
    }
    if (_lf_normal_termination) {
        LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
    }
}

#ifdef FEDERATED_AUTHENTICATED
/**
 * Perform HMAC-based authentication with the RTI, using the federation ID
 * as an HMAC key.
 * @return 0 for success, -1 for failure.
 */
static int perform_hmac_authentication() {

    // Send buffer including message type, federate ID, federate's nonce.
    size_t fed_id_length = sizeof(uint16_t);
    size_t message_length = 1 + fed_id_length + NONCE_LENGTH;
    unsigned char fed_hello_buf[message_length];
    fed_hello_buf[0] = MSG_TYPE_FED_NONCE;
    encode_uint16((uint16_t)_lf_my_fed_id, &fed_hello_buf[1]);
    unsigned char fed_nonce[NONCE_LENGTH];
    RAND_bytes(fed_nonce, NONCE_LENGTH);
    memcpy(&fed_hello_buf[1 + fed_id_length], fed_nonce, NONCE_LENGTH);

    write_to_socket_fail_on_error(
        &_fed.socket_TCP_RTI, message_length, fed_hello_buf, NULL,
        "Failed to write nonce.");

    // Check HMAC of received FED_RESPONSE message.
    unsigned int hmac_length = SHA256_HMAC_LENGTH;
    size_t federation_id_length = strnlen(federation_metadata.federation_id, 255);

    unsigned char received[1 + NONCE_LENGTH + hmac_length];
    if (read_from_socket_close_on_error(&_fed.socket_TCP_RTI, 1 + NONCE_LENGTH + hmac_length, received)) {
        lf_print_warning("Failed to read RTI response.");
        return -1;
    }
    if (received[0] != MSG_TYPE_RTI_RESPONSE) {
        if (received[0] == MSG_TYPE_FAILED) {
            lf_print_error("RTI has failed.");
            return -1;
        } else {
            lf_print_error(
                    "Received unexpected response %u from the RTI (see net_common.h).",
                    received[0]);
            return -1;
        }
    }
    // Create tag to compare to received tag.
    unsigned char buf_to_check[1 + fed_id_length + NONCE_LENGTH];
    buf_to_check[0] = MSG_TYPE_RTI_RESPONSE;
    encode_uint16((uint16_t)_lf_my_fed_id, &buf_to_check[1]);
    memcpy(&buf_to_check[1 + fed_id_length], fed_nonce, NONCE_LENGTH);
    unsigned char fed_tag[hmac_length];
    HMAC(EVP_sha256(), federation_metadata.federation_id, federation_id_length, buf_to_check, 1 + fed_id_length + NONCE_LENGTH,
         fed_tag, &hmac_length);

    // Compare received tag and created tag.
    if (memcmp(&received[1 + NONCE_LENGTH], fed_tag, hmac_length) != 0) {
        // HMAC does not match. Send back a MSG_TYPE_REJECT message.
        lf_print_error("HMAC authentication failed.");
        unsigned char response[2];
        response[0] = MSG_TYPE_REJECT;
        response[1] = HMAC_DOES_NOT_MATCH;

        // Ignore errors on writing back.
        write_to_socket(_fed.socket_TCP_RTI, 2, response);
        return -1;
    } else {
        LF_PRINT_LOG("HMAC verified.");
        // HMAC tag is created with MSG_TYPE_FED_RESPONSE and received federate nonce.
        unsigned char mac_buf[1 + NONCE_LENGTH];
        mac_buf[0] = MSG_TYPE_FED_RESPONSE;
        memcpy(&mac_buf[1], &received[1], NONCE_LENGTH);
        // Buffer for message type and HMAC tag.
        unsigned char sender[1 + hmac_length];
        sender[0] = MSG_TYPE_FED_RESPONSE;
        HMAC(EVP_sha256(), federation_metadata.federation_id, federation_id_length, mac_buf, 1 + NONCE_LENGTH,
             &sender[1], &hmac_length);

        write_to_socket_fail_on_error(
            &_fed.socket_TCP_RTI, 1 + hmac_length, sender, NULL,
            "Failed to write fed response.");
    }
    return 0;
}
#endif

static void close_rti_socket() {
    shutdown(_fed.socket_TCP_RTI, SHUT_RDWR);
    close(_fed.socket_TCP_RTI);
    _fed.socket_TCP_RTI = -1;
}

/**
 * Return in the result a struct with the address info for the specified hostname and port.
 * The memory for the result is dynamically allocated and must be freed using freeaddrinfo.
 * @param hostname The host name.
 * @param port The port number.
 * @param result The struct into which to write.
 */
static void rti_address(const char* hostname, uint16_t port, struct addrinfo** result) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          /* Allow IPv4 */
    hints.ai_socktype = SOCK_STREAM;    /* Stream socket */
    hints.ai_protocol = IPPROTO_TCP;    /* TCP protocol */
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_flags = AI_NUMERICSERV;    /* Allow only numeric port numbers */

    // Convert port number to string.
    char str[6];
    sprintf(str, "%u", port);

    // Get address structure matching hostname and hints criteria, and
    // set port to the port number provided in str. There should only 
    // ever be one matching address structure, and we connect to that.
    if (getaddrinfo(hostname, (const char*)&str, &hints, result)) {
        lf_print_error_and_exit("No host for RTI matching given hostname: %s", hostname);
    }
}

/**
 * Send the specified timestamp to the RTI and wait for a response.
 * The specified timestamp should be current physical time of the
 * federate, and the response will be the designated start time for
 * the federate. This procedure blocks until the response is
 * received from the RTI.
 * @param my_physical_time The physical time at this federate.
 * @return The designated start time for the federate.
 */
static instant_t get_start_time_from_rti(instant_t my_physical_time) {
    // Send the timestamp marker first.
    send_time(MSG_TYPE_TIMESTAMP, my_physical_time);

    // Read bytes from the socket. We need 9 bytes.
    // Buffer for message ID plus timestamp.
    size_t buffer_length = 1 + sizeof(instant_t);
    unsigned char buffer[buffer_length];

    read_from_socket_fail_on_error(&_fed.socket_TCP_RTI, buffer_length, buffer, NULL,
            "Failed to read MSG_TYPE_TIMESTAMP message from RTI.");
    LF_PRINT_DEBUG("Read 9 bytes.");

    // First byte received is the message ID.
    if (buffer[0] != MSG_TYPE_TIMESTAMP) {
        if (buffer[0] == MSG_TYPE_FAILED) {
            lf_print_error_and_exit("RTI has failed.");
        }
        lf_print_error_and_exit(
                "Expected a MSG_TYPE_TIMESTAMP message from the RTI. Got %u (see net_common.h).",
                buffer[0]);
    }

    instant_t timestamp = extract_int64(&(buffer[1]));

    tag_t tag = {.time = timestamp, .microstep = 0};
    // Trace the event when tracing is enabled
    tracepoint_federate_from_rti(receive_TIMESTAMP, _lf_my_fed_id, &tag);
    lf_print("Starting timestamp is: " PRINTF_TIME ".", timestamp);
    LF_PRINT_LOG("Current physical time is: " PRINTF_TIME ".", lf_time_physical());

    return timestamp;
}

/**
 * Handle a time advance grant (TAG) message from the RTI.
 * This updates the last known status tag for each network input
 * port, and broadcasts a signal, which may cause a blocking
 * port absent reaction to unblock.
 *
 * In addition, this updates the last known TAG/PTAG and broadcasts
 * a notification of this update, which may unblock whichever worker
 * thread is trying to advance time.
 *
 * @note This function is very similar to handle_provisinal_tag_advance_grant() except that
 *  it sets last_TAG_was_provisional to false.
 */
static void handle_tag_advance_grant(void) {
    // Environment is always the one corresponding to the top-level scheduling enclave.
    environment_t *env;
    _lf_get_environments(&env);

    size_t bytes_to_read = sizeof(instant_t) + sizeof(microstep_t);
    unsigned char buffer[bytes_to_read];
    read_from_socket_fail_on_error(&_fed.socket_TCP_RTI, bytes_to_read, buffer, NULL,
            "Failed to read tag advance grant from RTI.");
    tag_t TAG = extract_tag(buffer);

    // Trace the event when tracing is enabled
    tracepoint_federate_from_rti(receive_TAG, _lf_my_fed_id, &TAG);

    LF_MUTEX_LOCK(&env->mutex);

    // Update the last known status tag of all network input ports
    // to the TAG received from the RTI. Here we assume that the RTI
    // knows the status of network ports up to and including the granted tag,
    // so by extension, we assume that the federate can safely rely
    // on the RTI to handle port statuses up until the granted tag.
    update_last_known_status_on_input_ports(TAG);

    // It is possible for this federate to have received a PTAG
    // earlier with the same tag as this TAG.
    if (lf_tag_compare(TAG, _fed.last_TAG) >= 0) {
        _fed.last_TAG = TAG;
        _fed.is_last_TAG_provisional = false;
        LF_PRINT_LOG("Received Time Advance Grant (TAG): " PRINTF_TAG ".",
                _fed.last_TAG.time - start_time, _fed.last_TAG.microstep);
    } else {
        LF_MUTEX_UNLOCK(&env->mutex);
        lf_print_error("Received a TAG " PRINTF_TAG " that wasn't larger "
                "than the previous TAG or PTAG " PRINTF_TAG ". Ignoring the TAG.",
                TAG.time - start_time, TAG.microstep,
                _fed.last_TAG.time - start_time, _fed.last_TAG.microstep);
        return;
    }
    // Notify everything that is blocked.
    lf_cond_broadcast(&env->event_q_changed);

    LF_MUTEX_UNLOCK(&env->mutex);
}

#ifdef FEDERATED_DECENTRALIZED
/**
 * @brief Return whether there exists an input port whose status is unknown.
 *
 * @param staa_elem A record of all input port actions.
 */
static bool a_port_is_unknown(staa_t* staa_elem) {
    bool do_wait = false;
    for (int j = 0; j < staa_elem->num_actions; ++j) {
        if (staa_elem->actions[j]->trigger->status == unknown) {
            do_wait = true;
            break;
        }
    }
    return do_wait;
}
#endif

/**
 * @brief Return the port ID of the port associated with the given action.
 * @return The port ID or -1 if there is no match.
 */
static int id_of_action(lf_action_base_t* input_port_action) {
    for (int i = 0; i < _lf_action_table_size; i++) {
        if (_lf_action_table[i] == input_port_action) return i;
    }
    return -1;
}

/**
 * @brief Thread handling setting the known absent status of input ports.
 * For the code-generated array of staa offsets `staa_lst`, which is sorted by STAA offset,
 * wait for physical time to advance to the current time plus the STAA offset,
 * then set the absent status of the input ports associated with the STAA.
 * Then wait for current time to advance and start over.
 */
#ifdef FEDERATED_DECENTRALIZED
static void* update_ports_from_staa_offsets(void* args) {
    initialize_lf_thread_id();
    if (staa_lst_size == 0) return NULL; // Nothing to do.
    // NOTE: Using only the top-level environment, which is the one that deals with network
    // input ports.
    environment_t *env;
    int num_envs = _lf_get_environments(&env);
    LF_MUTEX_LOCK(&env->mutex);
    while (1) {
        LF_PRINT_DEBUG("**** (update thread) starting");
        tag_t tag_when_started_waiting = lf_tag(env);
        for (int i = 0; i < staa_lst_size; ++i) {
            staa_t* staa_elem = staa_lst[i];
            // The staa_elem is adjusted in the code generator to have subtracted the delay on the connection.
            // The list is sorted in increasing order of adjusted STAA offsets.
            // The wait_until function automatically adds the lf_fed_STA_offset to the wait time.
            interval_t wait_until_time = env->current_tag.time + staa_elem->STAA;
            LF_PRINT_DEBUG("**** (update thread) original wait_until_time: " PRINTF_TIME, wait_until_time - lf_time_start());
    
            // The wait_until call will release the env->mutex while it is waiting.
            // However, it will not release the env->mutex if the wait time is too small.
            // At the cost of a small additional delay in deciding a port is absent,
            // we require a minimum wait time here.  Otherwise, if both the STAA and STA are
            // zero, this thread will fail to ever release the environment mutex.
            // This causes chaos.  The MIN_SLEEP_DURATION is the smallest amount of time
            // that wait_until will actually wait. Note that this strategy does not
            // block progress of any execution that is actually processing events.
            // It only slightly delays the decision that an event is absent, and only
            // if the STAA and STA are extremely small.
            if (lf_fed_STA_offset + staa_elem->STAA < 5 * MIN_SLEEP_DURATION) {
                wait_until_time += 5 * MIN_SLEEP_DURATION;
            }
            while (a_port_is_unknown(staa_elem)) {
                LF_PRINT_DEBUG("**** (update thread) waiting until: " PRINTF_TIME, wait_until_time - lf_time_start());
                if (wait_until(env, wait_until_time, &lf_port_status_changed)) {
                    if (lf_tag_compare(lf_tag(env), tag_when_started_waiting) != 0) {
                        break;
                    }
                    /* Possibly useful for debugging:
                    tag_t current_tag = lf_tag(env);
                    LF_PRINT_DEBUG("**** (update thread) Assuming absent! " PRINTF_TAG, current_tag.time - lf_time_start(), current_tag.microstep);
                    LF_PRINT_DEBUG("**** (update thread) Lag is " PRINTF_TIME, current_tag.time - lf_time_physical());
                    LF_PRINT_DEBUG("**** (update thread) Wait until time is " PRINTF_TIME, wait_until_time - lf_time_start());
                    */

                    for (int j = 0; j < staa_elem->num_actions; ++j) {
                        lf_action_base_t* input_port_action = staa_elem->actions[j];
                        if (input_port_action->trigger->status == unknown) {
                            input_port_action->trigger->status = absent;
                            LF_PRINT_DEBUG("**** (update thread) Assuming port absent at time " PRINTF_TIME, lf_tag(env).time - start_time);
                            update_last_known_status_on_input_port(env, lf_tag(env), id_of_action(input_port_action));
                            lf_cond_broadcast(&lf_port_status_changed);
                        }
                    }
                }
                // If the tag has advanced, start over.
                if (lf_tag_compare(lf_tag(env), tag_when_started_waiting) != 0) break;
            }
            // If the tag has advanced, start over.
            if (lf_tag_compare(lf_tag(env), tag_when_started_waiting) != 0) break;
        }
        // If the tag has advanced, start over.
        if (lf_tag_compare(lf_tag(env), tag_when_started_waiting) != 0) continue;

        // At this point, the current tag is the same as when we started waiting
        // and all ports should be known, and hence max_level_allowed_to_advance
        // should be INT_MAX.  Check this to prevent an infinite wait.
        if (max_level_allowed_to_advance != INT_MAX) {
            // If this occurs, then the current tag advanced during a wait.
            // Some ports may have been reset to uknown during that wait, in which case,
            // it would be huge mistake to enter the wait for a new tag below because the
            // program will freeze.  First, check whether any ports are unknown:
            bool port_unkonwn = false;
            for (int i = 0; i < staa_lst_size; ++i) {
                staa_t* staa_elem = staa_lst[i];
                if (a_port_is_unknown(staa_elem)) {
                    port_unkonwn = true;
                    break;
                }
            }
            if (!port_unkonwn) {
                // If this occurs, then there is a race condition that can lead to deadlocks.
                lf_print_error_and_exit("**** (update thread) Inconsistency: All ports are known, but MLAA is blocking.");
            }
            
            // Since max_level_allowed_to_advance will block advancement of time, we cannot follow
            // through to the next step without deadlocking.  Wait some time, then continue.
            // The wait is necessary to prevent a busy wait.
            lf_sleep(2 * MIN_SLEEP_DURATION);
            continue;
        }

        // Wait until we progress to a new tag.
        while (lf_tag_compare(lf_tag(env), tag_when_started_waiting) == 0) {
            // The following will release the env->mutex while waiting.
            LF_PRINT_DEBUG("**** (update thread) Waiting for tags to not match: " PRINTF_TAG ", " PRINTF_TAG,
                lf_tag(env).time - lf_time_start(), lf_tag(env).microstep,
                tag_when_started_waiting.time -lf_time_start(), tag_when_started_waiting.microstep);
            // Ports are reset to unknown at the start of new tag, so that will wake this up.
            lf_cond_wait(&lf_port_status_changed);
        }
        LF_PRINT_DEBUG("**** (update thread) Tags after wait: " PRINTF_TAG ", " PRINTF_TAG,
            lf_tag(env).time - lf_time_start(), lf_tag(env).microstep,
            tag_when_started_waiting.time -lf_time_start(), tag_when_started_waiting.microstep);
    }
    LF_MUTEX_UNLOCK(&env->mutex);
}
#endif // FEDERATED_DECENTRALIZED

/**
 * Handle a provisional tag advance grant (PTAG) message from the RTI.
 * This updates the last known TAG/PTAG and broadcasts
 * a notification of this update, which may unblock whichever worker
 * thread is trying to advance time.
 * If current_time is less than the specified PTAG, then this will
 * also insert into the event_q a dummy event with the specified tag.
 * This will ensure that the federate advances time to the specified
 * tag and, for centralized coordination, stimulates null-message-sending
 * output reactions at that tag.
 *
 * @note This function is similar to handle_tag_advance_grant() except that
 *  it sets last_TAG_was_provisional to true and also it does not update the
 *  last known tag for input ports.
 */
static void handle_provisional_tag_advance_grant() {
    // Environment is always the one corresponding to the top-level scheduling enclave.
    environment_t *env;
    _lf_get_environments(&env);

    size_t bytes_to_read = sizeof(instant_t) + sizeof(microstep_t);
    unsigned char buffer[bytes_to_read];
    read_from_socket_fail_on_error(&_fed.socket_TCP_RTI, bytes_to_read, buffer, NULL,
            "Failed to read provisional tag advance grant from RTI.");
    tag_t PTAG = extract_tag(buffer);

    // Trace the event when tracing is enabled
    tracepoint_federate_from_rti(receive_PTAG, _lf_my_fed_id, &PTAG);

    // Note: it is important that last_known_status_tag of ports does not
    // get updated to a PTAG value because a PTAG does not indicate that
    // the RTI knows about the status of all ports up to and _including_
    // the value of PTAG. Only a TAG message indicates that.
    LF_MUTEX_LOCK(&env->mutex);

    // Sanity check
    if (lf_tag_compare(PTAG, _fed.last_TAG) < 0
            || (lf_tag_compare(PTAG, _fed.last_TAG) == 0 && !_fed.is_last_TAG_provisional)) {
        LF_MUTEX_UNLOCK(&env->mutex);
        lf_print_error_and_exit("Received a PTAG " PRINTF_TAG " that is equal or earlier "
                "than an already received TAG " PRINTF_TAG ".",
                PTAG.time, PTAG.microstep,
                _fed.last_TAG.time, _fed.last_TAG.microstep);
    }

    _fed.last_TAG = PTAG;
    _fed.is_last_TAG_provisional = true;
    LF_PRINT_LOG("At tag " PRINTF_TAG ", received Provisional Tag Advance Grant (PTAG): " PRINTF_TAG ".",
            env->current_tag.time - start_time, env->current_tag.microstep,
            _fed.last_TAG.time - start_time, _fed.last_TAG.microstep);

    // Even if we don't modify the event queue, we need to broadcast a change
    // because we do not need to continue to wait for a TAG.
    lf_cond_broadcast(&env->event_q_changed);
    // Notify level advance thread which is blocked.
    lf_update_max_level(_fed.last_TAG, _fed.is_last_TAG_provisional);
    lf_cond_broadcast(&lf_port_status_changed);

    // Possibly insert a dummy event into the event queue if current time is behind
    // (which it should be). Do not do this if the federate has not fully
    // started yet.

    instant_t dummy_event_time = PTAG.time;
    microstep_t dummy_event_relative_microstep = PTAG.microstep;

    if (lf_tag_compare(env->current_tag, PTAG) == 0) {
        // The current tag can equal the PTAG if we are at the start time
        // or if this federate has been able to advance time to the current
        // tag (e.g., it has no upstream federates). In either case, either
        // it is already treating the current tag as PTAG cycle (e.g. at the
        // start time) or it will be completing the current cycle and sending
        // a LTC message shortly. In either case, there is nothing more to do.
        LF_MUTEX_UNLOCK(&env->mutex);
        return;
    } else if (lf_tag_compare(env->current_tag, PTAG) > 0) {
        // Current tag is greater than the PTAG.
        // It could be that we have sent an LTC that crossed with the incoming
        // PTAG or that we have advanced to a tag greater than the PTAG.
        // In the former case, there is nothing more to do.
        // In the latter case, we may be blocked processing a PTAG cycle at
        // a greater tag or we may be in the middle of processing a regular
        // TAG. In either case, we know that at the PTAG tag, all outputs
        // have either been sent or are absent, so we can send an LTC.
        // Send an LTC to indicate absent outputs.
        lf_latest_tag_complete(PTAG);
        // Nothing more to do.
        LF_MUTEX_UNLOCK(&env->mutex);
        return;
    } else if (PTAG.time == env->current_tag.time) {
        // We now know env->current_tag < PTAG, but the times are equal.
        // Adjust the microstep for scheduling the dummy event.
        dummy_event_relative_microstep -= env->current_tag.microstep;
    }
    // We now know env->current_tag < PTAG.

    if (dummy_event_time != FOREVER) {
        // Schedule a dummy event at the specified time and (relative) microstep.
        LF_PRINT_DEBUG("At tag " PRINTF_TAG ", inserting into the event queue a dummy event "
               "with time " PRINTF_TIME " and (relative) microstep " PRINTF_MICROSTEP ".",
        env->current_tag.time - start_time, env->current_tag.microstep,
        dummy_event_time - start_time, dummy_event_relative_microstep);
        // Dummy event points to a NULL trigger and NULL real event.
        event_t* dummy = _lf_create_dummy_events(env,
                NULL, dummy_event_time, NULL, dummy_event_relative_microstep);
        pqueue_insert(env->event_q, dummy);
    }

    LF_MUTEX_UNLOCK(&env->mutex);
}

/**
 * Handle a MSG_TYPE_STOP_GRANTED message from the RTI.
 *
 * This function removes the global barrier on
 * logical time raised when lf_request_stop() was
 * called in the environment for each enclave.
 */
static void handle_stop_granted_message() {

    size_t bytes_to_read = MSG_TYPE_STOP_GRANTED_LENGTH - 1;
    unsigned char buffer[bytes_to_read];
    read_from_socket_fail_on_error(&_fed.socket_TCP_RTI, bytes_to_read, buffer, NULL,
            "Failed to read stop granted from RTI.");

    tag_t received_stop_tag = extract_tag(buffer);

    // Trace the event when tracing is enabled
    tracepoint_federate_from_rti(receive_STOP_GRN, _lf_my_fed_id, &received_stop_tag);

    LF_PRINT_LOG("Received from RTI a MSG_TYPE_STOP_GRANTED message with elapsed tag " PRINTF_TAG ".",
            received_stop_tag.time - start_time, received_stop_tag.microstep);

    environment_t *env;
    int num_environments = _lf_get_environments(&env);

    for (int i = 0; i < num_environments; i++) {
        LF_MUTEX_LOCK(&env[i].mutex);

        // Sanity check.
        if (lf_tag_compare(received_stop_tag, env[i].current_tag) <= 0) {
            lf_print_error("RTI granted a MSG_TYPE_STOP_GRANTED tag that is equal to or less than this federate's current tag " PRINTF_TAG ". "
                    "Stopping at the next microstep instead.",
                    env[i].current_tag.time - start_time, env[i].current_tag.microstep);
            received_stop_tag = env[i].current_tag;
            received_stop_tag.microstep++;
        }

        lf_set_stop_tag(&env[i], received_stop_tag);
        LF_PRINT_DEBUG("Setting the stop tag to " PRINTF_TAG ".",
                    env[i].stop_tag.time - start_time,
                    env[i].stop_tag.microstep);

        if (env[i].barrier.requestors) _lf_decrement_tag_barrier_locked(&env[i]);
        lf_cond_broadcast(&env[i].event_q_changed);
        LF_MUTEX_UNLOCK(&env[i].mutex);
    }
}

/**
 * Handle a MSG_TYPE_STOP_REQUEST message from the RTI.
 */
static void handle_stop_request_message() {
    size_t bytes_to_read = MSG_TYPE_STOP_REQUEST_LENGTH - 1;
    unsigned char buffer[bytes_to_read];
    read_from_socket_fail_on_error(&_fed.socket_TCP_RTI, bytes_to_read, buffer, NULL,
            "Failed to read stop request from RTI.");
    tag_t tag_to_stop = extract_tag(buffer);

    // Trace the event when tracing is enabled
    tracepoint_federate_from_rti(receive_STOP_REQ, _lf_my_fed_id, &tag_to_stop);
    LF_PRINT_LOG("Received from RTI a MSG_TYPE_STOP_REQUEST signal with tag " PRINTF_TAG ".",
             tag_to_stop.time - start_time,
             tag_to_stop.microstep);

    extern lf_mutex_t global_mutex;
    extern bool lf_stop_requested;
    bool already_blocked = false;

    LF_MUTEX_LOCK(&global_mutex);
    if (lf_stop_requested) {
        LF_PRINT_LOG("Ignoring MSG_TYPE_STOP_REQUEST from RTI because lf_request_stop has been called locally.");
        already_blocked = true;
    }
    // Treat the stop request from the RTI as if a local stop request had been received.
    lf_stop_requested = true;
    LF_MUTEX_UNLOCK(&global_mutex);

    // If we have previously received from the RTI a stop request,
    // or we have previously sent a stop request to the RTI,
    // then we have already blocked tag advance in enclaves.
    // Do not do this twice. The record of whether the first has occurred
    // is guarded by the outbound socket mutex.
    // The second is guarded by the global mutex.
    // Note that the RTI should not send stop requests more than once to federates.
    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    if (_fed.received_stop_request_from_rti) {
        LF_PRINT_LOG("Redundant MSG_TYPE_STOP_REQUEST from RTI. Ignoring it.");
        already_blocked = true;
    } else if (!already_blocked) {
        // Do this only if lf_request_stop has not been called because it will
        // prevent lf_request_stop from sending.
        _fed.received_stop_request_from_rti = true;
    }
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);

    if (already_blocked) {
        // Either we have sent a stop request to the RTI ourselves,
        // or we have previously received a stop request from the RTI.
        // Nothing more to do. Tag advance is already blocked on enclaves.
        return;
    }

    // Iterate over the scheduling enclaves to find the maximum current tag
    // and adjust the tag_to_stop if any of those is greater than tag_to_stop.
    // If not done previously, block tag advance in the enclave.
    environment_t *env;
    int num_environments = _lf_get_environments(&env);
    for (int i = 0; i < num_environments; i++) {
        LF_MUTEX_LOCK(&env[i].mutex);
        if (lf_tag_compare(tag_to_stop, env[i].current_tag) <= 0) {
            // Can't stop at the requested tag. Make a counteroffer.
            tag_to_stop = env->current_tag;
            tag_to_stop.microstep++;
        }
        // Set a barrier to prevent the enclave from advancing past the so-far tag to stop.
        _lf_increment_tag_barrier_locked(&env[i], tag_to_stop);

        LF_MUTEX_UNLOCK(&env[i].mutex);
    }
    // Send the reply, which is the least tag at which we can stop.
    unsigned char outgoing_buffer[MSG_TYPE_STOP_REQUEST_REPLY_LENGTH];
    ENCODE_STOP_REQUEST_REPLY(outgoing_buffer, tag_to_stop.time, tag_to_stop.microstep);

    // Trace the event when tracing is enabled
    tracepoint_federate_to_rti(send_STOP_REQ_REP, _lf_my_fed_id, &tag_to_stop);

    // Send the current logical time to the RTI.
    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    write_to_socket_fail_on_error(
            &_fed.socket_TCP_RTI, MSG_TYPE_STOP_REQUEST_REPLY_LENGTH, outgoing_buffer, &lf_outbound_socket_mutex,
            "Failed to send the answer to MSG_TYPE_STOP_REQUEST to RTI.");
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);

    LF_PRINT_DEBUG("Sent MSG_TYPE_STOP_REQUEST_REPLY to RTI with tag " PRINTF_TAG,
            tag_to_stop.time, tag_to_stop.microstep);
}

/**
 * Send a resign signal to the RTI.
 */
static void send_resign_signal(environment_t* env) {
    size_t bytes_to_write = 1;
    unsigned char buffer[bytes_to_write];
    buffer[0] = MSG_TYPE_RESIGN;
    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    write_to_socket_fail_on_error(
            &_fed.socket_TCP_RTI, bytes_to_write, &(buffer[0]), &lf_outbound_socket_mutex,
            "Failed to send MSG_TYPE_RESIGN.");
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
    LF_PRINT_LOG("Resigned.");
}

/**
 * Send a failed signal to the RTI.
 */
static void send_failed_signal(environment_t* env) {
    size_t bytes_to_write = 1;
    unsigned char buffer[bytes_to_write];
    buffer[0] = MSG_TYPE_FAILED;
    write_to_socket_fail_on_error(
            &_fed.socket_TCP_RTI, bytes_to_write, &(buffer[0]), NULL,
            "Failed to send MSG_TYPE_FAILED.");
    LF_PRINT_LOG("Failed.");
}

/**
 * Handle a failed signal from the RTI. The RTI will only fail
 * if it is forced to exit, e.g. by a SIG_INT. Hence, this federate
 * will exit immediately with an error condition, counting on the
 * termination functions to handle any cleanup needed. 
 */
static void handle_rti_failed_message(void) {
    exit(1);
}

/**
 * Thread that listens for TCP inputs from the RTI.
 * When messages arrive, this calls the appropriate handler.
 * @param args Ignored
 */
static void* listen_to_rti_TCP(void* args) {
    initialize_lf_thread_id();
    // Buffer for incoming messages.
    // This does not constrain the message size
    // because the message will be put into malloc'd memory.
    unsigned char buffer[FED_COM_BUFFER_SIZE];

    // Listen for messages from the federate.
    while (1) {
        // Check whether the RTI socket is still valid
        if (_fed.socket_TCP_RTI < 0) {
            lf_print_warning("Socket to the RTI unexpectedly closed.");
            return NULL;
        }
        // Read one byte to get the message type.
        // This will exit if the read fails.
        int read_failed = read_from_socket(_fed.socket_TCP_RTI, 1, buffer);
        if (read_failed < 0) {
            if (errno == ECONNRESET) {
                lf_print_error("Socket connection to the RTI was closed by the RTI without"
                        " properly sending an EOF first. Considering this a soft error.");
                // FIXME: If this happens, possibly a new RTI must be elected.
                _fed.socket_TCP_RTI = -1;
                return NULL;
            } else {
                lf_print_error("Socket connection to the RTI has been broken with error %d: %s."
                        " The RTI should close connections with an EOF first."
                        " Considering this a soft error.",
                        errno,
                        strerror(errno));
                // FIXME: If this happens, possibly a new RTI must be elected.
                _fed.socket_TCP_RTI = -1;
                return NULL;
            }
        } else if (read_failed > 0) {
            // EOF received.
            lf_print("Connection to the RTI closed with an EOF.");
            _fed.socket_TCP_RTI = -1;
            return NULL;
        }
        switch (buffer[0]) {
            case MSG_TYPE_TAGGED_MESSAGE:
                if (handle_tagged_message(&_fed.socket_TCP_RTI, -1)) {
                    // Failures to complete the read of messages from the RTI are fatal.
                    lf_print_error_and_exit("Failed to complete the reading of a message from the RTI.");
                }
                break;
            case MSG_TYPE_TAG_ADVANCE_GRANT:
                handle_tag_advance_grant();
                break;
            case MSG_TYPE_PROVISIONAL_TAG_ADVANCE_GRANT:
                handle_provisional_tag_advance_grant();
                break;
            case MSG_TYPE_STOP_REQUEST:
                handle_stop_request_message();
                break;
            case MSG_TYPE_STOP_GRANTED:
                handle_stop_granted_message();
                break;
            case MSG_TYPE_PORT_ABSENT:
                if (handle_port_absent_message(&_fed.socket_TCP_RTI, -1)) {
                    // Failures to complete the read of absent messages from the RTI are fatal.
                    lf_print_error_and_exit("Failed to complete the reading of an absent message from the RTI.");
                }
                break;
            case MSG_TYPE_FAILED:
                handle_rti_failed_message();
                break;
            case MSG_TYPE_CLOCK_SYNC_T1:
            case MSG_TYPE_CLOCK_SYNC_T4:
                lf_print_error("Federate %d received unexpected clock sync message from RTI on TCP socket.",
                            _lf_my_fed_id);
                break;
            default:
                lf_print_error_and_exit("Received from RTI an unrecognized TCP message type: %hhx.", buffer[0]);
                // Trace the event when tracing is enabled
                tracepoint_federate_from_rti(receive_UNIDENTIFIED, _lf_my_fed_id, NULL);
            }
    }
    return NULL;
}

/**
 * Modify the specified tag, if necessary, to be an earlier tag based
 * on the current physical time. The earlier tag is necessary if this federate
 * has downstream federates and also has physical actions that may trigger
 * outputs.  In that case, the earlier tag will be the current physical time
 * plus the minimum delay on all such physical actions plus any other delays
 * along the path from the triggering physical action to the output port
 * minus one nanosecond. The modified tag is assured of being less than any
 * output tag that might later be produced.
 * @param tag A pointer to the proposed NET.
 * @return True if this federate requires this modification and the tag was
 *  modified.
 */
static bool bounded_NET(tag_t* tag) {
    // The tag sent by this function is a promise that, absent
    // inputs from another federate, this federate will not produce events
    // earlier than t. But if there are downstream federates and there is
    // a physical action (not counting receivers from upstream federates),
    // then we can only promise up to current physical time (plus the minimum
    // of all minimum delays on the physical actions).
    // In this case, we send a NET message with the current physical time
    // to permit downstream federates to advance. To avoid
    // overwhelming the network, this NET message should be sent periodically
    // at specified intervals controlled by the target parameter
    // coordination-options: {advance-message-interval: time units}.
    // The larger the interval, the more downstream federates will lag
    // behind real time, but the less network traffic. If this option is
    // missing, we issue a warning message suggesting that a redesign
    // might be in order so that outputs don't depend on physical actions.
    LF_PRINT_DEBUG("Checking NET to see whether it should be bounded by physical time."
            " Min delay from physical action: " PRINTF_TIME ".",
            _fed.min_delay_from_physical_action_to_federate_output);
    if (_fed.min_delay_from_physical_action_to_federate_output >= 0LL
            && _fed.has_downstream
    ) {
        // There is a physical action upstream of some output from this
        // federate, and there is at least one downstream federate.
        // Compare the tag to the current physical time.
        instant_t physical_time = lf_time_physical();
        if (physical_time + _fed.min_delay_from_physical_action_to_federate_output < tag->time) {
            // Can only promise up and not including this new time:
            tag->time = physical_time + _fed.min_delay_from_physical_action_to_federate_output - 1L;
            tag->microstep = 0;
            LF_PRINT_LOG("Has physical actions that bound NET to " PRINTF_TAG ".",
                    tag->time - start_time, tag->microstep);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Public functions (declared in reactor.h)
// An empty version of this function is code generated for unfederated execution.

/**
 * Close sockets used to communicate with other federates, if they are open,
 * and send a MSG_TYPE_RESIGN message to the RTI. This implements the function
 * defined in reactor.h. For unfederated execution, the code generator
 * generates an empty implementation.
 * @param env The environment of the federate
 */
void lf_terminate_execution(environment_t* env) {
    assert(env != GLOBAL_ENVIRONMENT);

    // For an abnormal termination (e.g. a SIGINT), we need to send a
    // MSG_TYPE_FAILED message to the RTI, but we should not acquire a mutex.
    if (_fed.socket_TCP_RTI >= 0) {
        if (_lf_normal_termination) {
            tracepoint_federate_to_rti(send_RESIGN, _lf_my_fed_id, &env->current_tag);
            send_resign_signal(env);
        } else {
            tracepoint_federate_to_rti(send_FAILED, _lf_my_fed_id, &env->current_tag);
            send_failed_signal(env);
        }
    }

    LF_PRINT_DEBUG("Closing incoming P2P sockets.");
    // Close any incoming P2P sockets that are still open.
    for (int i=0; i < NUMBER_OF_FEDERATES; i++) {
        close_inbound_socket(i, 1);
        // Ignore errors. Mark the socket closed.
        _fed.sockets_for_inbound_p2p_connections[i] = -1;
    }

    // Check for all outgoing physical connections in
    // _fed.sockets_for_outbound_p2p_connections and
    // if the socket ID is not -1, the connection is still open.
    // Send an EOF by closing the socket here.
    for (int i=0; i < NUMBER_OF_FEDERATES; i++) {

        // Close outbound connections, in case they have not closed themselves.
        // This will result in EOF being sent to the remote federate, except for
        // abnormal termination, in which case it will just close the socket.
        int flag = _lf_normal_termination? 1 : -1;
        close_outbound_socket(i, flag);
    }

    LF_PRINT_DEBUG("Waiting for inbound p2p socket listener threads.");
    // Wait for each inbound socket listener thread to close.
    if (_fed.number_of_inbound_p2p_connections > 0 && _fed.inbound_socket_listeners != NULL) {
        LF_PRINT_LOG("Waiting for %zu threads listening for incoming messages to exit.",
                _fed.number_of_inbound_p2p_connections);
        for (int i=0; i < _fed.number_of_inbound_p2p_connections; i++) {
            // Ignoring errors here.
            lf_thread_join(_fed.inbound_socket_listeners[i], NULL);
        }
    }

    LF_PRINT_DEBUG("Waiting for RTI's socket listener threads.");
    // Wait for the thread listening for messages from the RTI to close.
    lf_thread_join(_fed.RTI_socket_listener, NULL);

    // For abnormal termination, there is no need to free memory.
    if (_lf_normal_termination) {
        LF_PRINT_DEBUG("Freeing memory occupied by the federate.");
        free(_fed.inbound_socket_listeners);
        free(federation_metadata.rti_host);
        free(federation_metadata.rti_user);
    }
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Public functions (declared in federate.h, in alphabetical order)

void lf_connect_to_federate(uint16_t remote_federate_id) {
    int result = -1;
    int count_retries = 0;

    // Ask the RTI for port number of the remote federate.
    // The buffer is used for both sending and receiving replies.
    // The size is what is needed for receiving replies.
    unsigned char buffer[sizeof(int32_t) + INET_ADDRSTRLEN + 1];
    int port = -1;
    struct in_addr host_ip_addr;
    int count_tries = 0;
    while (port == -1 && !_lf_termination_executed) {
        buffer[0] = MSG_TYPE_ADDRESS_QUERY;
        // NOTE: Sending messages in little endian.
        encode_uint16(remote_federate_id, &(buffer[1]));

        LF_PRINT_DEBUG("Sending address query for federate %d.", remote_federate_id);
        // Trace the event when tracing is enabled
        tracepoint_federate_to_rti(send_ADR_QR, _lf_my_fed_id, NULL);

        LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
        write_to_socket_fail_on_error(
                &_fed.socket_TCP_RTI, sizeof(uint16_t) + 1, buffer, &lf_outbound_socket_mutex,
                "Failed to send address query for federate %d to RTI.",
                remote_federate_id);
        LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);

        // Read RTI's response.
        read_from_socket_fail_on_error(&_fed.socket_TCP_RTI, sizeof(int32_t) + 1, buffer, NULL,
                "Failed to read the requested port number for federate %d from RTI.",
                remote_federate_id);

        if (buffer[0] != MSG_TYPE_ADDRESS_QUERY) {
            // Unexpected reply.  Could be that RTI has failed and sent a resignation.
            if (buffer[0] == MSG_TYPE_FAILED) {
                lf_print_error_and_exit("RTI has failed.");
            } else {
                lf_print_error_and_exit("Unexpected reply of type %hhu from RTI (see net_common.h).", buffer[0]);
            }
        }
        port = extract_int32(&buffer[1]);

        read_from_socket_fail_on_error(
                &_fed.socket_TCP_RTI, sizeof(host_ip_addr), (unsigned char*)&host_ip_addr, NULL,
                "Failed to read the IP address for federate %d from RTI.",
                remote_federate_id);

        // A reply of -1 for the port means that the RTI does not know
        // the port number of the remote federate, presumably because the
        // remote federate has not yet sent an MSG_TYPE_ADDRESS_ADVERTISEMENT message to the RTI.
        // Sleep for some time before retrying.
        if (port == -1) {
            if (count_tries++ >= CONNECT_MAX_RETRIES) {
                lf_print_error_and_exit("TIMEOUT obtaining IP/port for federate %d from the RTI.",
                        remote_federate_id);
            }
            // Wait ADDRESS_QUERY_RETRY_INTERVAL nanoseconds.
            lf_sleep(ADDRESS_QUERY_RETRY_INTERVAL);
        }
    }
    assert(port < 65536);
    assert(port > 0);
    uint16_t uport = (uint16_t)port;

#if LOG_LEVEL > 3
    // Print the received IP address in a human readable format
    // Create the human readable format of the received address.
    // This is avoided unless LOG_LEVEL is high enough to
    // subdue the overhead caused by inet_ntop().
    char hostname[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &host_ip_addr, hostname, INET_ADDRSTRLEN);
    LF_PRINT_LOG("Received address %s port %d for federate %d from RTI.",
            hostname, uport, remote_federate_id);
#endif

    // Iterate until we either successfully connect or exceed the number of
    // attempts given by CONNECT_MAX_RETRIES.
    int socket_id = -1;
    while (result < 0 && !_lf_termination_executed) {
        // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
        socket_id = create_real_time_tcp_socket_errexit();

        // Server file descriptor.
        struct sockaddr_in server_fd;
        // Zero out the server_fd struct.
        bzero((char*)&server_fd, sizeof(server_fd));

        // Set up the server_fd fields.
        server_fd.sin_family = AF_INET;    // IPv4
        server_fd.sin_addr = host_ip_addr; // Received from the RTI

        // Convert the port number from host byte order to network byte order.
        server_fd.sin_port = htons(uport);
        result = connect(
            socket_id,
            (struct sockaddr *)&server_fd,
            sizeof(server_fd));

        if (result != 0) {
            lf_print_error("Failed to connect to federate %d on port %d.", remote_federate_id, uport);

            // Try again after some time if the connection failed.
            // Note that this should not really happen since the remote federate should be
            // accepting socket connections. But possibly it will be busy (in process of accepting
            // another socket connection?). Hence, we retry.
            count_retries++;
            if (count_retries > CONNECT_MAX_RETRIES) {
                // If the remote federate is not accepting the connection after CONNECT_MAX_RETRIES
                // treat it as a soft error condition and return.
                lf_print_error("Failed to connect to federate %d after %d retries. Giving up.",
                            remote_federate_id, CONNECT_MAX_RETRIES);
                return;
            }
            lf_print_warning("Could not connect to federate %d. Will try again every" PRINTF_TIME "nanoseconds.\n",
                   remote_federate_id, ADDRESS_QUERY_RETRY_INTERVAL);
            
            // Check whether the RTI is still there.
            if (rti_failed()) break;

            // Wait ADDRESS_QUERY_RETRY_INTERVAL nanoseconds.
            lf_sleep(ADDRESS_QUERY_RETRY_INTERVAL);
        } else {
            // Connect was successful.
            size_t buffer_length = 1 + sizeof(uint16_t) + 1;
            unsigned char buffer[buffer_length];
            buffer[0] = MSG_TYPE_P2P_SENDING_FED_ID;
            if (_lf_my_fed_id > UINT16_MAX) {
                // This error is very unlikely to occur.
                lf_print_error_and_exit("Too many federates! More than %d.", UINT16_MAX);
            }
            encode_uint16((uint16_t)_lf_my_fed_id, (unsigned char*)&(buffer[1]));
            unsigned char federation_id_length = (unsigned char)strnlen(federation_metadata.federation_id, 255);
            buffer[sizeof(uint16_t) + 1] = federation_id_length;
            // Trace the event when tracing is enabled
            tracepoint_federate_to_federate(send_FED_ID, _lf_my_fed_id, remote_federate_id, NULL);
            
            // No need for a mutex because we have the only handle on the socket.
            write_to_socket_fail_on_error(&socket_id,
                    buffer_length, buffer, NULL,
                    "Failed to send fed_id to federate %d.", remote_federate_id);
            write_to_socket_fail_on_error(&socket_id,
                    federation_id_length, (unsigned char*)federation_metadata.federation_id, NULL,
                    "Failed to send federation id to federate %d.",
                    remote_federate_id);

            read_from_socket_fail_on_error(&socket_id, 1, (unsigned char*)buffer, NULL,
                    "Failed to read MSG_TYPE_ACK from federate %d in response to sending fed_id.",
                    remote_federate_id);
            if (buffer[0] != MSG_TYPE_ACK) {
                // Get the error code.
                read_from_socket_fail_on_error(&socket_id, 1, (unsigned char*)buffer, NULL,
                        "Failed to read error code from federate %d in response to sending fed_id.", remote_federate_id);
                lf_print_error("Received MSG_TYPE_REJECT message from remote federate (%d).", buffer[0]);
                result = -1;
                continue;
            } else {
                lf_print("Connected to federate %d, port %d.", remote_federate_id, port);
                // Trace the event when tracing is enabled
                tracepoint_federate_to_federate(receive_ACK, _lf_my_fed_id, remote_federate_id, NULL);
            }
        }
    }
    // Once we set this variable, then all future calls to close() on this
    // socket ID should reset it to -1 within a critical section.
    _fed.sockets_for_outbound_p2p_connections[remote_federate_id] = socket_id;
}

void lf_connect_to_rti(const char* hostname, int port) {
    LF_PRINT_LOG("Connecting to the RTI.");

    // Override passed hostname and port if passed as runtime arguments.
    hostname = federation_metadata.rti_host ? federation_metadata.rti_host : hostname;
    port = federation_metadata.rti_port >= 0 ? federation_metadata.rti_port : port;

    // Adjust the port.
    uint16_t uport = 0;
    if (port < 0 || port > INT16_MAX) {
        lf_print_error(
                "lf_connect_to_rti(): Specified port (%d) is out of range,"
                " using the default port %d instead.",
                port, DEFAULT_PORT
        );
        uport = DEFAULT_PORT;
        port = 0; // Mark so that increments occur between tries.
    } else {
        uport = (uint16_t)port;
    }
    if (uport == 0) {
        uport = DEFAULT_PORT;
    }

    // Create a socket
    _fed.socket_TCP_RTI = create_real_time_tcp_socket_errexit();

    int result = -1;
    int count_retries = 0;
    struct addrinfo* res = NULL;

    while (count_retries++ < CONNECT_MAX_RETRIES && !_lf_termination_executed) {
        if (res != NULL) {
            // This is a repeated attempt.
            if (_fed.socket_TCP_RTI >= 0) close_rti_socket();

            lf_sleep(CONNECT_RETRY_INTERVAL);

            // Create a new socket.
            _fed.socket_TCP_RTI = create_real_time_tcp_socket_errexit();

            if (port == 0) {
                // Free previously allocated address info.
                freeaddrinfo(res);
                // Increment the port number.
                uport++;
                if (uport >= DEFAULT_PORT + MAX_NUM_PORT_ADDRESSES) uport = DEFAULT_PORT;

                // Reconstruct the address info.
                rti_address(hostname, uport, &res);
            }
            lf_print("Trying RTI again on port %d (attempt %d).", uport, count_retries);
        } else {
            // This is the first attempt.
            rti_address(hostname, uport, &res);
        }

        result = connect(_fed.socket_TCP_RTI, res->ai_addr, res->ai_addrlen);
        if (result < 0) continue; // Connect failed.

        // Have connected to an RTI, but not sure it's the right RTI.
        // Send a MSG_TYPE_FED_IDS message and wait for a reply.
        // Notify the RTI of the ID of this federate and its federation.

#ifdef FEDERATED_AUTHENTICATED
        LF_PRINT_LOG("Connected to an RTI. Performing HMAC-based authentication using federation ID.");
        if (perform_hmac_authentication()) {
            if (port == 0) {
                continue; // Try again with a new port.
            } else {
                // No point in trying again because it will be the same port.
                close_rti_socket();
                lf_print_error_and_exit("Authentication failed.");
            }
        }
#else
        LF_PRINT_LOG("Connected to an RTI. Sending federation ID for authentication.");
#endif

        // Send the message type first.
        unsigned char buffer[4];
        buffer[0] = MSG_TYPE_FED_IDS;
        // Next send the federate ID.
        if (_lf_my_fed_id > UINT16_MAX) {
            lf_print_error_and_exit("Too many federates! More than %d.", UINT16_MAX);
        }
        encode_uint16((uint16_t)_lf_my_fed_id, &buffer[1]);
        // Next send the federation ID length.
        // The federation ID is limited to 255 bytes.
        size_t federation_id_length = strnlen(federation_metadata.federation_id, 255);
        buffer[1 + sizeof(uint16_t)] = (unsigned char)(federation_id_length & 0xff);

        // Trace the event when tracing is enabled
        tracepoint_federate_to_rti(send_FED_ID, _lf_my_fed_id, NULL);

        // No need for a mutex here because no other threads are writing to this socket.
        if (write_to_socket(_fed.socket_TCP_RTI, 2 + sizeof(uint16_t), buffer)) {
            continue; // Try again, possibly on a new port.
        }

        // Next send the federation ID itself.
        if (write_to_socket(
                _fed.socket_TCP_RTI,
                federation_id_length,
                (unsigned char*)federation_metadata.federation_id)) {
            continue; // Try again.
        }

        // Wait for a response.
        // The response will be MSG_TYPE_REJECT if the federation ID doesn't match.
        // Otherwise, it will be either MSG_TYPE_ACK or MSG_TYPE_UDP_PORT, where the latter
        // is used if clock synchronization will be performed.
        unsigned char response;

        LF_PRINT_DEBUG("Waiting for response to federation ID from the RTI.");

        if (read_from_socket(_fed.socket_TCP_RTI, 1, &response)) {
            continue; // Try again.
        }
        if (response == MSG_TYPE_REJECT) {
            // Trace the event when tracing is enabled
            tracepoint_federate_from_rti(receive_REJECT, _lf_my_fed_id, NULL);
            // Read one more byte to determine the cause of rejection.
            unsigned char cause;
            read_from_socket_fail_on_error(&_fed.socket_TCP_RTI, 1, &cause, NULL,
                    "Failed to read the cause of rejection by the RTI.");
            if (cause == FEDERATION_ID_DOES_NOT_MATCH || cause == WRONG_SERVER) {
                lf_print_warning("Connected to the wrong RTI on port %d. Will try again", uport);
                continue;
            }
        } else if (response == MSG_TYPE_ACK) {
            // Trace the event when tracing is enabled
            tracepoint_federate_from_rti(receive_ACK, _lf_my_fed_id, NULL);
            LF_PRINT_LOG("Received acknowledgment from the RTI.");
            break;
        } else if (response == MSG_TYPE_RESIGN) {
            lf_print_warning("RTI on port %d resigned. Will try again", uport);
            continue;
        } else {
            lf_print_warning("RTI on port %d gave unexpect response %u. Will try again", uport, response);
            continue;
        }
    }
    if (result < 0) {
        lf_print_error_and_exit("Failed to connect to RTI after %d tries.", CONNECT_MAX_RETRIES);
    }

    freeaddrinfo(res);           /* No longer needed */

    // Call a generated (external) function that sends information
    // about connections between this federate and other federates
    // where messages are routed through the RTI.
    // @see MSG_TYPE_NEIGHBOR_STRUCTURE in net_common.h
    lf_send_neighbor_structure_to_RTI(_fed.socket_TCP_RTI);

    uint16_t udp_port = setup_clock_synchronization_with_rti();

    // Write the returned port number to the RTI
    unsigned char UDP_port_number[1 + sizeof(uint16_t)];
    UDP_port_number[0] = MSG_TYPE_UDP_PORT;
    encode_uint16(udp_port, &(UDP_port_number[1]));
    write_to_socket_fail_on_error(&_fed.socket_TCP_RTI, 1 + sizeof(uint16_t), UDP_port_number, NULL,
                "Failed to send the UDP port number to the RTI.");

    lf_print("Connected to RTI at %s:%d.", hostname, uport);
}

void lf_create_server(int specified_port) {
    assert(specified_port <= UINT16_MAX && specified_port >= 0);
    uint16_t port = (uint16_t)specified_port;
    LF_PRINT_LOG("Creating a socket server on port %d.", port);
    // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
    int socket_descriptor = create_real_time_tcp_socket_errexit();

    // Server file descriptor.
    struct sockaddr_in server_fd;
    // Zero out the server address structure.
    bzero((char*)&server_fd, sizeof(server_fd));

    server_fd.sin_family = AF_INET;            // IPv4
    server_fd.sin_addr.s_addr = INADDR_ANY;    // All interfaces, 0.0.0.0.
    // Convert the port number from host byte order to network byte order.
    server_fd.sin_port = htons(port);

    int result = bind(
            socket_descriptor,
            (struct sockaddr *) &server_fd,
            sizeof(server_fd));
    int count = 0;
    while (result < 0 && count++ < PORT_BIND_RETRY_LIMIT) {
        lf_sleep(PORT_BIND_RETRY_INTERVAL);
        result = bind(
                socket_descriptor,
                (struct sockaddr *) &server_fd,
                sizeof(server_fd));
    }
    if (result < 0) {
        lf_print_error_and_exit("Failed to bind socket on port %d.", port);
    }

    // Set the global server port.
    if (specified_port == 0) {
        // Need to retrieve the port number assigned by the OS.
        struct sockaddr_in assigned;
        socklen_t addr_len = sizeof(assigned);
        if (getsockname(socket_descriptor, (struct sockaddr *) &assigned, &addr_len) < 0) {
            lf_print_error_and_exit("Failed to retrieve assigned port number.");
        }
        _fed.server_port = ntohs(assigned.sin_port);
    } else {
        _fed.server_port = port;
    }

    // Enable listening for socket connections.
    // The second argument is the maximum number of queued socket requests,
    // which according to the Mac man page is limited to 128.
    listen(socket_descriptor, 128);

    LF_PRINT_LOG("Server for communicating with other federates started using port %d.", _fed.server_port);

    // Send the server port number to the RTI
    // on an MSG_TYPE_ADDRESS_ADVERTISEMENT message (@see net_common.h).
    unsigned char buffer[sizeof(int32_t) + 1];
    buffer[0] = MSG_TYPE_ADDRESS_ADVERTISEMENT;
    encode_int32(_fed.server_port, &(buffer[1]));

    // Trace the event when tracing is enabled
    tracepoint_federate_to_rti(send_ADR_AD, _lf_my_fed_id, NULL);

    // No need for a mutex because we have the only handle on this socket.
    write_to_socket_fail_on_error(&_fed.socket_TCP_RTI, sizeof(int32_t) + 1, (unsigned char*)buffer, NULL,
                    "Failed to send address advertisement.");

    LF_PRINT_DEBUG("Sent port %d to the RTI.", _fed.server_port);

    // Set the global server socket
    _fed.server_socket = socket_descriptor;
}

void lf_enqueue_port_absent_reactions(environment_t* env){
    assert(env != GLOBAL_ENVIRONMENT);
#ifdef FEDERATED_CENTRALIZED
    if (!_fed.has_downstream) {
        // This federate is not connected to any downstream federates via a
        // logical connection. No need to trigger port absent
        // reactions.
        return;
    }
#endif
    LF_PRINT_DEBUG("Enqueueing port absent reactions at time " PRINTF_TIME ".", (env->current_tag.time - start_time));
    if (num_port_absent_reactions == 0) {
        LF_PRINT_DEBUG("No port absent reactions.");
        return;
    }
    for (int i = 0; i < num_port_absent_reactions; i++) {
        reaction_t* reaction = port_absent_reaction[i];
        if (reaction && reaction->status == inactive) {
            LF_PRINT_DEBUG("Inserting port absent reaction on reaction queue.");
            lf_scheduler_trigger_reaction(env->scheduler, reaction, -1);
        }
    }
}

void* lf_handle_p2p_connections_from_federates(void* env_arg) {
    assert(env_arg);
    environment_t* env = (environment_t *) env_arg;
    int received_federates = 0;
    // Allocate memory to store thread IDs.
    _fed.inbound_socket_listeners = (lf_thread_t*)calloc(_fed.number_of_inbound_p2p_connections, sizeof(lf_thread_t));
    while (received_federates < _fed.number_of_inbound_p2p_connections && !_lf_termination_executed) {
        // Wait for an incoming connection request.
        struct sockaddr client_fd;
        uint32_t client_length = sizeof(client_fd);
        int socket_id = accept(_fed.server_socket, &client_fd, &client_length);

        if (socket_id < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                if (rti_failed()) break;
                else continue; // Try again.
            } else if (errno == EPERM) {
                lf_print_error_system_failure("Firewall permissions prohibit connection.");
            } else {
                lf_print_error_system_failure("A fatal error occurred while accepting a new socket.");
            }
        }
        LF_PRINT_LOG("Accepted new connection from remote federate.");

        size_t header_length = 1 + sizeof(uint16_t) + 1;
        unsigned char buffer[header_length];
        int read_failed = read_from_socket(socket_id, header_length, (unsigned char*)&buffer);
        if (read_failed || buffer[0] != MSG_TYPE_P2P_SENDING_FED_ID) {
            lf_print_warning("Federate received invalid first message on P2P socket. Closing socket.");
            if (read_failed == 0) {
                // Wrong message received.
                unsigned char response[2];
                response[0] = MSG_TYPE_REJECT;
                response[1] = WRONG_SERVER;
                // Trace the event when tracing is enabled
                tracepoint_federate_to_federate(send_REJECT, _lf_my_fed_id, -3, NULL);
                // Ignore errors on this response.
                write_to_socket(socket_id, 2, response);
            }
            close(socket_id);
            continue;
        }

        // Get the federation ID and check it.
        unsigned char federation_id_length = buffer[header_length - 1];
        char remote_federation_id[federation_id_length];
        read_failed = read_from_socket(socket_id, federation_id_length, (unsigned char*)remote_federation_id);
        if (read_failed || (strncmp(federation_metadata.federation_id, remote_federation_id, strnlen(federation_metadata.federation_id, 255)) != 0)) {
            lf_print_warning("Received invalid federation ID. Closing socket.");
            if (read_failed == 0) {
                unsigned char response[2];
                response[0] = MSG_TYPE_REJECT;
                response[1] = FEDERATION_ID_DOES_NOT_MATCH;
                // Trace the event when tracing is enabled
                tracepoint_federate_to_federate(send_REJECT, _lf_my_fed_id, -3, NULL);
                // Ignore errors on this response.
                write_to_socket(socket_id, 2, response);
            }
            close(socket_id);
            continue;
        }

        // Extract the ID of the sending federate.
        uint16_t remote_fed_id = extract_uint16((unsigned char*)&(buffer[1]));
        LF_PRINT_DEBUG("Received sending federate ID %d.", remote_fed_id);

        // Trace the event when tracing is enabled
        tracepoint_federate_to_federate(receive_FED_ID, _lf_my_fed_id, remote_fed_id, NULL);

        // Once we record the socket_id here, all future calls to close() on
        // the socket should be done while holding the socket_mutex, and this array
        // element should be reset to -1 during that critical section.
        // Otherwise, there can be race condition where, during termination,
        // two threads attempt to simultaneously close the socket.
        _fed.sockets_for_inbound_p2p_connections[remote_fed_id] = socket_id;

        // Send an MSG_TYPE_ACK message.
        unsigned char response = MSG_TYPE_ACK;

        // Trace the event when tracing is enabled
        tracepoint_federate_to_federate(send_ACK, _lf_my_fed_id, remote_fed_id, NULL);

        LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
        write_to_socket_fail_on_error(
                &_fed.sockets_for_inbound_p2p_connections[remote_fed_id],
                1, (unsigned char*)&response,
                &lf_outbound_socket_mutex,
                "Failed to write MSG_TYPE_ACK in response to federate %d.",
                remote_fed_id);
        LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);

        // Start a thread to listen for incoming messages from other federates.
        // The fed_id is a uint16_t, which we assume can be safely cast to and from void*.
        void* fed_id_arg = (void*)(uintptr_t)remote_fed_id;
        int result = lf_thread_create(
                &_fed.inbound_socket_listeners[received_federates],
                listen_to_federates,
                fed_id_arg);
        if (result != 0) {
            // Failed to create a listening thread.
            LF_MUTEX_LOCK(&socket_mutex);
            if (_fed.sockets_for_inbound_p2p_connections[remote_fed_id] != -1) {
                close(socket_id);
                _fed.sockets_for_inbound_p2p_connections[remote_fed_id] = -1;
            }
            LF_MUTEX_UNLOCK(&socket_mutex);
            lf_print_error_and_exit(
                    "Failed to create a thread to listen for incoming physical connection. Error code: %d.",
                    result
            );
        }

        received_federates++;
    }

    LF_PRINT_LOG("All %zu remote federates are connected.", _fed.number_of_inbound_p2p_connections);
    return NULL;
}

void lf_latest_tag_complete(tag_t tag_to_send) {
    int compare_with_last_tag = lf_tag_compare(_fed.last_sent_LTC, tag_to_send);
    if (compare_with_last_tag >= 0) {
        return;
    }
    LF_PRINT_LOG("Sending Latest Tag Complete (LTC) " PRINTF_TAG " to the RTI.",
            tag_to_send.time - start_time,
            tag_to_send.microstep);
    send_tag(MSG_TYPE_LATEST_TAG_COMPLETE, tag_to_send);
    _fed.last_sent_LTC = tag_to_send;
}

parse_rti_code_t lf_parse_rti_addr(const char* rti_addr) {
    bool has_host = false, has_port = false, has_user = false;
    rti_addr_info_t rti_addr_info = {0};
    extract_rti_addr_info(rti_addr, &rti_addr_info);
    if (!rti_addr_info.has_host && !rti_addr_info.has_port && !rti_addr_info.has_user) {
        return FAILED_TO_PARSE;
    }
    if (rti_addr_info.has_host) {
        if (validate_host(rti_addr_info.rti_host_str)) {
            char* rti_host = (char*) calloc(256, sizeof(char));
            strncpy(rti_host, rti_addr_info.rti_host_str, 255);
            federation_metadata.rti_host = rti_host;
        } else {
            return INVALID_HOST;
        }
    }
    if (rti_addr_info.has_port) {
        if (validate_port(rti_addr_info.rti_port_str)) {
            federation_metadata.rti_port = atoi(rti_addr_info.rti_port_str);
        } else {
            return INVALID_PORT;
        }
    }
    if (rti_addr_info.has_user) {
        if (validate_user(rti_addr_info.rti_user_str)) {
            char* rti_user = (char*) calloc(256, sizeof(char));
            strncpy(rti_user, rti_addr_info.rti_user_str, 255);
            federation_metadata.rti_user = rti_user;
        } else {
            return INVALID_USER;
        }
    }
    return SUCCESS;
}

void lf_reset_status_fields_on_input_port_triggers() {
    environment_t *env;
    _lf_get_environments(&env);
    tag_t now = lf_tag(env);
    for (int i = 0; i < _lf_action_table_size; i++) {
        if (lf_tag_compare(_lf_action_table[i]->trigger->last_known_status_tag, now) >= 0) {
            set_network_port_status(i, absent);  // Default may be overriden to become present.
        } else {
            set_network_port_status(i, unknown);
        }
    }
    LF_PRINT_DEBUG("Resetting port status fields.");
    lf_update_max_level(_fed.last_TAG, _fed.is_last_TAG_provisional);
    lf_cond_broadcast(&lf_port_status_changed);
}

int lf_send_message(int message_type,
                  unsigned short port,
                  unsigned short federate,
                  const char* next_destination_str,
                  size_t length,
                  unsigned char* message) {
    unsigned char header_buffer[1 + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t)];
    // First byte identifies this as a timed message.
    if (message_type != MSG_TYPE_P2P_MESSAGE ) {
        lf_print_error("lf_send_message: Unsupported message type (%d).", message_type);
        return -1;
    }
    header_buffer[0] = (unsigned char)message_type;
    // Next two bytes identify the destination port.
    // NOTE: Send messages little endian (network order), not big endian.
    encode_uint16(port, &(header_buffer[1]));

    // Next two bytes identify the destination federate.
    encode_uint16(federate, &(header_buffer[1 + sizeof(uint16_t)]));

    // The next four bytes are the message length.
    encode_int32((int32_t)length, &(header_buffer[1 + sizeof(uint16_t) + sizeof(uint16_t)]));

    LF_PRINT_LOG("Sending untagged message to %s.", next_destination_str);

    // Header:  message_type + port_id + federate_id + length of message + timestamp + microstep
    const int header_length = 1 + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t);

    // Use a mutex lock to prevent multiple threads from simultaneously sending.
    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    
    int* socket = &_fed.sockets_for_outbound_p2p_connections[federate];

    // Trace the event when tracing is enabled
    tracepoint_federate_to_federate(send_P2P_MSG, _lf_my_fed_id, federate, NULL);

    int result = write_to_socket_close_on_error(socket, header_length, header_buffer);
    if (result == 0) {
        // Header sent successfully. Send the body.
        result = write_to_socket_close_on_error(socket, length, message);
    }
    if (result != 0) {
        // Message did not send. Since this is used for physical connections, this is not critical.
        lf_print_warning("Failed to send message to %s. Dropping the message.", next_destination_str);
    }
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
    return result;
}

tag_t lf_send_next_event_tag(environment_t* env, tag_t tag, bool wait_for_reply) {
    assert(env != GLOBAL_ENVIRONMENT);
    while (true) {
        if (!_fed.has_downstream && !_fed.has_upstream) {
            // This federate is not connected (except possibly by physical links)
            // so there is no need for the RTI to get involved.

            // NOTE: If the event queue is empty, then the time argument is either
            // the timeout_time or FOREVER. If -fast is also set, then
            // it matters whether there are upstream federates connected by physical
            // connections, which do not affect _fed.has_upstream. Perhaps we
            // should not return immediately because
            // then the execution will hit its timeout_time and fail to receive any
            // messages sent by upstream federates.
            // However, -fast is really incompatible with federated execution with
            // physical connections, so I don't think we need to worry about this.
            LF_PRINT_DEBUG("Granted tag " PRINTF_TAG " because the federate has neither "
                    "upstream nor downstream federates.",
                    tag.time - start_time, tag.microstep);
            return tag;
        }

        // If time advance (TAG or PTAG) has already been granted for this tag
        // or a larger tag, then return immediately.
        if (lf_tag_compare(_fed.last_TAG, tag) >= 0) {
            LF_PRINT_DEBUG("Granted tag " PRINTF_TAG " because TAG or PTAG has been received.",
                    _fed.last_TAG.time - start_time, _fed.last_TAG.microstep);
            return _fed.last_TAG;
        }

        // Copy the tag because bounded_NET() may modify it.
        tag_t original_tag = tag;

        // A NET sent by this function is a promise that, absent
        // inputs from another federate, this federate will not produce events
        // earlier than t. But if there are downstream federates and there is
        // a physical action (not counting receivers from upstream federates),
        // then we can only promise up to current physical time (plus the minimum
        // of all minimum delays on the physical actions).
        // If wait_for_reply is false, leave the tag alone.
        bool tag_bounded_by_physical_time = wait_for_reply ?
                bounded_NET(&tag)
                : false;

        // What we do next depends on whether the NET has been bounded by
        // physical time or by an event on the event queue.
        if (!tag_bounded_by_physical_time) {
            // This if statement does not fall through but rather returns.
            // NET is not bounded by physical time or has no downstream federates.
            // Normal case.
            send_tag(MSG_TYPE_NEXT_EVENT_TAG, tag);
            _fed.last_sent_NET = tag;
            LF_PRINT_LOG("Sent next event tag (NET) " PRINTF_TAG " to RTI.",
                    tag.time - start_time, tag.microstep);

            if (!wait_for_reply) {
                LF_PRINT_LOG("Not waiting for reply to NET.");
                return tag;
            }

            // If there are no upstream federates, return immediately, without
            // waiting for a reply. This federate does not need to wait for
            // any other federate.
            // NOTE: If fast execution is being used, it may be necessary to
            // throttle upstream federates.
            if (!_fed.has_upstream) {
                LF_PRINT_DEBUG("Not waiting for reply to NET " PRINTF_TAG " because I "
                        "have no upstream federates.",
                        tag.time - start_time, tag.microstep);
                return tag;
            }

            // Wait until a TAG is received from the RTI.
            while (true) {
                // Wait until either something changes on the event queue or
                // the RTI has responded with a TAG.
                LF_PRINT_DEBUG("Waiting for a TAG from the RTI with _fed.last_TAG= " PRINTF_TAG " and net=" PRINTF_TAG,
                        _fed.last_TAG.time - start_time, _fed.last_TAG.microstep,
                        tag.time - start_time, tag.microstep);
                if (lf_cond_wait(&env->event_q_changed) != 0) {
                    lf_print_error("Wait error.");
                }
                // Check whether the new event on the event queue requires sending a new NET.
                tag_t next_tag = get_next_event_tag(env);
                if (
                    lf_tag_compare(_fed.last_TAG, next_tag) >= 0
                    || lf_tag_compare(_fed.last_TAG, tag) >= 0
                ) {
                    return _fed.last_TAG;
                }
                if (lf_tag_compare(next_tag, tag) != 0) {
                    send_tag(MSG_TYPE_NEXT_EVENT_TAG, next_tag);
                    _fed.last_sent_NET = next_tag;
                    LF_PRINT_LOG("Sent next event tag (NET) " PRINTF_TAG " to RTI from loop.",
                        next_tag.time - lf_time_start(), next_tag.microstep);
                }
            }
        }

        if (tag.time != FOREVER) {
            // Create a dummy event that will force this federate to advance time and subsequently
            // enable progress for downstream federates. Increment the time by ADVANCE_MESSAGE_INTERVAL
            // to prevent too frequent dummy events.
            event_t* dummy = _lf_create_dummy_events(env, NULL, tag.time + ADVANCE_MESSAGE_INTERVAL, NULL, 0);
            pqueue_insert(env->event_q, dummy);
        }

        LF_PRINT_DEBUG("Inserted a dummy event for logical time " PRINTF_TIME ".",
                tag.time - lf_time_start());

        if (!wait_for_reply) {
            LF_PRINT_LOG("Not waiting for physical time to advance further.");
            return tag;
        }

        // This federate should repeatedly advance its tag to ensure downstream federates can make progress.
        // Before advancing to the next tag, we need to wait some time so that we don't overwhelm the network and the
        // RTI. That amount of time will be no greater than ADVANCE_MESSAGE_INTERVAL in the future.
        LF_PRINT_DEBUG("Waiting for physical time to elapse or an event on the event queue.");

        instant_t wait_until_time_ns = lf_time_physical() + ADVANCE_MESSAGE_INTERVAL;

        // Regardless of the ADVANCE_MESSAGE_INTERVAL, do not let this
        // wait exceed the time of the next tag.
        if (wait_until_time_ns > original_tag.time) {
            wait_until_time_ns = original_tag.time;
        }

        lf_clock_cond_timedwait(&env->event_q_changed, wait_until_time_ns);

        LF_PRINT_DEBUG("Wait finished or interrupted.");

        // Either the timeout expired or the wait was interrupted by an event being
        // put onto the event queue. In either case, we can just loop around.
        // The next iteration will determine whether another
        // NET should be sent or not.
        tag = get_next_event_tag(env);
    }
}

void lf_send_port_absent_to_federate(
        environment_t* env,
        interval_t additional_delay,
        unsigned short port_ID,
        unsigned short fed_ID) {
    assert(env != GLOBAL_ENVIRONMENT);

    // Construct the message
    size_t message_length = 1 + sizeof(port_ID) + sizeof(fed_ID) + sizeof(instant_t) + sizeof(microstep_t);
    unsigned char buffer[message_length];

    // Apply the additional delay to the current tag and use that as the intended
    // tag of the outgoing message. Note that if there is delay on the connection,
    // then we cannot promise no message with tag = current_tag + delay because a
    // subsequent reaction might produce such a message. But we can promise no
    // message with a tag strictly less than current_tag + delay.
    tag_t current_message_intended_tag = lf_delay_strict(env->current_tag, additional_delay);

    LF_PRINT_LOG("Sending port "
            "absent for tag " PRINTF_TAG " for port %d to federate %d.",
            current_message_intended_tag.time - start_time,
            current_message_intended_tag.microstep,
            port_ID, fed_ID);

    buffer[0] = MSG_TYPE_PORT_ABSENT;
    encode_uint16(port_ID, &(buffer[1]));
    encode_uint16(fed_ID, &(buffer[1+sizeof(port_ID)]));
    encode_tag(&(buffer[1+sizeof(port_ID)+sizeof(fed_ID)]), current_message_intended_tag);

#ifdef FEDERATED_CENTRALIZED
    // Send the absent message through the RTI
    int* socket = &_fed.socket_TCP_RTI;
#else
    // Send the absent message directly to the federate
    int* socket = &_fed.sockets_for_outbound_p2p_connections[fed_ID];
#endif

    if (socket == &_fed.socket_TCP_RTI) {
        tracepoint_federate_to_rti(
                send_PORT_ABS, _lf_my_fed_id, &current_message_intended_tag);
    } else {
        tracepoint_federate_to_federate(
                send_PORT_ABS, _lf_my_fed_id, fed_ID, &current_message_intended_tag);
    }

    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    int result = write_to_socket_close_on_error(socket, message_length, buffer);
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);

    if (result != 0) {
        // Write failed. Response depends on whether coordination is centralized.
        if (socket == &_fed.socket_TCP_RTI) {
            // Centralized coordination. This is a critical error.
            lf_print_error_system_failure("Failed to send port absent message for port %hu to federate %hu.",
                    port_ID, fed_ID);
        } else {
            // Decentralized coordination. This is not a critical error.
            lf_print_warning("Failed to send port absent message for port %hu to federate %hu.",
                    port_ID, fed_ID);
        }
    }
}

int lf_send_stop_request_to_rti(tag_t stop_tag) {

    // Send a stop request with the specified tag to the RTI
    unsigned char buffer[MSG_TYPE_STOP_REQUEST_LENGTH];
    // Stop at the next microstep
    stop_tag.microstep++;
    ENCODE_STOP_REQUEST(buffer, stop_tag.time, stop_tag.microstep);

    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);
    // Do not send a stop request if a stop request has been previously received from the RTI.
    if (!_fed.received_stop_request_from_rti) {
        LF_PRINT_LOG("Sending to RTI a MSG_TYPE_STOP_REQUEST message with tag " PRINTF_TAG ".",
                stop_tag.time - start_time,
                stop_tag.microstep);

        if (_fed.socket_TCP_RTI < 0) {
            lf_print_warning("Socket is no longer connected. Dropping message.");
            LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
            return -1;
        }
        // Trace the event when tracing is enabled
        tracepoint_federate_to_rti(send_STOP_REQ, _lf_my_fed_id, &stop_tag);

        write_to_socket_fail_on_error(&_fed.socket_TCP_RTI, MSG_TYPE_STOP_REQUEST_LENGTH,
                buffer, &lf_outbound_socket_mutex,
                "Failed to send stop time " PRINTF_TIME " to the RTI.", stop_tag.time - start_time);

        // Treat this sending  as equivalent to having received a stop request from the RTI.
        _fed.received_stop_request_from_rti = true;
        LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
        return 0;
    } else {
        LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
        return 1;
    }
}

int lf_send_tagged_message(environment_t* env,
                        interval_t additional_delay,
                        int message_type,
                        unsigned short port,
                        unsigned short federate,
                        const char* next_destination_str,
                        size_t length,
                        unsigned char* message) {
    assert(env != GLOBAL_ENVIRONMENT);

    size_t header_length = 1 + sizeof(uint16_t) + sizeof(uint16_t)
            + sizeof(int32_t) + sizeof(instant_t) + sizeof(microstep_t);
    unsigned char header_buffer[header_length];

    if (message_type != MSG_TYPE_TAGGED_MESSAGE && message_type != MSG_TYPE_P2P_TAGGED_MESSAGE) {
        lf_print_error("lf_send_message: Unsupported message type (%d).", message_type);
        return -1;
    }

    size_t buffer_head = 0;
    // First byte is the message type.
    header_buffer[buffer_head] = (unsigned char)message_type;
    buffer_head += sizeof(unsigned char);
    // Next two bytes identify the destination port.
    // NOTE: Send messages little endian, not big endian.
    encode_uint16(port, &(header_buffer[buffer_head]));
    buffer_head += sizeof(uint16_t);

    // Next two bytes identify the destination federate.
    encode_uint16(federate, &(header_buffer[buffer_head]));
    buffer_head += sizeof(uint16_t);

    // The next four bytes are the message length.
    encode_int32((int32_t)length, &(header_buffer[buffer_head]));
    buffer_head += sizeof(int32_t);

    // Apply the additional delay to the current tag and use that as the intended
    // tag of the outgoing message.
    tag_t current_message_intended_tag = lf_delay_tag(env->current_tag, additional_delay);

    if (lf_is_tag_after_stop_tag(env, current_message_intended_tag)) {
        // Message tag is past the timeout time (the stop time) so it should not be sent.
        LF_PRINT_LOG("Dropping message because it will be after the timeout time.");
        return -1;
    }

    // Next 8 + 4 will be the tag (timestamp, microstep)
    encode_tag(
        &(header_buffer[buffer_head]),
        current_message_intended_tag
    );

    LF_PRINT_LOG("Sending message with tag " PRINTF_TAG " to %s.",
            current_message_intended_tag.time - start_time,
            current_message_intended_tag.microstep,
            next_destination_str);

    // Use a mutex lock to prevent multiple threads from simultaneously sending.
    LF_MUTEX_LOCK(&lf_outbound_socket_mutex);

    int* socket;
    if (message_type == MSG_TYPE_P2P_TAGGED_MESSAGE) {
        socket = &_fed.sockets_for_outbound_p2p_connections[federate];
        tracepoint_federate_to_federate(send_P2P_TAGGED_MSG, _lf_my_fed_id, federate, &current_message_intended_tag);
    } else {
        socket = &_fed.socket_TCP_RTI;
        tracepoint_federate_to_rti(send_TAGGED_MSG, _lf_my_fed_id, &current_message_intended_tag);
    }

    int result = write_to_socket_close_on_error(socket, header_length, header_buffer);
    if (result == 0) {
        // Header sent successfully. Send the body.
        result = write_to_socket_close_on_error(socket, length, message);
    }
    if (result != 0) {
        // Message did not send. Handling depends on message type.
        if (message_type == MSG_TYPE_P2P_TAGGED_MESSAGE) {
            lf_print_warning("Failed to send message to %s. Dropping the message.", next_destination_str);
        } else {
            lf_print_error_system_failure("Failed to send message to %s. Connection lost to the RTI.",
                    next_destination_str);
        }
    }
    LF_MUTEX_UNLOCK(&lf_outbound_socket_mutex);
    return result;
}

void lf_set_federation_id(const char* fid) {
    federation_metadata.federation_id = fid;
}

#ifdef FEDERATED_DECENTRALIZED
void lf_spawn_staa_thread(){
    lf_thread_create(&_fed.staaSetter, update_ports_from_staa_offsets, NULL);
}
#endif // FEDERATED_DECENTRALIZED

void lf_stall_advance_level_federation(environment_t* env, size_t level) {
    LF_PRINT_DEBUG("Acquiring the environment mutex.");
    LF_MUTEX_LOCK(&env->mutex);
    LF_PRINT_DEBUG("Waiting on MLAA with next_reaction_level %zu and MLAA %d.", level, max_level_allowed_to_advance);
    while (((int) level) >= max_level_allowed_to_advance) {
        lf_cond_wait(&lf_port_status_changed);
    };
    LF_PRINT_DEBUG("Exiting wait with MLAA %d and next_reaction_level %zu.", max_level_allowed_to_advance, level);
    LF_MUTEX_UNLOCK(&env->mutex);
}

void lf_synchronize_with_other_federates(void) {

    LF_PRINT_DEBUG("Synchronizing with other federates.");

    // Reset the start time to the coordinated start time for all federates.
    // Note that this does not grant execution to this federate.
    start_time = get_start_time_from_rti(lf_time_physical());
    lf_tracing_set_start_time(start_time);

    // Start a thread to listen for incoming TCP messages from the RTI.
    // @note Up until this point, the federate has been listening for messages
    //  from the RTI in a sequential manner in the main thread. From now on, a
    //  separate thread is created to allow for asynchronous communication.
    lf_thread_create(&_fed.RTI_socket_listener, listen_to_rti_TCP, NULL);
    lf_thread_t thread_id;
    if (create_clock_sync_thread(&thread_id)) {
        lf_print_warning("Failed to create thread to handle clock synchronization.");
    }
}

bool lf_update_max_level(tag_t tag, bool is_provisional) {
    // This always needs the top-level environment, which will be env[0].
    environment_t *env;
    _lf_get_environments(&env);
    int prev_max_level_allowed_to_advance = max_level_allowed_to_advance;
    max_level_allowed_to_advance = INT_MAX;
#ifdef FEDERATED_DECENTRALIZED
    size_t action_table_size = _lf_action_table_size;
    lf_action_base_t** action_table = _lf_action_table;
#else
    // Note that the following test is never true for decentralized coordination,
    // where tag always is NEVER_TAG.
    if ((lf_tag_compare(env->current_tag, tag) < 0) || (
        lf_tag_compare(env->current_tag, tag) == 0 && !is_provisional
    )) {
        LF_PRINT_DEBUG("Updated MLAA to %d at time " PRINTF_TIME ".",
              max_level_allowed_to_advance,
              lf_time_logical_elapsed(env)
        );
        // Safe to complete the current tag
        return (prev_max_level_allowed_to_advance != max_level_allowed_to_advance);
    }

    size_t action_table_size = _lf_zero_delay_cycle_action_table_size;
    lf_action_base_t** action_table = _lf_zero_delay_cycle_action_table;
#endif // FEDERATED_DECENTRALIZED
    for (int i = 0; i < action_table_size; i++) {
        lf_action_base_t* input_port_action = action_table[i];
#ifdef FEDERATED_DECENTRALIZED
        // In decentralized execution, if the current_tag is close enough to the
        // start tag and there is a large enough delay on an incoming
        // connection, then there is no need to block progress waiting for this
        // port status.  This is irrelevant for centralized because blocking only
        // occurs on zero-delay cycles.
        if (
            (_lf_action_delay_table[i] == 0 && env->current_tag.time == start_time && env->current_tag.microstep == 0)
            || (_lf_action_delay_table[i] > 0 && lf_tag_compare(
                env->current_tag,
                lf_delay_strict((tag_t) {.time=start_time, .microstep=0}, _lf_action_delay_table[i])
            ) <= 0)
        ) {
            continue;
        }
#endif // FEDERATED_DECENTRALIZED
        // If the current tag is greater than the last known status tag of the input port,
        // and the input port is not physical, then block on that port by ensuring
        // the MLAA is no greater than the level of that port.
        // For centralized coordination, this is applied only to input ports coming from
        // federates that are in a ZDC.  For decentralized coordination, this is applied
        // to all input ports.
        if (lf_tag_compare(env->current_tag,
                input_port_action->trigger->last_known_status_tag) > 0
                && !input_port_action->trigger->is_physical) {
            max_level_allowed_to_advance = LF_MIN(
                max_level_allowed_to_advance,
                ((int) LF_LEVEL(input_port_action->trigger->reactions[0]->index))
            );
        }
    }
    LF_PRINT_DEBUG("Updated MLAA to %d at time " PRINTF_TIME ".",
        max_level_allowed_to_advance,
        lf_time_logical_elapsed(env)
    );
    return (prev_max_level_allowed_to_advance != max_level_allowed_to_advance);
}

#endif
