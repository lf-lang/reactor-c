#if defined STANDALONE_RTI
/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Erling Jellum
 * @author Chadlia Jerad
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Runtime infrastructure (RTI) for distributed Lingua Franca programs.
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
#include <string.h>

// Global variables defined in tag.c:
extern instant_t start_time;

/**
 * Local reference to the rti_remote object
 */
static rti_remote_t *rti_remote;

bool _lf_federate_reports_error = false;

// A convenient macro for getting the `federate_info_t *` at index `_idx`
// and casting it.
#define GET_FED_INFO(_idx) (federate_info_t *)rti_remote->base.scheduling_nodes[_idx]

lf_mutex_t rti_mutex;
lf_cond_t received_start_times;
lf_cond_t sent_start_time;

extern int lf_critical_section_enter(environment_t *env) {
    return lf_mutex_lock(&rti_mutex);
}

extern int lf_critical_section_exit(environment_t *env) {
    return lf_mutex_unlock(&rti_mutex);
}

/**
 * Create a server and enable listening for socket connections.
 * If the specified port if it is non-zero, it will attempt to acquire that port.
 * If it fails, it will repeatedly attempt up to PORT_BIND_RETRY_LIMIT times with
 * a delay of PORT_BIND_RETRY_INTERVAL in between. If the specified port is
 * zero, then it will attempt to acquire DEFAULT_PORT first. If this fails, then it
 * will repeatedly attempt up to PORT_BIND_RETRY_LIMIT times, incrementing the port
 * number between attempts, with no delay between attempts.  Once it has incremented
 * the port number MAX_NUM_PORT_ADDRESSES times, it will cycle around and begin again
 * with DEFAULT_PORT.
 *
 * @param port The port number to use or 0 to start trying at DEFAULT_PORT.
 * @param socket_type The type of the socket for the server (TCP or UDP).
 * @return The socket descriptor on which to accept connections.
 */
static int create_rti_server(uint16_t port, socket_type_t socket_type) {
    // Timeout time for the communications of the server
    struct timeval timeout_time = {
            .tv_sec = TCP_TIMEOUT_TIME / BILLION,
            .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000
    };
    // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
    int socket_descriptor = -1;
    if (socket_type == TCP) {
        socket_descriptor = create_real_time_tcp_socket_errexit();
    } else if (socket_type == UDP) {
        socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // Set the appropriate timeout time
        timeout_time = (struct timeval){
                .tv_sec = UDP_TIMEOUT_TIME / BILLION,
                .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000
        };
    }
    if (socket_descriptor < 0) {
        lf_print_error_system_failure("Failed to create RTI socket.");
    }

    // Set the option for this socket to reuse the same address
    int true_variable = 1; // setsockopt() requires a reference to the value assigned to an option
    if (setsockopt(
            socket_descriptor,
            SOL_SOCKET,
            SO_REUSEADDR,
            &true_variable,
            sizeof(int32_t)) < 0) {
        lf_print_error("RTI failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
    }
    // Set the timeout on the socket so that read and write operations don't block for too long
    if (setsockopt(
            socket_descriptor,
            SOL_SOCKET,
            SO_RCVTIMEO,
            (const char *)&timeout_time,
            sizeof(timeout_time)) < 0) {
        lf_print_error("RTI failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
    }
    if (setsockopt(
            socket_descriptor,
            SOL_SOCKET,
            SO_SNDTIMEO,
            (const char *)&timeout_time,
            sizeof(timeout_time)) < 0) {
        lf_print_error("RTI failed to set SO_SNDTIMEO option on the socket: %s.", strerror(errno));
    }

    /*
     * The following used to permit reuse of a port that an RTI has previously
     * used that has not been released. We no longer do this, and instead retry
     * some number of times after waiting.

    // SO_REUSEPORT (since Linux 3.9)
    //       Permits multiple AF_INET or AF_INET6 sockets to be bound to an
    //       identical socket address.  This option must be set on each
    //       socket (including the first socket) prior to calling bind(2)
    //       on the socket.  To prevent port hijacking, all of the
    //       processes binding to the same address must have the same
    //       effective UID.  This option can be employed with both TCP and
    //       UDP sockets.

    int reuse = 1;
    #ifdef SO_REUSEPORT
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEPORT,
            (const char*)&reuse, sizeof(reuse)) < 0)  {
        perror("setsockopt(SO_REUSEPORT) failed");
    }
    #endif
    */

    // Server file descriptor.
    struct sockaddr_in server_fd;
    // Zero out the server address structure.
    bzero((char *)&server_fd, sizeof(server_fd));

    uint16_t specified_port = port;
    if (specified_port == 0) port = DEFAULT_PORT;

    server_fd.sin_family = AF_INET;         // IPv4
    server_fd.sin_addr.s_addr = INADDR_ANY; // All interfaces, 0.0.0.0.
    // Convert the port number from host byte order to network byte order.
    server_fd.sin_port = htons(port);

    int result = bind(
            socket_descriptor,
            (struct sockaddr *)&server_fd,
            sizeof(server_fd));

    // Try repeatedly to bind to a port. If no specific port is specified, then
    // increment the port number each time.

    int count = 1;
    while (result != 0 && count++ < PORT_BIND_RETRY_LIMIT) {
        if (specified_port == 0) {
            lf_print_warning("RTI failed to get port %d.", port);
            port++;
            if (port >= DEFAULT_PORT + MAX_NUM_PORT_ADDRESSES) port = DEFAULT_PORT;
            lf_print_warning("RTI will try again with port %d.", port);
            server_fd.sin_port = htons(port);
            // Do not sleep.
        } else {
            lf_print("RTI failed to get port %d. Will try again.", port);
            lf_sleep(PORT_BIND_RETRY_INTERVAL);
        }
        result = bind(
            socket_descriptor,
            (struct sockaddr *)&server_fd,
            sizeof(server_fd));
    }
    if (result != 0) {
        lf_print_error_and_exit("Failed to bind the RTI socket. Port %d is not available. ", port);
    }
    char *type = "TCP";
    if (socket_type == UDP) {
        type = "UDP";
    }
    lf_print("RTI using %s port %d for federation %s.", type, port, rti_remote->federation_id);

    if (socket_type == TCP) {
        rti_remote->final_port_TCP = port;
        // Enable listening for socket connections.
        // The second argument is the maximum number of queued socket requests,
        // which according to the Mac man page is limited to 128.
        listen(socket_descriptor, 128);
    } else if (socket_type == UDP) {
        rti_remote->final_port_UDP = port;
        // No need to listen on the UDP socket
    }

    return socket_descriptor;
}

void notify_tag_advance_grant(scheduling_node_t *e, tag_t tag) {
    if (e->state == NOT_CONNECTED
            || lf_tag_compare(tag, e->last_granted) <= 0
            || lf_tag_compare(tag, e->last_provisionally_granted) < 0) {
        return;
    }
    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (e->state == PENDING) {
        // Need to wait here.
        lf_cond_wait(&sent_start_time);
    }
    size_t message_length = 1 + sizeof(int64_t) + sizeof(uint32_t);
    unsigned char buffer[message_length];
    buffer[0] = MSG_TYPE_TAG_ADVANCE_GRANT;
    encode_int64(tag.time, &(buffer[1]));
    encode_int32((int32_t)tag.microstep, &(buffer[1 + sizeof(int64_t)]));

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_to_federate(rti_remote->base.trace, send_TAG, e->id, &tag);
    }
    // This function is called in notify_advance_grant_if_safe(), which is a long
    // function. During this call, the socket might close, causing the following write_to_socket
    // to fail. Consider a failure here a soft failure and update the federate's status.
    if (write_to_socket(((federate_info_t *)e)->socket, message_length, buffer)) {
        lf_print_error("RTI failed to send tag advance grant to federate %d.", e->id);
        e->state = NOT_CONNECTED;
    } else {
        e->last_granted = tag;
        LF_PRINT_LOG("RTI sent to federate %d the tag advance grant (TAG) " PRINTF_TAG ".",
                     e->id, tag.time - start_time, tag.microstep);
    }
}

void notify_provisional_tag_advance_grant(scheduling_node_t *e, tag_t tag) {
    if (e->state == NOT_CONNECTED
            || lf_tag_compare(tag, e->last_granted) <= 0
            || lf_tag_compare(tag, e->last_provisionally_granted) <= 0) {
        return;
    }
    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (e->state == PENDING) {
        // Need to wait here.
        lf_cond_wait(&sent_start_time);
    }
    size_t message_length = 1 + sizeof(int64_t) + sizeof(uint32_t);
    unsigned char buffer[message_length];
    buffer[0] = MSG_TYPE_PROVISIONAL_TAG_ADVANCE_GRANT;
    encode_int64(tag.time, &(buffer[1]));
    encode_int32((int32_t)tag.microstep, &(buffer[1 + sizeof(int64_t)]));

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_to_federate(rti_remote->base.trace, send_PTAG, e->id, &tag);
    }
    // This function is called in notify_advance_grant_if_safe(), which is a long
    // function. During this call, the socket might close, causing the following write_to_socket
    // to fail. Consider a failure here a soft failure and update the federate's status.
    if (write_to_socket(((federate_info_t *)e)->socket, message_length, buffer)) {
        lf_print_error("RTI failed to send tag advance grant to federate %d.", e->id);
        e->state = NOT_CONNECTED;
    } else {
        e->last_provisionally_granted = tag;
        LF_PRINT_LOG("RTI sent to federate %d the Provisional Tag Advance Grant (PTAG) " PRINTF_TAG ".",
                     e->id, tag.time - start_time, tag.microstep);

        // Send PTAG to all upstream federates, if they have not had
        // a later or equal PTAG or TAG sent previously and if their transitive
        // NET is greater than or equal to the tag.
        // This is needed to stimulate absent messages from upstream and break deadlocks.
        // The scenario this deals with is illustrated in `test/C/src/federated/FeedbackDelay2.lf`
        // and `test/C/src/federated/FeedbackDelay4.lf`.
        // Note that this is transitive.
        // NOTE: This is not needed for enclaves because zero-delay loops are prohibited.
        // It's only needed for federates, which is why this is implemented here.
        for (int j = 0; j < e->num_upstream; j++) {
            scheduling_node_t *upstream = rti_remote->base.scheduling_nodes[e->upstream[j]];

            // Ignore this federate if it has resigned.
            if (upstream->state == NOT_CONNECTED)
                continue;

            tag_t earliest = earliest_future_incoming_message_tag(upstream);
            tag_t strict_earliest = eimt_strict(upstream);  // Non-ZDC version.

            // If these tags are equal, then a TAG or PTAG should have already been granted,
            // in which case, another will not be sent. But it may not have been already granted.
            if (lf_tag_compare(earliest, tag) > 0) {
                notify_tag_advance_grant(upstream, tag);
            } else if (lf_tag_compare(earliest, tag) == 0 && lf_tag_compare(strict_earliest, tag) > 0) {
                notify_provisional_tag_advance_grant(upstream, tag);
            }
        }
    }
}

void update_federate_next_event_tag_locked(uint16_t federate_id, tag_t next_event_tag) {
    federate_info_t *fed = GET_FED_INFO(federate_id);
    tag_t min_in_transit_tag = pqueue_tag_peek_tag(fed->in_transit_message_tags);
    if (lf_tag_compare(min_in_transit_tag, next_event_tag) < 0) {
        next_event_tag = min_in_transit_tag;
    }
    update_scheduling_node_next_event_tag_locked(&(fed->enclave), next_event_tag);
}

void handle_port_absent_message(federate_info_t *sending_federate, unsigned char *buffer) {
    size_t message_size = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int64_t) + sizeof(uint32_t);

    read_from_socket_fail_on_error(
            &sending_federate->socket, message_size, &(buffer[1]), NULL,
            " RTI failed to read port absent message from federate %u.",
            sending_federate->enclave.id);

    uint16_t reactor_port_id = extract_uint16(&(buffer[1]));
    uint16_t federate_id = extract_uint16(&(buffer[1 + sizeof(uint16_t)]));
    tag_t tag = extract_tag(&(buffer[1 + 2 * sizeof(uint16_t)]));

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_PORT_ABS, sending_federate->enclave.id, &tag);
    }

    // Need to acquire the mutex lock to ensure that the thread handling
    // messages coming from the socket connected to the destination does not
    // issue a TAG before this message has been forwarded.
    LF_MUTEX_LOCK(&rti_mutex);

    // If the destination federate is no longer connected, issue a warning
    // and return.
    federate_info_t *fed = GET_FED_INFO(federate_id);
    if (fed->enclave.state == NOT_CONNECTED) {
        LF_MUTEX_UNLOCK(&rti_mutex);
        lf_print_warning("RTI: Destination federate %d is no longer connected. Dropping message.",
                         federate_id);
        LF_PRINT_LOG("Fed status: next_event " PRINTF_TAG ", "
                     "completed " PRINTF_TAG ", "
                     "last_granted " PRINTF_TAG ", "
                     "last_provisionally_granted " PRINTF_TAG ".",
                     fed->enclave.next_event.time - start_time,
                     fed->enclave.next_event.microstep,
                     fed->enclave.completed.time - start_time,
                     fed->enclave.completed.microstep,
                     fed->enclave.last_granted.time - start_time,
                     fed->enclave.last_granted.microstep,
                     fed->enclave.last_provisionally_granted.time - start_time,
                     fed->enclave.last_provisionally_granted.microstep);
        return;
    }

    LF_PRINT_LOG("RTI forwarding port absent message for port %u to federate %u.",
                 reactor_port_id,
                 federate_id);

    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (fed->enclave.state == PENDING) {
        // Need to wait here.
        lf_cond_wait(&sent_start_time);
    }

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_to_federate(rti_remote->base.trace, send_PORT_ABS, federate_id, &tag);
    }

    // Forward the message.
    write_to_socket_fail_on_error(&fed->socket, message_size + 1, buffer, &rti_mutex,
                                  "RTI failed to forward message to federate %d.", federate_id);

    LF_MUTEX_UNLOCK(&rti_mutex);
}

void handle_timed_message(federate_info_t *sending_federate, unsigned char *buffer) {
    size_t header_size = 1 + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t)
            + sizeof(int64_t) + sizeof(uint32_t);
    // Read the header, minus the first byte which has already been read.
    read_from_socket_fail_on_error(
            &sending_federate->socket, header_size - 1, &(buffer[1]), NULL,
            "RTI failed to read the timed message header from remote federate.");
    // Extract the header information. of the sender
    uint16_t reactor_port_id;
    uint16_t federate_id;
    size_t length;
    tag_t intended_tag;
    // Extract information from the header.
    extract_timed_header(&(buffer[1]), &reactor_port_id, &federate_id, &length, &intended_tag);

    size_t total_bytes_to_read = length + header_size;
    size_t bytes_to_read = length;

    if (FED_COM_BUFFER_SIZE < header_size + 1) {
        lf_print_error_and_exit("Buffer size (%d) is not large enough to "
                                "read the header plus one byte.",
                                FED_COM_BUFFER_SIZE);
    }

    // Cut up the payload in chunks.
    if (bytes_to_read > FED_COM_BUFFER_SIZE - header_size) {
        bytes_to_read = FED_COM_BUFFER_SIZE - header_size;
    }

    LF_PRINT_LOG("RTI received message from federate %d for federate %u port %u with intended tag " PRINTF_TAG ". Forwarding.",
                 sending_federate->enclave.id, federate_id, reactor_port_id,
                 intended_tag.time - lf_time_start(), intended_tag.microstep);

    read_from_socket_fail_on_error(
            &sending_federate->socket, bytes_to_read, &(buffer[header_size]), NULL,
            "RTI failed to read timed message from federate %d.", federate_id);
    size_t bytes_read = bytes_to_read + header_size;
    // Following only works for string messages.
    // LF_PRINT_DEBUG("Message received by RTI: %s.", buffer + header_size);

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_TAGGED_MSG, sending_federate->enclave.id, &intended_tag);
    }

    // Need to acquire the mutex lock to ensure that the thread handling
    // messages coming from the socket connected to the destination does not
    // issue a TAG before this message has been forwarded.
    LF_MUTEX_LOCK(&rti_mutex);

    // If the destination federate is no longer connected, issue a warning
    // and return.
    federate_info_t *fed = GET_FED_INFO(federate_id);
    if (fed->enclave.state == NOT_CONNECTED) {
        LF_MUTEX_UNLOCK(&rti_mutex);
        lf_print_warning("RTI: Destination federate %d is no longer connected. Dropping message.",
                         federate_id);
        LF_PRINT_LOG("Fed status: next_event " PRINTF_TAG ", "
                     "completed " PRINTF_TAG ", "
                     "last_granted " PRINTF_TAG ", "
                     "last_provisionally_granted " PRINTF_TAG ".",
                     fed->enclave.next_event.time - start_time,
                     fed->enclave.next_event.microstep,
                     fed->enclave.completed.time - start_time,
                     fed->enclave.completed.microstep,
                     fed->enclave.last_granted.time - start_time,
                     fed->enclave.last_granted.microstep,
                     fed->enclave.last_provisionally_granted.time - start_time,
                     fed->enclave.last_provisionally_granted.microstep);
        return;
    }

    LF_PRINT_DEBUG(
        "RTI forwarding message to port %d of federate %hu of length %zu.",
        reactor_port_id,
        federate_id,
        length);

    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (fed->enclave.state == PENDING) {
        // Need to wait here.
        lf_cond_wait(&sent_start_time);
    }

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_to_federate(rti_remote->base.trace, send_TAGGED_MSG, federate_id, &intended_tag);
    }

    write_to_socket_fail_on_error(&fed->socket, bytes_read, buffer, &rti_mutex,
            "RTI failed to forward message to federate %d.", federate_id);

    // The message length may be longer than the buffer,
    // in which case we have to handle it in chunks.
    size_t total_bytes_read = bytes_read;
    while (total_bytes_read < total_bytes_to_read) {
        LF_PRINT_DEBUG("Forwarding message in chunks.");
        bytes_to_read = total_bytes_to_read - total_bytes_read;
        if (bytes_to_read > FED_COM_BUFFER_SIZE) {
            bytes_to_read = FED_COM_BUFFER_SIZE;
        }
        read_from_socket_fail_on_error(&sending_federate->socket, bytes_to_read, buffer, NULL,
                "RTI failed to read message chunks.");
        total_bytes_read += bytes_to_read;

        // FIXME: a mutex needs to be held for this so that other threads
        // do not write to destination_socket and cause interleaving. However,
        // holding the rti_mutex might be very expensive. Instead, each outgoing
        // socket should probably have its own mutex.
        write_to_socket_fail_on_error(&fed->socket, bytes_to_read, buffer, &rti_mutex,
                                      "RTI failed to send message chunks.");
    }

    // Record this in-transit message in federate's in-transit message queue.
    if (lf_tag_compare(fed->enclave.completed, intended_tag) < 0) {
        // Add a record of this message to the list of in-transit messages to this federate.
        pqueue_tag_insert_if_no_match(
            fed->in_transit_message_tags,
            intended_tag);
        LF_PRINT_DEBUG(
            "RTI: Adding a message with tag " PRINTF_TAG " to the list of in-transit messages for federate %d.",
            intended_tag.time - lf_time_start(),
            intended_tag.microstep,
            federate_id);
    } else {
        lf_print_error(
            "RTI: Federate %d has already completed tag " PRINTF_TAG
            ", but there is an in-transit message with tag " PRINTF_TAG " from federate %hu. "
            "This is going to cause an STP violation under centralized coordination.",
            federate_id,
            fed->enclave.completed.time - lf_time_start(),
            fed->enclave.completed.microstep,
            intended_tag.time - lf_time_start(),
            intended_tag.microstep,
            sending_federate->enclave.id);
        // FIXME: Drop the federate?
    }

    // If the message tag is less than the most recently received NET from the federate,
    // then update the federate's next event tag to match the message tag.
    if (lf_tag_compare(intended_tag, fed->enclave.next_event) < 0) {
        update_federate_next_event_tag_locked(federate_id, intended_tag);
    }

    LF_MUTEX_UNLOCK(&rti_mutex);
}

void handle_latest_tag_complete(federate_info_t *fed) {
    unsigned char buffer[sizeof(int64_t) + sizeof(uint32_t)];
    read_from_socket_fail_on_error(&fed->socket, sizeof(int64_t) + sizeof(uint32_t), buffer, NULL,
            "RTI failed to read the content of the logical tag complete from federate %d.",
            fed->enclave.id);
    tag_t completed = extract_tag(buffer);
    if (rti_remote->base.tracing_enabled)     {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_LTC, fed->enclave.id, &completed);
    }
    _logical_tag_complete(&(fed->enclave), completed);

    // FIXME: Should this function be in the enclave version?
    LF_MUTEX_LOCK(&rti_mutex);
    // See if we can remove any of the recorded in-transit messages for this.
    pqueue_tag_remove_up_to(fed->in_transit_message_tags, completed);
    LF_MUTEX_UNLOCK(&rti_mutex);
}

void handle_next_event_tag(federate_info_t *fed) {
    unsigned char buffer[sizeof(int64_t) + sizeof(uint32_t)];
    read_from_socket_fail_on_error(&fed->socket, sizeof(int64_t) + sizeof(uint32_t), buffer, NULL,
            "RTI failed to read the content of the next event tag from federate %d.",
            fed->enclave.id);

    // Acquire a mutex lock to ensure that this state does not change while a
    // message is in transport or being used to determine a TAG.
    LF_MUTEX_LOCK(&rti_mutex); // FIXME: Instead of using a mutex, it might be more efficient to use a
                              // select() mechanism to read and process federates' buffers in an orderly fashion.

    tag_t intended_tag = extract_tag(buffer);
    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_NET, fed->enclave.id, &intended_tag);
    }
    LF_PRINT_LOG("RTI received from federate %d the Next Event Tag (NET) " PRINTF_TAG,
            fed->enclave.id, intended_tag.time - start_time,
            intended_tag.microstep);
    update_federate_next_event_tag_locked(
        fed->enclave.id,
        intended_tag);
    LF_MUTEX_UNLOCK(&rti_mutex);
}

/////////////////// STOP functions ////////////////////

/**
 * Boolean used to prevent the RTI from sending the
 * MSG_TYPE_STOP_GRANTED message multiple times.
 */
bool stop_granted_already_sent_to_federates = false;

/**
 * Once the RTI has seen proposed tags from all connected federates,
 * it will broadcast a MSG_TYPE_STOP_GRANTED carrying the _RTI.max_stop_tag.
 * This function also checks the most recently received NET from
 * each federate and resets that be no greater than the _RTI.max_stop_tag.
 *
 * This function assumes the caller holds the rti_mutex lock.
 */
static void broadcast_stop_time_to_federates_locked() {
    if (stop_granted_already_sent_to_federates == true) {
        return;
    }
    stop_granted_already_sent_to_federates = true;

    // Reply with a stop granted to all federates
    unsigned char outgoing_buffer[MSG_TYPE_STOP_GRANTED_LENGTH];
    ENCODE_STOP_GRANTED(outgoing_buffer, rti_remote->base.max_stop_tag.time, rti_remote->base.max_stop_tag.microstep);

    // Iterate over federates and send each the message.
    for (int i = 0; i < rti_remote->base.number_of_scheduling_nodes; i++) {
        federate_info_t *fed = GET_FED_INFO(i);
        if (fed->enclave.state == NOT_CONNECTED) {
            continue;
        }
        if (lf_tag_compare(fed->enclave.next_event, rti_remote->base.max_stop_tag) >= 0) {
            // Need the next_event to be no greater than the stop tag.
            fed->enclave.next_event = rti_remote->base.max_stop_tag;
        }
        if (rti_remote->base.tracing_enabled) {
            tracepoint_rti_to_federate(rti_remote->base.trace, send_STOP_GRN, fed->enclave.id, &rti_remote->base.max_stop_tag);
        }
        write_to_socket_fail_on_error(
                &fed->socket, MSG_TYPE_STOP_GRANTED_LENGTH, outgoing_buffer, &rti_mutex,
                "RTI failed to send MSG_TYPE_STOP_GRANTED message to federate %d.", fed->enclave.id);
    }

    LF_PRINT_LOG("RTI sent to federates MSG_TYPE_STOP_GRANTED with tag " PRINTF_TAG,
            rti_remote->base.max_stop_tag.time - start_time,
            rti_remote->base.max_stop_tag.microstep);
}

/**
 * Mark a federate requesting stop. If the number of federates handling stop reaches the
 * NUM_OF_FEDERATES, broadcast MSG_TYPE_STOP_GRANTED to every federate.
 * This function assumes the _RTI.mutex is already locked.
 * @param fed The federate that has requested a stop.
 * @return 1 if stop time has been sent to all federates and 0 otherwise.
 */
static int mark_federate_requesting_stop(federate_info_t *fed) {
    if (!fed->requested_stop) {
        rti_remote->base.num_scheduling_nodes_handling_stop++;
        fed->requested_stop = true;
    }
    if (rti_remote->base.num_scheduling_nodes_handling_stop
            == rti_remote->base.number_of_scheduling_nodes) {
        // We now have information about the stop time of all
        // federates.
        broadcast_stop_time_to_federates_locked();
        return 1;
    }
    return 0;
}

/**
 * Thread to time out if federates do not reply to stop request.
 */
static void* wait_for_stop_request_reply(void* args) {
    // Divide the time into small chunks and check periodically.
    interval_t chunk = MAX_TIME_FOR_REPLY_TO_STOP_REQUEST/30;
    int count = 0;
    while (count++ < 30) {
        if (stop_granted_already_sent_to_federates) return NULL;
        lf_sleep(chunk);
    }
    // If we reach here, then error out.
    lf_print_error_and_exit("Received only %d stop request replies within timeout "
            PRINTF_TIME "ns. RTI is exiting.",
            rti_remote->base.num_scheduling_nodes_handling_stop,
            MAX_TIME_FOR_REPLY_TO_STOP_REQUEST
    );
    return NULL;
}

void handle_stop_request_message(federate_info_t *fed) {
    LF_PRINT_DEBUG("RTI handling stop_request from federate %d.", fed->enclave.id);

    size_t bytes_to_read = MSG_TYPE_STOP_REQUEST_LENGTH - 1;
    unsigned char buffer[bytes_to_read];
    read_from_socket_fail_on_error(&fed->socket, bytes_to_read, buffer, NULL,
            "RTI failed to read the MSG_TYPE_STOP_REQUEST payload from federate %d.",
            fed->enclave.id);

    // Extract the proposed stop tag for the federate
    tag_t proposed_stop_tag = extract_tag(buffer);

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_STOP_REQ, fed->enclave.id, &proposed_stop_tag);
    }

    LF_PRINT_LOG("RTI received from federate %d a MSG_TYPE_STOP_REQUEST message with tag " PRINTF_TAG ".",
            fed->enclave.id, proposed_stop_tag.time - start_time, proposed_stop_tag.microstep);

    // Acquire a mutex lock to ensure that this state does change while a
    // message is in transport or being used to determine a TAG.
    LF_MUTEX_LOCK(&rti_mutex);

    // Check whether we have already received a stop_tag
    // from this federate
    if (fed->requested_stop) {
        // If stop request messages have already been broadcast, treat this as if it were a reply.
        if (rti_remote->stop_in_progress) {
            mark_federate_requesting_stop(fed);
        }
        LF_MUTEX_UNLOCK(&rti_mutex);
        return;
    }

    // Update the maximum stop tag received from federates
    if (lf_tag_compare(proposed_stop_tag, rti_remote->base.max_stop_tag) > 0) {
        rti_remote->base.max_stop_tag = proposed_stop_tag;
    }

    // If all federates have replied, send stop request granted.
    if (mark_federate_requesting_stop(fed)) {
        // Have send stop request granted to all federates. Nothing more to do.
        LF_MUTEX_UNLOCK(&rti_mutex);
        return;
    }

    // Forward the stop request to all other federates that have not
    // also issued a stop request.
    unsigned char stop_request_buffer[MSG_TYPE_STOP_REQUEST_LENGTH];
    ENCODE_STOP_REQUEST(stop_request_buffer,
            rti_remote->base.max_stop_tag.time, rti_remote->base.max_stop_tag.microstep);

    // Iterate over federates and send each the MSG_TYPE_STOP_REQUEST message
    // if we do not have a stop_time already for them. Do not do this more than once.
    if (rti_remote->stop_in_progress) {
        LF_MUTEX_UNLOCK(&rti_mutex);
        return;
    }
    rti_remote->stop_in_progress = true;
    // Need a timeout here in case a federate never replies.
    lf_thread_t timeout_thread;
    lf_thread_create(&timeout_thread, wait_for_stop_request_reply, NULL);
    
    for (int i = 0; i < rti_remote->base.number_of_scheduling_nodes; i++) {
        federate_info_t *f = GET_FED_INFO(i);
        if (f->enclave.id != fed->enclave.id && f->requested_stop == false) {
            if (f->enclave.state == NOT_CONNECTED) {
                mark_federate_requesting_stop(f);
                continue;
            }
            if (rti_remote->base.tracing_enabled) {
                tracepoint_rti_to_federate(rti_remote->base.trace, send_STOP_REQ, f->enclave.id, &rti_remote->base.max_stop_tag);
            }
            write_to_socket_fail_on_error(&f->socket, MSG_TYPE_STOP_REQUEST_LENGTH, stop_request_buffer, &rti_mutex,
                    "RTI failed to forward MSG_TYPE_STOP_REQUEST message to federate %d.", f->enclave.id);
        }
    }
    LF_PRINT_LOG("RTI forwarded to federates MSG_TYPE_STOP_REQUEST with tag (" PRINTF_TIME ", %u).",
                 rti_remote->base.max_stop_tag.time - start_time,
                 rti_remote->base.max_stop_tag.microstep);
    LF_MUTEX_UNLOCK(&rti_mutex);
}

void handle_stop_request_reply(federate_info_t *fed) {
    size_t bytes_to_read = MSG_TYPE_STOP_REQUEST_REPLY_LENGTH - 1;
    unsigned char buffer_stop_time[bytes_to_read];
    read_from_socket_fail_on_error(&fed->socket, bytes_to_read, buffer_stop_time, NULL,
            "RTI failed to read the reply to MSG_TYPE_STOP_REQUEST message from federate %d.",
            fed->enclave.id);

    tag_t federate_stop_tag = extract_tag(buffer_stop_time);

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_STOP_REQ_REP, fed->enclave.id, &federate_stop_tag);
    }

    LF_PRINT_LOG("RTI received from federate %d STOP reply tag " PRINTF_TAG ".", fed->enclave.id,
            federate_stop_tag.time - start_time,
            federate_stop_tag.microstep);

    // Acquire the mutex lock so that we can change the state of the RTI
    LF_MUTEX_LOCK(&rti_mutex);
    // If the federate has not requested stop before, count the reply
    if (lf_tag_compare(federate_stop_tag, rti_remote->base.max_stop_tag) > 0) {
        rti_remote->base.max_stop_tag = federate_stop_tag;
    }
    mark_federate_requesting_stop(fed);
    LF_MUTEX_UNLOCK(&rti_mutex);
}

//////////////////////////////////////////////////

void handle_address_query(uint16_t fed_id) {
    federate_info_t *fed = GET_FED_INFO(fed_id);
    // Use buffer both for reading and constructing the reply.
    // The length is what is needed for the reply.
    unsigned char buffer[1 + sizeof(int32_t)];
    read_from_socket_fail_on_error(&fed->socket, sizeof(uint16_t), (unsigned char *)buffer, NULL,
            "Failed to read address query.");
    uint16_t remote_fed_id = extract_uint16(buffer);

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_ADR_QR, fed_id, NULL);
    }

    LF_PRINT_DEBUG("RTI received address query from %d for %d.", fed_id, remote_fed_id);

    // NOTE: server_port initializes to -1, which means the RTI does not know
    // the port number because it has not yet received an MSG_TYPE_ADDRESS_ADVERTISEMENT message
    // from this federate. In that case, it will respond by sending -1.

    // Response message is also of type MSG_TYPE_ADDRESS_QUERY.
    buffer[0] = MSG_TYPE_ADDRESS_QUERY;

    // Encode the port number.
    federate_info_t *remote_fed = GET_FED_INFO(remote_fed_id);

    // Send the port number (which could be -1).
    LF_MUTEX_LOCK(&rti_mutex);
    encode_int32(remote_fed->server_port, (unsigned char *)&buffer[1]);
    write_to_socket_fail_on_error(
            &fed->socket, sizeof(int32_t) + 1, (unsigned char *)buffer, &rti_mutex,
            "Failed to write port number to socket of federate %d.", fed_id);

    // Send the server IP address to federate.
    write_to_socket_fail_on_error(
            &fed->socket, sizeof(remote_fed->server_ip_addr),
            (unsigned char *)&remote_fed->server_ip_addr, &rti_mutex,
            "Failed to write ip address to socket of federate %d.", fed_id);
    LF_MUTEX_UNLOCK(&rti_mutex);

    LF_PRINT_DEBUG("Replied to address query from federate %d with address %s:%d.",
            fed_id, remote_fed->server_hostname, remote_fed->server_port);
}

void handle_address_ad(uint16_t federate_id) {
    federate_info_t *fed = GET_FED_INFO(federate_id);
    // Read the port number of the federate that can be used for physical
    // connections to other federates
    int32_t server_port = -1;
    unsigned char buffer[sizeof(int32_t)];
    read_from_socket_fail_on_error(&fed->socket, sizeof(int32_t), (unsigned char *)buffer, NULL,
            "Error reading port data from federate %d.", federate_id);

    server_port = extract_int32(buffer);

    assert(server_port < 65536);

    LF_MUTEX_LOCK(&rti_mutex);
    fed->server_port = server_port;
    LF_MUTEX_UNLOCK(&rti_mutex);

    LF_PRINT_LOG("Received address advertisement with port %d from federate %d.", server_port, federate_id);
    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_ADR_AD, federate_id, NULL);
    }
}

void handle_timestamp(federate_info_t *my_fed) {
    unsigned char buffer[sizeof(int64_t)];
    // Read bytes from the socket. We need 8 bytes.
    read_from_socket_fail_on_error(&my_fed->socket, sizeof(int64_t), (unsigned char *)&buffer, NULL,
            "ERROR reading timestamp from federate %d.\n", my_fed->enclave.id);

    int64_t timestamp = swap_bytes_if_big_endian_int64(*((int64_t *)(&buffer)));
    if (rti_remote->base.tracing_enabled) {
        tag_t tag = {.time = timestamp, .microstep = 0};
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_TIMESTAMP, my_fed->enclave.id, &tag);
    }
    LF_PRINT_DEBUG("RTI received timestamp message with time: " PRINTF_TIME ".", timestamp);

    LF_MUTEX_LOCK(&rti_mutex);
    rti_remote->num_feds_proposed_start++;
    if (timestamp > rti_remote->max_start_time) {
        rti_remote->max_start_time = timestamp;
    }
    if (rti_remote->num_feds_proposed_start == rti_remote->base.number_of_scheduling_nodes) {
        // All federates have proposed a start time.
        lf_cond_broadcast(&received_start_times);
    } else {
        // Some federates have not yet proposed a start time.
        // wait for a notification.
        while (rti_remote->num_feds_proposed_start < rti_remote->base.number_of_scheduling_nodes) {
            // FIXME: Should have a timeout here?
            lf_cond_wait(&received_start_times);
        }
    }

    LF_MUTEX_UNLOCK(&rti_mutex);

    // Send back to the federate the maximum time plus an offset on a TIMESTAMP
    // message.
    unsigned char start_time_buffer[MSG_TYPE_TIMESTAMP_LENGTH];
    start_time_buffer[0] = MSG_TYPE_TIMESTAMP;
    // Add an offset to this start time to get everyone starting together.
    start_time = rti_remote->max_start_time + DELAY_START;
    encode_int64(swap_bytes_if_big_endian_int64(start_time), &start_time_buffer[1]);

    if (rti_remote->base.tracing_enabled) {
        tag_t tag = {.time = start_time, .microstep = 0};
        tracepoint_rti_to_federate(rti_remote->base.trace, send_TIMESTAMP, my_fed->enclave.id, &tag);
    }
    if (write_to_socket(my_fed->socket, MSG_TYPE_TIMESTAMP_LENGTH, start_time_buffer)) {
        lf_print_error("Failed to send the starting time to federate %d.", my_fed->enclave.id);
    }

    LF_MUTEX_LOCK(&rti_mutex);
    // Update state for the federate to indicate that the MSG_TYPE_TIMESTAMP
    // message has been sent. That MSG_TYPE_TIMESTAMP message grants time advance to
    // the federate to the start time.
    my_fed->enclave.state = GRANTED;
    lf_cond_broadcast(&sent_start_time);
    LF_PRINT_LOG("RTI sent start time " PRINTF_TIME " to federate %d.", start_time, my_fed->enclave.id);
    LF_MUTEX_UNLOCK(&rti_mutex);
}

void send_physical_clock(unsigned char message_type, federate_info_t *fed, socket_type_t socket_type) {
    if (fed->enclave.state == NOT_CONNECTED) {
        lf_print_warning("Clock sync: RTI failed to send physical time to federate %d. Socket not connected.\n",
                         fed->enclave.id);
        return;
    }
    unsigned char buffer[sizeof(int64_t) + 1];
    buffer[0] = message_type;
    int64_t current_physical_time = lf_time_physical();
    encode_int64(current_physical_time, &(buffer[1]));

    // Send the message
    if (socket_type == UDP) {
        // FIXME: UDP_addr is never initialized.
        LF_PRINT_DEBUG("Clock sync: RTI sending UDP message type %u.", buffer[0]);
        ssize_t bytes_written = sendto(rti_remote->socket_descriptor_UDP, buffer, 1 + sizeof(int64_t), 0,
                (struct sockaddr *)&fed->UDP_addr, sizeof(fed->UDP_addr));
        if (bytes_written < (ssize_t)sizeof(int64_t) + 1) {
            lf_print_warning("Clock sync: RTI failed to send physical time to federate %d: %s\n",
                    fed->enclave.id,
                    strerror(errno));
            return;
        }
    }
    else if (socket_type == TCP) {
        LF_PRINT_DEBUG("Clock sync:  RTI sending TCP message type %u.", buffer[0]);
        LF_MUTEX_LOCK(&rti_mutex);
        write_to_socket_fail_on_error(&fed->socket, 1 + sizeof(int64_t), buffer, &rti_mutex,
                "Clock sync: RTI failed to send physical time to federate %d.",
                fed->enclave.id);
        LF_MUTEX_UNLOCK(&rti_mutex);
    }
    LF_PRINT_DEBUG("Clock sync: RTI sent PHYSICAL_TIME_SYNC_MESSAGE with timestamp " PRINTF_TIME 
            " to federate %d.",
            current_physical_time,
            fed->enclave.id);
}

void handle_physical_clock_sync_message(federate_info_t *my_fed, socket_type_t socket_type) {
    // Lock the mutex to prevent interference between sending the two
    // coded probe messages.
    LF_MUTEX_LOCK(&rti_mutex);
    // Reply with a T4 type message
    send_physical_clock(MSG_TYPE_CLOCK_SYNC_T4, my_fed, socket_type);
    // Send the corresponding coded probe immediately after,
    // but only if this is a UDP channel.
    if (socket_type == UDP) {
        send_physical_clock(MSG_TYPE_CLOCK_SYNC_CODED_PROBE, my_fed, socket_type);
    }
    LF_MUTEX_UNLOCK(&rti_mutex);
}

void *clock_synchronization_thread(void *noargs) {

    // Wait until all federates have been notified of the start time.
    // FIXME: Use lf_ version of this when merged with master.
    LF_MUTEX_LOCK(&rti_mutex);
    while (rti_remote->num_feds_proposed_start < rti_remote->base.number_of_scheduling_nodes) {
        lf_cond_wait(&received_start_times);
    }
    LF_MUTEX_UNLOCK(&rti_mutex);

    // Wait until the start time before starting clock synchronization.
    // The above wait ensures that start_time has been set.
    interval_t ns_to_wait = start_time - lf_time_physical();

    if (ns_to_wait > 0LL) {
        lf_sleep(ns_to_wait);
    }

    // Initiate a clock synchronization every rti->clock_sync_period_ns
    // Initiate a clock synchronization every rti->clock_sync_period_ns
    struct timespec sleep_time = {(time_t)rti_remote->clock_sync_period_ns / BILLION,
            rti_remote->clock_sync_period_ns % BILLION};
    struct timespec remaining_time;

    bool any_federates_connected = true;
    while (any_federates_connected) {
        // Sleep
        lf_sleep(rti_remote->clock_sync_period_ns); // Can be interrupted
        any_federates_connected = false;
        for (int fed_id = 0; fed_id < rti_remote->base.number_of_scheduling_nodes; fed_id++) {
            federate_info_t *fed = GET_FED_INFO(fed_id);
            if (fed->enclave.state == NOT_CONNECTED) {
                // FIXME: We need better error handling here, but clock sync failure
                // should not stop execution.
                lf_print_error("Clock sync failed with federate %d. Not connected.", fed_id);
                continue;
            } else if (!fed->clock_synchronization_enabled) {
                continue;
            }
            // Send the RTI's current physical time to the federate
            // Send on UDP.
            LF_PRINT_DEBUG("RTI sending T1 message to initiate clock sync round.");
            send_physical_clock(MSG_TYPE_CLOCK_SYNC_T1, fed, UDP);

            // Listen for reply message, which should be T3.
            size_t message_size = 1 + sizeof(int32_t);
            unsigned char buffer[message_size];
            // Maximum number of messages that we discard before giving up on this cycle.
            // If the T3 message from this federate does not arrive and we keep receiving
            // other messages, then give up on this federate and move to the next federate.
            int remaining_attempts = 5;
            while (remaining_attempts > 0) {
                remaining_attempts--;
                int read_failed = read_from_socket(rti_remote->socket_descriptor_UDP, message_size, buffer);
                // If any errors occur, either discard the message or the clock sync round.
                if (!read_failed) {
                    if (buffer[0] == MSG_TYPE_CLOCK_SYNC_T3) {
                        int32_t fed_id_2 = extract_int32(&(buffer[1]));
                        // Check that this message came from the correct federate.
                        if (fed_id_2 != fed->enclave.id) {
                            // Message is from the wrong federate. Discard the message.
                            lf_print_warning("Clock sync: Received T3 message from federate %d, "
                                             "but expected one from %d. Discarding message.",
                                             fed_id_2, fed->enclave.id);
                            continue;
                        }
                        LF_PRINT_DEBUG("Clock sync: RTI received T3 message from federate %d.", fed_id_2);
                        handle_physical_clock_sync_message(GET_FED_INFO(fed_id_2), UDP);
                        break;
                    } else {
                        // The message is not a T3 message. Discard the message and
                        // continue waiting for the T3 message. This is possibly a message
                        // from a previous cycle that was discarded.
                        lf_print_warning(
                                "Clock sync: Unexpected UDP message %u. Expected %u from federate %d. "
                                "Discarding message.",
                                buffer[0],
                                MSG_TYPE_CLOCK_SYNC_T3,
                                fed->enclave.id);
                        continue;
                    }
                } else {
                    lf_print_warning("Clock sync: Read from UDP socket failed: %s. "
                            "Skipping clock sync round for federate %d.",
                            strerror(errno),
                            fed->enclave.id);
                    remaining_attempts = -1;
                }
            }
            if (remaining_attempts > 0) {
                any_federates_connected = true;
            }
        }
    }
    return NULL;
}

/**
 * Handle MSG_TYPE_FAILED sent by a federate. This message is sent by a federate
 * that is exiting in failure.  In this case, the RTI will
 * also terminate abnormally, returning a non-zero exit code when it exits.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param my_fed The federate sending a MSG_TYPE_FAILED message.
 */
static void handle_federate_failed(federate_info_t *my_fed) {
    // Nothing more to do. Close the socket and exit.
    LF_MUTEX_LOCK(&rti_mutex);

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_FAILED, my_fed->enclave.id, NULL);
    }

    // Set the flag telling the RTI to exit with an error code when it exits.
    _lf_federate_reports_error = true;
    lf_print_error("RTI: Federate %d reports an error and has exited.", my_fed->enclave.id);

    my_fed->enclave.state = NOT_CONNECTED;

    // Indicate that there will no further events from this federate.
    my_fed->enclave.next_event = FOREVER_TAG;

    // According to this: https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket,
    // the close should happen when receiving a 0 length message from the other end.
    // Here, we just signal the other side that no further writes to the socket are
    // forthcoming, which should result in the other end getting a zero-length reception.
    shutdown(my_fed->socket, SHUT_RDWR);

    // We can now safely close the socket.
    close(my_fed->socket); //  from unistd.h

    // Check downstream federates to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which upstream federates have been visited.
    bool *visited = (bool *)calloc(rti_remote->base.number_of_scheduling_nodes, sizeof(bool)); // Initializes to 0.
    notify_downstream_advance_grant_if_safe(&(my_fed->enclave), visited);
    free(visited);

    LF_MUTEX_UNLOCK(&rti_mutex);
}

/**
 * Handle MSG_TYPE_RESIGN sent by a federate. This message is sent at the time of termination
 * after all shutdown events are processed on the federate.
 * 
 * This function assumes the caller does not hold the mutex.
 *
 * @note At this point, the RTI might have outgoing messages to the federate. This
 * function thus first performs a shutdown on the socket, which sends an EOF. It then
 * waits for the remote socket to be closed before closing the socket itself.
 *
 * @param my_fed The federate sending a MSG_TYPE_RESIGN message.
 */
static void handle_federate_resign(federate_info_t *my_fed) {
    // Nothing more to do. Close the socket and exit.
    LF_MUTEX_LOCK(&rti_mutex);

    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_from_federate(rti_remote->base.trace, receive_RESIGN, my_fed->enclave.id, NULL);
    }

    lf_print("RTI: Federate %d has resigned.", my_fed->enclave.id);

    my_fed->enclave.state = NOT_CONNECTED;

    // Indicate that there will no further events from this federate.
    my_fed->enclave.next_event = FOREVER_TAG;

    // According to this: https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket,
    // the close should happen when receiving a 0 length message from the other end.
    // Here, we just signal the other side that no further writes to the socket are
    // forthcoming, which should result in the other end getting a zero-length reception.
    shutdown(my_fed->socket, SHUT_WR);

    // Wait for the federate to send an EOF or a socket error to occur.
    // Discard any incoming bytes. Normally, this read should return 0 because
    // the federate is resigning and should itself invoke shutdown.
    unsigned char buffer[10];
    while (read(my_fed->socket, buffer, 10) > 0);

    // We can now safely close the socket.
    close(my_fed->socket); //  from unistd.h

    // Check downstream federates to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which upstream federates have been visited.
    bool *visited = (bool *)calloc(rti_remote->base.number_of_scheduling_nodes, sizeof(bool)); // Initializes to 0.
    notify_downstream_advance_grant_if_safe(&(my_fed->enclave), visited);
    free(visited);

    LF_MUTEX_UNLOCK(&rti_mutex);
}

void *federate_info_thread_TCP(void *fed) {
    federate_info_t *my_fed = (federate_info_t *)fed;

    // Buffer for incoming messages.
    // This does not constrain the message size because messages
    // are forwarded piece by piece.
    unsigned char buffer[FED_COM_BUFFER_SIZE];

    // Listen for messages from the federate.
    while (my_fed->enclave.state != NOT_CONNECTED) {
        // Read no more than one byte to get the message type.
        int read_failed = read_from_socket(my_fed->socket, 1, buffer);
        if (read_failed) {
            // Socket is closed
            lf_print_warning("RTI: Socket to federate %d is closed. Exiting the thread.", my_fed->enclave.id);
            my_fed->enclave.state = NOT_CONNECTED;
            my_fed->socket = -1;
            // FIXME: We need better error handling here, but do not stop execution here.
            break;
        }
        LF_PRINT_DEBUG("RTI: Received message type %u from federate %d.", buffer[0], my_fed->enclave.id);
        switch (buffer[0]) {
        case MSG_TYPE_TIMESTAMP:
            handle_timestamp(my_fed);
            break;
        case MSG_TYPE_ADDRESS_QUERY:
            handle_address_query(my_fed->enclave.id);
            break;
        case MSG_TYPE_ADDRESS_ADVERTISEMENT:
            handle_address_ad(my_fed->enclave.id);
            break;
        case MSG_TYPE_TAGGED_MESSAGE:
            handle_timed_message(my_fed, buffer);
            break;
        case MSG_TYPE_RESIGN:
            handle_federate_resign(my_fed);
            return NULL;
        case MSG_TYPE_NEXT_EVENT_TAG:
            handle_next_event_tag(my_fed);
            break;
        case MSG_TYPE_LATEST_TAG_COMPLETE:
            handle_latest_tag_complete(my_fed);
            break;
        case MSG_TYPE_STOP_REQUEST:
            handle_stop_request_message(my_fed); // FIXME: Reviewed until here.
                                                 // Need to also look at
                                                 // notify_advance_grant_if_safe()
                                                 // and notify_downstream_advance_grant_if_safe()
            break;
        case MSG_TYPE_STOP_REQUEST_REPLY:
            handle_stop_request_reply(my_fed);
            break;
        case MSG_TYPE_PORT_ABSENT:
            handle_port_absent_message(my_fed, buffer);
            break;
        case MSG_TYPE_FAILED:
            handle_federate_failed(my_fed);
            return NULL;
        default:
            lf_print_error("RTI received from federate %d an unrecognized TCP message type: %u.", my_fed->enclave.id, buffer[0]);
            if (rti_remote->base.tracing_enabled) {
                tracepoint_rti_from_federate(rti_remote->base.trace, receive_UNIDENTIFIED, my_fed->enclave.id, NULL);
            }
        }
    }

    // Nothing more to do. Close the socket and exit.
    // Prevent multiple threads from closing the same socket at the same time.
    LF_MUTEX_LOCK(&rti_mutex);
    close(my_fed->socket); //  from unistd.h
    LF_MUTEX_UNLOCK(&rti_mutex);
    return NULL;
}

void send_reject(int *socket_id, unsigned char error_code) {
    LF_PRINT_DEBUG("RTI sending MSG_TYPE_REJECT.");
    unsigned char response[2];
    response[0] = MSG_TYPE_REJECT;
    response[1] = error_code;
    LF_MUTEX_LOCK(&rti_mutex);
    // NOTE: Ignore errors on this response.
    if (write_to_socket(*socket_id, 2, response)) {
        lf_print_warning("RTI failed to write MSG_TYPE_REJECT message on the socket.");
    }
    // Close the socket.
    shutdown(*socket_id, SHUT_RDWR);
    close(*socket_id);
    *socket_id = -1;
    LF_MUTEX_UNLOCK(&rti_mutex);
}

/**
 * Listen for a MSG_TYPE_FED_IDS message, which includes as a payload
 * a federate ID and a federation ID. If the federation ID
 * matches this federation, send an MSG_TYPE_ACK and otherwise send
 * a MSG_TYPE_REJECT message.
 * @param socket_id Pointer to the socket on which to listen.
 * @param client_fd The socket address.
 * @return The federate ID for success or -1 for failure.
 */
static int32_t receive_and_check_fed_id_message(int *socket_id, struct sockaddr_in *client_fd) {
    // Buffer for message ID, federate ID, and federation ID length.
    size_t length = 1 + sizeof(uint16_t) + 1; // Message ID, federate ID, length of fedration ID.
    unsigned char buffer[length];

    // Read bytes from the socket. We need 4 bytes.
    if (read_from_socket_close_on_error(socket_id, length, buffer)) {
        lf_print_error("RTI failed to read from accepted socket.");
        return -1;
    }

    uint16_t fed_id = rti_remote->base.number_of_scheduling_nodes; // Initialize to an invalid value.

    // First byte received is the message type.
    if (buffer[0] != MSG_TYPE_FED_IDS) {
        if (rti_remote->base.tracing_enabled) {
            tracepoint_rti_to_federate(rti_remote->base.trace, send_REJECT, fed_id, NULL);
        }
        if (buffer[0] == MSG_TYPE_P2P_SENDING_FED_ID || buffer[0] == MSG_TYPE_P2P_TAGGED_MESSAGE) {
            // The federate is trying to connect to a peer, not to the RTI.
            // It has connected to the RTI instead.
            // FIXME: This should not happen, but apparently has been observed.
            // It should not happen because the peers get the port and IP address
            // of the peer they want to connect to from the RTI.
            // If the connection is a peer-to-peer connection between two
            // federates, reject the connection with the WRONG_SERVER error.
            send_reject(socket_id, WRONG_SERVER);
        } else {
            send_reject(socket_id, UNEXPECTED_MESSAGE);
        }
        lf_print_error("RTI expected a MSG_TYPE_FED_IDS message. Got %u (see net_common.h).", buffer[0]);
        return -1;
    } else {
        // Received federate ID.
        fed_id = extract_uint16(buffer + 1);
        LF_PRINT_DEBUG("RTI received federate ID: %d.", fed_id);

        // Read the federation ID.  First read the length, which is one byte.
        size_t federation_id_length = (size_t)buffer[sizeof(uint16_t) + 1];
        char federation_id_received[federation_id_length + 1]; // One extra for null terminator.
        // Next read the actual federation ID.
        if (read_from_socket_close_on_error(socket_id, federation_id_length,
                (unsigned char *)federation_id_received)) {
            lf_print_error("RTI failed to read federation id from federate %d.", fed_id);
            return -1;
        }

        // Terminate the string with a null.
        federation_id_received[federation_id_length] = 0;

        LF_PRINT_DEBUG("RTI received federation ID: %s.", federation_id_received);

        if (rti_remote->base.tracing_enabled) {
            tracepoint_rti_from_federate(rti_remote->base.trace, receive_FED_ID, fed_id, NULL);
        }
        // Compare the received federation ID to mine.
        if (strncmp(rti_remote->federation_id, federation_id_received, federation_id_length) != 0) {
            // Federation IDs do not match. Send back a MSG_TYPE_REJECT message.
            lf_print_warning("Federate from another federation %s attempted to connect to RTI in federation %s.",
                    federation_id_received,
                    rti_remote->federation_id);
            if (rti_remote->base.tracing_enabled) {
                tracepoint_rti_to_federate(rti_remote->base.trace, send_REJECT, fed_id, NULL);
            }
            send_reject(socket_id, FEDERATION_ID_DOES_NOT_MATCH);
            return -1;
        } else {
            if (fed_id >= rti_remote->base.number_of_scheduling_nodes) {
                // Federate ID is out of range.
                lf_print_error("RTI received federate ID %d, which is out of range.", fed_id);
                if (rti_remote->base.tracing_enabled) {
                    tracepoint_rti_to_federate(rti_remote->base.trace, send_REJECT, fed_id, NULL);
                }
                send_reject(socket_id, FEDERATE_ID_OUT_OF_RANGE);
                return -1;
            } else {
                if ((rti_remote->base.scheduling_nodes[fed_id])->state != NOT_CONNECTED) {
                    lf_print_error("RTI received duplicate federate ID: %d.", fed_id);
                    if (rti_remote->base.tracing_enabled) {
                        tracepoint_rti_to_federate(rti_remote->base.trace, send_REJECT, fed_id, NULL);
                    }
                    send_reject(socket_id, FEDERATE_ID_IN_USE);
                    return -1;
                }
            }
        }
    }
    federate_info_t *fed = GET_FED_INFO(fed_id);
    // The MSG_TYPE_FED_IDS message has the right federation ID.
    // Assign the address information for federate.
    // The IP address is stored here as an in_addr struct (in .server_ip_addr) that can be useful
    // to create sockets and can be efficiently sent over the network.
    // First, convert the sockaddr structure into a sockaddr_in that contains an internet address.
    struct sockaddr_in *pV4_addr = client_fd;
    // Then extract the internet address (which is in IPv4 format) and assign it as the federate's socket server
    fed->server_ip_addr = pV4_addr->sin_addr;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    // Create the human readable format and copy that into
    // the .server_hostname field of the federate.
    char str[INET_ADDRSTRLEN + 1];
    inet_ntop(AF_INET, &fed->server_ip_addr, str, INET_ADDRSTRLEN);
    strncpy(fed->server_hostname, str, INET_ADDRSTRLEN);

    LF_PRINT_DEBUG("RTI got address %s from federate %d.", fed->server_hostname, fed_id);
#endif
    fed->socket = *socket_id;

    // Set the federate's state as pending
    // because it is waiting for the start time to be
    // sent by the RTI before beginning its execution.
    fed->enclave.state = PENDING;

    LF_PRINT_DEBUG("RTI responding with MSG_TYPE_ACK to federate %d.", fed_id);
    // Send an MSG_TYPE_ACK message.
    unsigned char ack_message = MSG_TYPE_ACK;
    if (rti_remote->base.tracing_enabled) {
        tracepoint_rti_to_federate(rti_remote->base.trace, send_ACK, fed_id, NULL);
    }
    LF_MUTEX_LOCK(&rti_mutex);
    if (write_to_socket_close_on_error(&fed->socket, 1, &ack_message)) {
        LF_MUTEX_UNLOCK(&rti_mutex);
        lf_print_error("RTI failed to write MSG_TYPE_ACK message to federate %d.", fed_id);
        return -1;
    }
    LF_MUTEX_UNLOCK(&rti_mutex);

    LF_PRINT_DEBUG("RTI sent MSG_TYPE_ACK to federate %d.", fed_id);

    return (int32_t)fed_id;
}

/**
 * Listen for a MSG_TYPE_NEIGHBOR_STRUCTURE message, and upon receiving it, fill
 * out the relevant information in the federate's struct.
 * @return 1 on success and 0 on failure.
 */
static int receive_connection_information(int *socket_id, uint16_t fed_id) {
    LF_PRINT_DEBUG("RTI waiting for MSG_TYPE_NEIGHBOR_STRUCTURE from federate %d.", fed_id);
    unsigned char connection_info_header[MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE];
    read_from_socket_fail_on_error(
            socket_id,
            MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE,
            connection_info_header,
            NULL,
            "RTI failed to read MSG_TYPE_NEIGHBOR_STRUCTURE message header from federate %d.",
            fed_id);

    if (connection_info_header[0] != MSG_TYPE_NEIGHBOR_STRUCTURE) {
        lf_print_error(
                "RTI was expecting a MSG_TYPE_UDP_PORT message from federate %d. Got %u instead. "
                "Rejecting federate.",
                fed_id, connection_info_header[0]);
        send_reject(socket_id, UNEXPECTED_MESSAGE);
        return 0;
    } else {
        federate_info_t *fed = GET_FED_INFO(fed_id);
        // Read the number of upstream and downstream connections
        fed->enclave.num_upstream = extract_int32(&(connection_info_header[1]));
        fed->enclave.num_downstream = extract_int32(&(connection_info_header[1 + sizeof(int32_t)]));
        LF_PRINT_DEBUG(
                "RTI got %d upstreams and %d downstreams from federate %d.",
                fed->enclave.num_upstream,
                fed->enclave.num_downstream,
                fed_id);

        // Allocate memory for the upstream and downstream pointers
        if (fed->enclave.num_upstream > 0) {
            fed->enclave.upstream = (int *)malloc(sizeof(uint16_t) * fed->enclave.num_upstream);
            // Allocate memory for the upstream delay pointers
            fed->enclave.upstream_delay = (interval_t *)malloc(
                    sizeof(interval_t) * fed->enclave.num_upstream);
        } else {
            fed->enclave.upstream = (int *)NULL;
            fed->enclave.upstream_delay = (interval_t *)NULL;
        }
        if (fed->enclave.num_downstream > 0) {
            fed->enclave.downstream = (int *)malloc(sizeof(uint16_t) * fed->enclave.num_downstream);
        } else {
            fed->enclave.downstream = (int *)NULL;
        }

        size_t connections_info_body_size = (
                (sizeof(uint16_t) + sizeof(int64_t)) * fed->enclave.num_upstream)
                + (sizeof(uint16_t) * fed->enclave.num_downstream);
        unsigned char *connections_info_body = NULL;
        if (connections_info_body_size > 0) {
            connections_info_body = (unsigned char *)malloc(connections_info_body_size);
            read_from_socket_fail_on_error(
                    socket_id,
                    connections_info_body_size,
                    connections_info_body,
                    NULL,
                    "RTI failed to read MSG_TYPE_NEIGHBOR_STRUCTURE message body from federate %d.",
                    fed_id);
            // Keep track of where we are in the buffer
            size_t message_head = 0;
            // First, read the info about upstream federates
            for (int i = 0; i < fed->enclave.num_upstream; i++) {
                fed->enclave.upstream[i] = extract_uint16(&(connections_info_body[message_head]));
                message_head += sizeof(uint16_t);
                fed->enclave.upstream_delay[i] = extract_int64(&(connections_info_body[message_head]));
                message_head += sizeof(int64_t);
            }

            // Next, read the info about downstream federates
            for (int i = 0; i < fed->enclave.num_downstream; i++) {
                fed->enclave.downstream[i] = extract_uint16(&(connections_info_body[message_head]));
                message_head += sizeof(uint16_t);
            }

            free(connections_info_body);
        }
    }
    LF_PRINT_DEBUG("RTI received neighbor structure from federate %d.", fed_id);
    return 1;
}

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
static int receive_udp_message_and_set_up_clock_sync(int *socket_id, uint16_t fed_id) {
    // Read the MSG_TYPE_UDP_PORT message from the federate regardless of the status of
    // clock synchronization. This message will tell the RTI whether the federate
    // is doing clock synchronization, and if it is, what port to use for UDP.
    LF_PRINT_DEBUG("RTI waiting for MSG_TYPE_UDP_PORT from federate %d.", fed_id);
    unsigned char response[1 + sizeof(uint16_t)];
    read_from_socket_fail_on_error(socket_id, 1 + sizeof(uint16_t), response, NULL,
            "RTI failed to read MSG_TYPE_UDP_PORT message from federate %d.", fed_id);
    if (response[0] != MSG_TYPE_UDP_PORT) {
        lf_print_error(
                "RTI was expecting a MSG_TYPE_UDP_PORT message from federate %d. Got %u instead. "
                "Rejecting federate.",
                fed_id, response[0]);
        send_reject(socket_id, UNEXPECTED_MESSAGE);
        return 0;
    } else {
        federate_info_t *fed = GET_FED_INFO(fed_id);
        if (rti_remote->clock_sync_global_status >= clock_sync_init) {
            // If no initial clock sync, no need perform initial clock sync.
            uint16_t federate_UDP_port_number = extract_uint16(&(response[1]));

            LF_PRINT_DEBUG("RTI got MSG_TYPE_UDP_PORT %u from federate %d.", federate_UDP_port_number, fed_id);

            // A port number of UINT16_MAX means initial clock sync should not be performed.
            if (federate_UDP_port_number != UINT16_MAX) {
                // Perform the initialization clock synchronization with the federate.
                // Send the required number of messages for clock synchronization
                for (int i = 0; i < rti_remote->clock_sync_exchanges_per_interval; i++) {
                    // Send the RTI's current physical time T1 to the federate.
                    send_physical_clock(MSG_TYPE_CLOCK_SYNC_T1, fed, TCP);

                    // Listen for reply message, which should be T3.
                    size_t message_size = 1 + sizeof(int32_t);
                    unsigned char buffer[message_size];
                    read_from_socket_fail_on_error(socket_id, message_size, buffer, NULL,
                            "Socket to federate %d unexpectedly closed.", fed_id);
                    if (buffer[0] == MSG_TYPE_CLOCK_SYNC_T3) {
                        int32_t fed_id = extract_int32(&(buffer[1]));
                        assert(fed_id > -1);
                        assert(fed_id < 65536);
                        LF_PRINT_DEBUG("RTI received T3 clock sync message from federate %d.", fed_id);
                        handle_physical_clock_sync_message(fed, TCP);
                    } else {
                        lf_print_error("Unexpected message %u from federate %d.", buffer[0], fed_id);
                        send_reject(socket_id, UNEXPECTED_MESSAGE);
                        return 0;
                    }
                }
                LF_PRINT_DEBUG("RTI finished initial clock synchronization with federate %d.", fed_id);
            }
            if (rti_remote->clock_sync_global_status >= clock_sync_on) {
                // If no runtime clock sync, no need to set up the UDP port.
                if (federate_UDP_port_number > 0) {
                    // Initialize the UDP_addr field of the federate struct
                    fed->UDP_addr.sin_family = AF_INET;
                    fed->UDP_addr.sin_port = htons(federate_UDP_port_number);
                    fed->UDP_addr.sin_addr = fed->server_ip_addr;
                }
            } else {
                // Disable clock sync after initial round.
                fed->clock_synchronization_enabled = false;
            }
        } else {
            // No clock synchronization at all.
            LF_PRINT_DEBUG("RTI: No clock synchronization for federate %d.", fed_id);
            // Clock synchronization is universally disabled via the clock-sync command-line parameter
            // (-c off was passed to the RTI).
            // Note that the federates are still going to send a
            // MSG_TYPE_UDP_PORT message but with a payload (port) of -1.
            fed->clock_synchronization_enabled = false;
        }
    }
    return 1;
}

#ifdef __RTI_AUTH__
/**
 * Authenticate incoming federate by performing HMAC-based authentication.
 * 
 * @param socket Socket for the incoming federate tryting to authenticate.
 * @return True if authentication is successful and false otherwise.
 */
static bool authenticate_federate(int *socket) {
    // Wait for MSG_TYPE_FED_NONCE from federate.
    size_t fed_id_length = sizeof(uint16_t);
    unsigned char buffer[1 + fed_id_length + NONCE_LENGTH];
    read_from_socket_fail_on_error(socket, 1 + fed_id_length + NONCE_LENGTH, buffer, NULL,
            "Failed to read MSG_TYPE_FED_NONCE");
    if (buffer[0] != MSG_TYPE_FED_NONCE) {
        lf_print_error_and_exit(
                "Received unexpected response %u from the FED (see net_common.h).",
                buffer[0]);
    }
    unsigned int hmac_length = SHA256_HMAC_LENGTH;
    size_t federation_id_length = strnlen(rti_remote->federation_id, 255);
    // HMAC tag is created with MSG_TYPE, federate ID, received federate nonce.
    unsigned char mac_buf[1 + fed_id_length + NONCE_LENGTH];
    mac_buf[0] = MSG_TYPE_RTI_RESPONSE;
    memcpy(&mac_buf[1], &buffer[1], fed_id_length);
    memcpy(&mac_buf[1 + fed_id_length], &buffer[1 + fed_id_length], NONCE_LENGTH);
    unsigned char hmac_tag[hmac_length];
    unsigned char *ret = HMAC(EVP_sha256(), rti_remote->federation_id,
                              federation_id_length, mac_buf, 1 + fed_id_length + NONCE_LENGTH,
                              hmac_tag, &hmac_length);
    if (ret == NULL) {
        lf_print_error_and_exit("HMAC construction failed for MSG_TYPE_RTI_RESPONSE.");
    }
    // Make buffer for message type, RTI's nonce, and HMAC tag.
    unsigned char sender[1 + NONCE_LENGTH + hmac_length];
    sender[0] = MSG_TYPE_RTI_RESPONSE;
    unsigned char rti_nonce[NONCE_LENGTH];
    RAND_bytes(rti_nonce, NONCE_LENGTH);
    memcpy(&sender[1], rti_nonce, NONCE_LENGTH);
    memcpy(&sender[1 + NONCE_LENGTH], hmac_tag, hmac_length);
    if (write_to_socket(*socket, 1 + NONCE_LENGTH + hmac_length, sender)) {
        lf_print_error("Failed to send nonce to federate.");
    }

    // Wait for MSG_TYPE_FED_RESPONSE
    unsigned char received[1 + hmac_length];
    read_from_socket_fail_on_error(socket, 1 + hmac_length, received, NULL,
            "Failed to read federate response.");
    if (received[0] != MSG_TYPE_FED_RESPONSE) {
        lf_print_error_and_exit(
            "Received unexpected response %u from the federate (see net_common.h).",
            received[0]);
        return false;
    }
    // HMAC tag is created with MSG_TYPE_FED_RESPONSE and RTI's nonce.
    unsigned char mac_buf2[1 + NONCE_LENGTH];
    mac_buf2[0] = MSG_TYPE_FED_RESPONSE;
    memcpy(&mac_buf2[1], rti_nonce, NONCE_LENGTH);
    unsigned char rti_tag[hmac_length];
    ret = HMAC(EVP_sha256(), rti_remote->federation_id, federation_id_length,
               mac_buf2, 1 + NONCE_LENGTH, rti_tag, &hmac_length);
    if (ret == NULL) {
        lf_print_error_and_exit("HMAC construction failed for MSG_TYPE_FED_RESPONSE.");
    }
    // Compare received tag and created tag.
    if (memcmp(&received[1], rti_tag, hmac_length) != 0) {
        // Federation IDs do not match. Send back a HMAC_DOES_NOT_MATCH message.
        lf_print_warning("HMAC authentication failed. Rejecting the federate.");
        send_reject(socket, HMAC_DOES_NOT_MATCH);
        return false;
    } else {
        LF_PRINT_LOG("Federate's HMAC verified.");
        return true;
    }
}
#endif

void lf_connect_to_federates(int socket_descriptor) {
    for (int i = 0; i < rti_remote->base.number_of_scheduling_nodes; i++) {
        // Wait for an incoming connection request.
        struct sockaddr client_fd;
        uint32_t client_length = sizeof(client_fd);
        // The following blocks until a federate connects.
        int socket_id = -1;
        while (1) {
            socket_id = accept(rti_remote->socket_descriptor_TCP, &client_fd, &client_length);
            if (socket_id >= 0) {
                // Got a socket
                break;
            } else if (socket_id < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
                lf_print_error_system_failure("RTI failed to accept the socket.");
            } else {
                // Try again
                lf_print_warning("RTI failed to accept the socket. %s. Trying again.", strerror(errno));
                continue;
            }
        }

// Wait for the first message from the federate when RTI -a option is on.
#ifdef __RTI_AUTH__
        if (rti_remote->authentication_enabled) {
            if (!authenticate_federate(&socket_id)) {
                lf_print_warning("RTI failed to authenticate the incoming federate.");
                // Close the socket.
                shutdown(socket_id, SHUT_RDWR);
                close(socket_id);
                socket_id = -1;
                // Ignore the federate that failed authentication.
                i--;
                continue;
            }
        }
#endif

        // The first message from the federate should contain its ID and the federation ID.
        int32_t fed_id = receive_and_check_fed_id_message(&socket_id, (struct sockaddr_in *)&client_fd);
        if (fed_id >= 0 && socket_id >= 0
                && receive_connection_information(&socket_id, (uint16_t)fed_id)
                && receive_udp_message_and_set_up_clock_sync(&socket_id, (uint16_t)fed_id)) {

            // Create a thread to communicate with the federate.
            // This has to be done after clock synchronization is finished
            // or that thread may end up attempting to handle incoming clock
            // synchronization messages.
            federate_info_t *fed = GET_FED_INFO(fed_id);
            lf_thread_create(&(fed->thread_id), federate_info_thread_TCP, fed);
        } else {
            // Received message was rejected. Try again.
            i--;
        }
    }
    // All federates have connected.
    LF_PRINT_DEBUG("All federates have connected to RTI.");

    if (rti_remote->clock_sync_global_status >= clock_sync_on) {
        // Create the thread that performs periodic PTP clock synchronization sessions
        // over the UDP channel, but only if the UDP channel is open and at least one
        // federate is performing runtime clock synchronization.
        bool clock_sync_enabled = false;
        for (int i = 0; i < rti_remote->base.number_of_scheduling_nodes; i++) {
            federate_info_t *fed_info = GET_FED_INFO(i);
            if (fed_info->clock_synchronization_enabled) {
                clock_sync_enabled = true;
                break;
            }
        }
        if (rti_remote->final_port_UDP != UINT16_MAX && clock_sync_enabled) {
            lf_thread_create(&rti_remote->clock_thread, clock_synchronization_thread, NULL);
        }
    }
}

void *respond_to_erroneous_connections(void *nothing) {
    while (true) {
        // Wait for an incoming connection request.
        struct sockaddr client_fd;
        uint32_t client_length = sizeof(client_fd);
        // The following will block until either a federate attempts to connect
        // or close(rti->socket_descriptor_TCP) is called.
        int socket_id = accept(rti_remote->socket_descriptor_TCP, &client_fd, &client_length);
        if (socket_id < 0) return NULL;

        if (rti_remote->all_federates_exited) {
            return NULL;
        }

        lf_print_error("RTI received an unexpected connection request. Federation is running.");
        unsigned char response[2];
        response[0] = MSG_TYPE_REJECT;
        response[1] = FEDERATION_ID_DOES_NOT_MATCH;
        // Ignore errors on this response.
        if (write_to_socket(socket_id, 2, response)) {
            lf_print_warning("RTI failed to write FEDERATION_ID_DOES_NOT_MATCH to erroneous incoming connection.");
        }
        // Close the socket.
        shutdown(socket_id, SHUT_RDWR);
        close(socket_id);
    }
    return NULL;
}

void initialize_federate(federate_info_t *fed, uint16_t id) {
    initialize_scheduling_node(&(fed->enclave), id);
    fed->requested_stop = false;
    fed->socket = -1; // No socket.
    fed->clock_synchronization_enabled = true;
    fed->in_transit_message_tags = pqueue_tag_init(10);
    strncpy(fed->server_hostname, "localhost", INET_ADDRSTRLEN);
    fed->server_ip_addr.s_addr = 0;
    fed->server_port = -1;
}

int32_t start_rti_server(uint16_t port) {
    _lf_initialize_clock();
    // Create the TCP socket server
    rti_remote->socket_descriptor_TCP = create_rti_server(port, TCP);
    lf_print("RTI: Listening for federates.");
    // Create the UDP socket server
    // Try to get the rti_remote->final_port_TCP + 1 port
    if (rti_remote->clock_sync_global_status >= clock_sync_on) {
        rti_remote->socket_descriptor_UDP = create_rti_server(rti_remote->final_port_TCP + 1, UDP);
    }
    return rti_remote->socket_descriptor_TCP;
}

void wait_for_federates(int socket_descriptor) {
    // Wait for connections from federates and create a thread for each.
    lf_connect_to_federates(socket_descriptor);

    // All federates have connected.
    lf_print("RTI: All expected federates have connected. Starting execution.");

    // The socket server will not continue to accept connections after all the federates
    // have joined.
    // In case some other federation's federates are trying to join the wrong
    // federation, need to respond. Start a separate thread to do that.
    lf_thread_t responder_thread;
    lf_thread_create(&responder_thread, respond_to_erroneous_connections, NULL);

    // Wait for federate threads to exit.
    void *thread_exit_status;
    for (int i = 0; i < rti_remote->base.number_of_scheduling_nodes; i++) {
        federate_info_t *fed = GET_FED_INFO(i);
        lf_print("RTI: Waiting for thread handling federate %d.", fed->enclave.id);
        lf_thread_join(fed->thread_id, &thread_exit_status);
        pqueue_tag_free(fed->in_transit_message_tags);
        lf_print("RTI: Federate %d thread exited.", fed->enclave.id);
    }

    rti_remote->all_federates_exited = true;

    // Shutdown and close the socket that is listening for incoming connections
    // so that the accept() call in respond_to_erroneous_connections returns.
    // That thread should then check rti->all_federates_exited and it should exit.
    if (shutdown(socket_descriptor, SHUT_RDWR)) {
        LF_PRINT_LOG("On shut down TCP socket, received reply: %s", strerror(errno));
    }
    // NOTE: In all common TCP/IP stacks, there is a time period,
    // typically between 30 and 120 seconds, called the TIME_WAIT period,
    // before the port is released after this close. This is because
    // the OS is preventing another program from accidentally receiving
    // duplicated packets intended for this program.
    close(socket_descriptor);

    if (rti_remote->socket_descriptor_UDP > 0) {
        if (shutdown(rti_remote->socket_descriptor_UDP, SHUT_RDWR)) {
            LF_PRINT_LOG("On shut down UDP socket, received reply: %s", strerror(errno));
        }
        close(rti_remote->socket_descriptor_UDP);
    }
}

void initialize_RTI(rti_remote_t *rti) {
    rti_remote = rti;

    // Initialize thread synchronization primitives
    LF_MUTEX_INIT(&rti_mutex);
    LF_COND_INIT(&received_start_times, &rti_mutex);
    LF_COND_INIT(&sent_start_time, &rti_mutex);

    initialize_rti_common(&rti_remote->base);
    rti_remote->base.mutex = &rti_mutex;

    // federation_rti related initializations
    rti_remote->max_start_time = 0LL;
    rti_remote->num_feds_proposed_start = 0;
    rti_remote->all_federates_exited = false;
    rti_remote->federation_id = "Unidentified Federation";
    rti_remote->user_specified_port = 0;
    rti_remote->final_port_TCP = 0;
    rti_remote->socket_descriptor_TCP = -1;
    rti_remote->final_port_UDP = UINT16_MAX;
    rti_remote->socket_descriptor_UDP = -1;
    rti_remote->clock_sync_global_status = clock_sync_init;
    rti_remote->clock_sync_period_ns = MSEC(10);
    rti_remote->clock_sync_exchanges_per_interval = 10;
    rti_remote->authentication_enabled = false;
    rti_remote->base.tracing_enabled = false;
    rti_remote->stop_in_progress = false;
}

void free_scheduling_nodes(scheduling_node_t **scheduling_nodes, uint16_t number_of_scheduling_nodes) {
    for (uint16_t i = 0; i < number_of_scheduling_nodes; i++) {
        // FIXME: Gives error freeing memory not allocated!!!!
        scheduling_node_t *node = scheduling_nodes[i];
        if (node->upstream != NULL)
            free(node->upstream);
        if (node->downstream != NULL)
            free(node->downstream);
    }
    free(scheduling_nodes);
}

#endif // STANDALONE_RTI
