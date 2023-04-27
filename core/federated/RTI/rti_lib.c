/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
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

#include "rti_lib.h"
#include <string.h>

// Global variables defined in tag.c:
extern instant_t start_time;

/**
 * The state of this RTI instance.
 */
RTI_instance_t _RTI = {
    .rti_mutex = PTHREAD_MUTEX_INITIALIZER,
    .received_start_times = PTHREAD_COND_INITIALIZER,
    .sent_start_time = PTHREAD_COND_INITIALIZER,
    .max_stop_tag = NEVER_TAG,
    .max_start_time = 0LL,
    .number_of_federates = 0,
    .num_feds_proposed_start = 0,
    .num_feds_handling_stop = 0,
    .all_federates_exited = false,
    .federation_id = "Unidentified Federation",
    .user_specified_port = 0,
    .final_port_TCP = 0,
    .socket_descriptor_TCP = -1,
    .final_port_UDP = UINT16_MAX,
    .socket_descriptor_UDP = -1,
    .clock_sync_global_status = clock_sync_init,
    .clock_sync_period_ns = MSEC(10),
    .clock_sync_exchanges_per_interval = 10,
    .authentication_enabled = false,
    .tracing_enabled = false
};

/**
 * Enter a critical section where logical time and the event queue are guaranteed
 * to not change unless they are changed within the critical section.
 * this can be implemented by disabling interrupts.
 * Users of this function must ensure that lf_init_critical_sections() is
 * called first and that lf_critical_section_exit() is called later.
 * @return 0 on success, platform-specific error number otherwise.
 */
extern int lf_critical_section_enter() {
    return pthread_mutex_lock(&_RTI.rti_mutex);
}

/**
 * Exit the critical section entered with lf_lock_time().
 * @return 0 on success, platform-specific error number otherwise.
 */
extern int lf_critical_section_exit() {
    return pthread_mutex_unlock(&_RTI.rti_mutex);
}

int create_server(int32_t specified_port, uint16_t port, socket_type_t socket_type) {
    // Timeout time for the communications of the server
    struct timeval timeout_time = {.tv_sec = TCP_TIMEOUT_TIME / BILLION, .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000};
    // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
    int socket_descriptor = -1;
    if (socket_type == TCP) {
        socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    } else if (socket_type == UDP) {
        socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
        // Set the appropriate timeout time
        timeout_time = (struct timeval){.tv_sec = UDP_TIMEOUT_TIME / BILLION, .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000};
    }
    if (socket_descriptor < 0) {
        lf_print_error_and_exit("Failed to create RTI socket.");
    }

    // Set the option for this socket to reuse the same address
    int true_variable = 1; // setsockopt() requires a reference to the value assigned to an option
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &true_variable, sizeof(int32_t)) < 0) {
        lf_print_error("RTI failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
    }
    // Set the timeout on the socket so that read and write operations don't block for too long
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
        lf_print_error("RTI failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
    }
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
        lf_print_error("RTI failed to set SO_SNDTIMEO option on the socket: %s.", strerror(errno));
    }

    /*
     * The following used to permit reuse of a port that an RTI has previously
     * used that has not been released. We no longer do this, but instead
     * increment the port number until an available port is found.

    // SO_REUSEPORT (since Linux 3.9)
    //       Permits multiple AF_INET or AF_INET6 sockets to be bound to an
    //       identical socket address.  This option must be set on each
    //       socket (including the first socket) prior to calling bind(2)
    //       on the socket.  To prevent port hijacking, all of the
    //       processes binding to the same address must have the same
    //       effective UID.  This option can be employed with both TCP and
    //       UDP sockets.

    int reuse = 1;
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR,
            (const char*)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

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
    bzero((char *) &server_fd, sizeof(server_fd));

    server_fd.sin_family = AF_INET;            // IPv4
    server_fd.sin_addr.s_addr = INADDR_ANY;    // All interfaces, 0.0.0.0.
    // Convert the port number from host byte order to network byte order.
    server_fd.sin_port = htons(port);

    int result = bind(
            socket_descriptor,
            (struct sockaddr *) &server_fd,
            sizeof(server_fd));

    // If the binding fails with this port and no particular port was specified
    // in the LF program, then try the next few ports in sequence.
    while (result != 0
            && specified_port == 0
            && port >= STARTING_PORT
            && port <= STARTING_PORT + PORT_RANGE_LIMIT) {
        lf_print("RTI failed to get port %d. Trying %d.", port, port + 1);
        port++;
        server_fd.sin_port = htons(port);
        result = bind(
                socket_descriptor,
                (struct sockaddr *) &server_fd,
                sizeof(server_fd));
    }
    if (result != 0) {
        if (specified_port == 0) {
            lf_print_error_and_exit("Failed to bind the RTI socket. Cannot find a usable port. "
                    "Consider increasing PORT_RANGE_LIMIT in net_common.h.");
        } else {
            lf_print_error_and_exit("Failed to bind the RTI socket. Specified port is not available. "
                    "Consider leaving the port unspecified");
        }
    }
    char* type = "TCP";
    if (socket_type == UDP) {
        type = "UDP";
    }
    lf_print("RTI using %s port %d for federation %s.", type, port, _RTI.federation_id);

    if (socket_type == TCP) {
        _RTI.final_port_TCP = port;
        // Enable listening for socket connections.
        // The second argument is the maximum number of queued socket requests,
        // which according to the Mac man page is limited to 128.
        listen(socket_descriptor, 128);
    } else if (socket_type == UDP) {
        _RTI.final_port_UDP = port;
        // No need to listen on the UDP socket
    }

    return socket_descriptor;
}

void send_tag_advance_grant(federate_t* fed, tag_t tag) {
    if (fed->state == NOT_CONNECTED
            || lf_tag_compare(tag, fed->last_granted) <= 0
            || lf_tag_compare(tag, fed->last_provisionally_granted) < 0
    ) {
        return;
    }
    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (_RTI.federates[fed->id].state == PENDING) {
        // Need to wait here.
        pthread_cond_wait(&_RTI.sent_start_time, &_RTI.rti_mutex);
    }
    size_t message_length = 1 + sizeof(int64_t) + sizeof(uint32_t);
    unsigned char buffer[message_length];
    buffer[0] = MSG_TYPE_TAG_ADVANCE_GRANT;
    encode_int64(tag.time, &(buffer[1]));
    encode_int32((int32_t)tag.microstep, &(buffer[1 + sizeof(int64_t)]));

    if (_RTI.tracing_enabled) {
        tracepoint_RTI_to_federate(send_TAG, fed->id, &tag);
    }
    // This function is called in send_advance_grant_if_safe(), which is a long
    // function. During this call, the socket might close, causing the following write_to_socket
    // to fail. Consider a failure here a soft failure and update the federate's status.
    ssize_t bytes_written = write_to_socket(fed->socket, message_length, buffer);
    if (bytes_written < (ssize_t)message_length) {
        lf_print_error("RTI failed to send tag advance grant to federate %d.", fed->id);
        if (bytes_written < 0) {
            fed->state = NOT_CONNECTED;
            // FIXME: We need better error handling, but don't stop other execution here.
            // mark_federate_requesting_stop(fed);
        }
    } else {
        fed->last_granted = tag;
        LF_PRINT_LOG("RTI sent to federate %d the tag advance grant (TAG) (%lld, %u).",
                fed->id, tag.time - start_time, tag.microstep);
    }
}

tag_t transitive_next_event(federate_t* fed, tag_t candidate, bool visited[]) {
    if (visited[fed->id] || fed->state == NOT_CONNECTED) {
        // Federate has stopped executing or we have visited it before.
        // No point in checking upstream federates.
        return candidate;
    }

    visited[fed->id] = true;
    tag_t result = fed->next_event;

    // If the candidate is less than this federate's next_event, use the candidate.
    if (lf_tag_compare(candidate, result) < 0) {
        result = candidate;
    }

    // The result cannot be earlier than the start time.
    if (result.time < start_time) {
        // Earliest next event cannot be before the start time.
        result = (tag_t){.time = start_time, .microstep = 0u};
    }

    // Check upstream federates to see whether any of them might send
    // an event that would result in an earlier next event.
    for (int i = 0; i < fed->num_upstream; i++) {
        tag_t upstream_result = transitive_next_event(
                &_RTI.federates[fed->upstream[i]], result, visited);

        // Add the "after" delay of the connection to the result.
        upstream_result = lf_delay_tag(upstream_result, fed->upstream_delay[i]);

        // If the adjusted event time is less than the result so far, update the result.
        if (lf_tag_compare(upstream_result, result) < 0) {
            result = upstream_result;
        }
    }
    if (lf_tag_compare(result, fed->completed) < 0) {
        result = fed->completed;
    }
    return result;
}

void send_provisional_tag_advance_grant(federate_t* fed, tag_t tag) {
    if (fed->state == NOT_CONNECTED
            || lf_tag_compare(tag, fed->last_granted) <= 0
            || lf_tag_compare(tag, fed->last_provisionally_granted) <= 0
    ) {
        return;
    }
    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (_RTI.federates[fed->id].state == PENDING) {
        // Need to wait here.
        pthread_cond_wait(&_RTI.sent_start_time, &_RTI.rti_mutex);
    }
    size_t message_length = 1 + sizeof(int64_t) + sizeof(uint32_t);
    unsigned char buffer[message_length];
    buffer[0] = MSG_TYPE_PROVISIONAL_TAG_ADVANCE_GRANT;
    encode_int64(tag.time, &(buffer[1]));
    encode_int32((int32_t)tag.microstep, &(buffer[1 + sizeof(int64_t)]));

    if (_RTI.tracing_enabled){
        tracepoint_RTI_to_federate(send_PTAG, fed->id, &tag);
    }
    // This function is called in send_advance_grant_if_safe(), which is a long
    // function. During this call, the socket might close, causing the following write_to_socket
    // to fail. Consider a failure here a soft failure and update the federate's status.
    ssize_t bytes_written = write_to_socket(fed->socket, message_length, buffer);

    if (bytes_written < (ssize_t)message_length) {
        lf_print_error("RTI failed to send tag advance grant to federate %d.", fed->id);
        if (bytes_written < 0) {
            fed->state = NOT_CONNECTED;
            // FIXME: We need better error handling, but don't stop other execution here.
            // mark_federate_requesting_stop(fed);
        }
    } else {
        fed->last_provisionally_granted = tag;
        LF_PRINT_LOG("RTI sent to federate %d the Provisional Tag Advance Grant (PTAG) (%lld, %u).",
                fed->id, tag.time - start_time, tag.microstep);

        // Send PTAG to all upstream federates, if they have not had
        // a later or equal PTAG or TAG sent previously and if their transitive
        // NET is greater than or equal to the tag.
        // NOTE: This could later be replaced with a TNET mechanism once
        // we have an available encoding of causality interfaces.
        // That might be more efficient.
        for (int j = 0; j < fed->num_upstream; j++) {
            federate_t* upstream = &_RTI.federates[fed->upstream[j]];

            // Ignore this federate if it has resigned.
            if (upstream->state == NOT_CONNECTED) continue;

            // To handle cycles, need to create a boolean array to keep
            // track of which upstream federates have been visited.
            bool* visited = (bool*)calloc(_RTI.number_of_federates, sizeof(bool)); // Initializes to 0.

            // Find the (transitive) next event tag upstream.
            tag_t upstream_next_event = transitive_next_event(
                    upstream, upstream->next_event, visited);
            free(visited);

            // If these tags are equal, then
            // a TAG or PTAG should have already been granted,
            // in which case, another will not be sent. But it
            // may not have been already granted.
            if (lf_tag_compare(upstream_next_event, tag) >= 0) {
                send_provisional_tag_advance_grant(upstream, tag);
            }
        }
    }
}

bool send_advance_grant_if_safe(federate_t* fed) {

    // Find the earliest LTC of upstream federates.
    tag_t min_upstream_completed = FOREVER_TAG;

    for (int j = 0; j < fed->num_upstream; j++) {
        federate_t* upstream = &_RTI.federates[fed->upstream[j]];

        // Ignore this federate if it has resigned.
        if (upstream->state == NOT_CONNECTED) continue;

        tag_t candidate = lf_delay_tag(upstream->completed, fed->upstream_delay[j]);

        if (lf_tag_compare(candidate, min_upstream_completed) < 0) {
            min_upstream_completed = candidate;
        }
    }
    LF_PRINT_LOG("Minimum upstream LTC for fed %d is (%lld, %u) "
            "(adjusted by after delay).",
            fed->id,
            min_upstream_completed.time - start_time, min_upstream_completed.microstep);
    if (lf_tag_compare(min_upstream_completed, fed->last_granted) > 0
        && lf_tag_compare(min_upstream_completed, fed->next_event) >= 0 // The federate has to advance its tag
    ) {
        send_tag_advance_grant(fed, min_upstream_completed);
        return true;
    }

    // Can't make progress based only on upstream LTCs.
    // If all (transitive) upstream federates of the federate
    // have earliest event tags such that the
    // federate can now advance its tag, then send it a TAG message.
    // Find the earliest event time of each such upstream federate,
    // adjusted by delays on the connections.

    // To handle cycles, need to create a boolean array to keep
    // track of which upstream federates have been visited.
    bool* visited = (bool*)calloc(_RTI.number_of_federates, sizeof(bool)); // Initializes to 0.

    // Find the tag of the earliest possible incoming message from
    // upstream federates.
    tag_t t_d = FOREVER_TAG;
    LF_PRINT_DEBUG("NOTE: FOREVER is displayed as (%lld, %u) and NEVER as (%lld, %u)",
            FOREVER_TAG.time - start_time, FOREVER_TAG.microstep,
            NEVER - start_time, 0u);

    for (int j = 0; j < fed->num_upstream; j++) {
        federate_t* upstream = &_RTI.federates[fed->upstream[j]];

        // Ignore this federate if it has resigned.
        if (upstream->state == NOT_CONNECTED) continue;

        // Find the (transitive) next event tag upstream.
        tag_t upstream_next_event = transitive_next_event(
                upstream, upstream->next_event, visited);

        LF_PRINT_DEBUG("Earliest next event upstream of fed %d at fed %d has tag (%lld, %u).",
                fed->id,
                upstream->id,
                upstream_next_event.time - start_time, upstream_next_event.microstep);

        // Adjust by the "after" delay.
        // Note that "no delay" is encoded as NEVER,
        // whereas one microstep delay is encoded as 0LL.
        tag_t candidate = lf_delay_tag(upstream_next_event, fed->upstream_delay[j]);

        if (lf_tag_compare(candidate, t_d) < 0) {
            t_d = candidate;
        }
    }
    free(visited);

    LF_PRINT_LOG("Earliest next event upstream has tag (%lld, %u).",
            t_d.time - start_time, t_d.microstep);

    if (
        lf_tag_compare(t_d, fed->next_event) > 0       // The federate has something to do.
        && lf_tag_compare(t_d, fed->last_provisionally_granted) >= 0  // The grant is not redundant
                                                                      // (equal is important to override any previous
                                                                      // PTAGs).
        && lf_tag_compare(t_d, fed->last_granted) > 0  // The grant is not redundant.
    ) {
        // All upstream federates have events with a larger tag than fed, so it is safe to send a TAG.
        LF_PRINT_LOG("Earliest upstream message time for fed %d is " PRINTF_TAG
                "(adjusted by after delay). Granting tag advance for " PRINTF_TAG,
                fed->id,
                t_d.time - lf_time_start(), t_d.microstep,
                fed->next_event.time - lf_time_start(),
                fed->next_event.microstep);
        send_tag_advance_grant(fed, fed->next_event);
    } else if (
        lf_tag_compare(t_d, fed->next_event) == 0      // The federate has something to do.
        && lf_tag_compare(t_d, fed->last_provisionally_granted) > 0  // The grant is not redundant.
        && lf_tag_compare(t_d, fed->last_granted) > 0  // The grant is not redundant.
    ) {
        // Some upstream federate has an event that has the same tag as fed's next event, so we can only provisionally
        // grant a TAG (via a PTAG).
        LF_PRINT_LOG("Earliest upstream message time for fed %d is " PRINTF_TAG
            " (adjusted by after delay). Granting provisional tag advance.",
            fed->id,
            t_d.time - start_time, t_d.microstep);

        send_provisional_tag_advance_grant(fed, t_d);
    }
    return false;
}

void send_downstream_advance_grants_if_safe(federate_t* fed, bool visited[]) {
    visited[fed->id] = true;
    for (int i = 0; i < fed->num_downstream; i++) {
        federate_t* downstream = &_RTI.federates[fed->downstream[i]];
        if (visited[downstream->id]) continue;
        send_advance_grant_if_safe(downstream);
        send_downstream_advance_grants_if_safe(downstream, visited);
    }
}

void update_federate_next_event_tag_locked(uint16_t federate_id, tag_t next_event_tag) {
    tag_t min_in_transit_tag = get_minimum_in_transit_message_tag(_RTI.federates[federate_id].in_transit_message_tags);
    if (lf_tag_compare(
            min_in_transit_tag,
            next_event_tag
        ) < 0
    ) {
        next_event_tag = min_in_transit_tag;
    }

    _RTI.federates[federate_id].next_event = next_event_tag;

    LF_PRINT_DEBUG(
       "RTI: Updated the recorded next event tag for federate %d to " PRINTF_TAG,
       federate_id,
       next_event_tag.time - lf_time_start(),
       next_event_tag.microstep
    );

    // Check to see whether we can reply now with a tag advance grant.
    // If the federate has no upstream federates, then it does not wait for
    // nor expect a reply. It just proceeds to advance time.
    if (_RTI.federates[federate_id].num_upstream > 0) {
        send_advance_grant_if_safe(&_RTI.federates[federate_id]);
    }
    // Check downstream federates to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which upstream federates have been visited.
    bool* visited = (bool*)calloc(_RTI.number_of_federates, sizeof(bool)); // Initializes to 0.
    send_downstream_advance_grants_if_safe(&_RTI.federates[federate_id], visited);
    free(visited);
}

void handle_port_absent_message(federate_t* sending_federate, unsigned char* buffer) {
    size_t message_size = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int64_t) + sizeof(uint32_t);

    read_from_socket_errexit(sending_federate->socket, message_size, &(buffer[1]),
                            " RTI failed to read port absent message from federate %u.",
                            sending_federate->id);

    uint16_t reactor_port_id = extract_uint16(&(buffer[1]));
    uint16_t federate_id = extract_uint16(&(buffer[1 + sizeof(uint16_t)]));
    tag_t tag = extract_tag(&(buffer[1 + 2 * sizeof(uint16_t)]));

    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_PORT_ABS, federate_id, &tag);
    }

    // Need to acquire the mutex lock to ensure that the thread handling
    // messages coming from the socket connected to the destination does not
    // issue a TAG before this message has been forwarded.
    pthread_mutex_lock(&_RTI.rti_mutex);

    // If the destination federate is no longer connected, issue a warning
    // and return.
    if (_RTI.federates[federate_id].state == NOT_CONNECTED) {
        pthread_mutex_unlock(&_RTI.rti_mutex);
        lf_print_warning("RTI: Destination federate %d is no longer connected. Dropping message.",
                federate_id);
        LF_PRINT_LOG("Fed status: next_event (%lld, %d), "
                "completed (%lld, %d), "
                "last_granted (%lld, %d), "
                "last_provisionally_granted (%lld, %d).",
                _RTI.federates[federate_id].next_event.time - start_time,
                _RTI.federates[federate_id].next_event.microstep,
                _RTI.federates[federate_id].completed.time - start_time,
                _RTI.federates[federate_id].completed.microstep,
                _RTI.federates[federate_id].last_granted.time - start_time,
                _RTI.federates[federate_id].last_granted.microstep,
                _RTI.federates[federate_id].last_provisionally_granted.time - start_time,
                _RTI.federates[federate_id].last_provisionally_granted.microstep
        );
        return;
    }

    LF_PRINT_LOG("RTI forwarding port absent message for port %u to federate %u.",
                reactor_port_id,
                federate_id);

    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (_RTI.federates[federate_id].state == PENDING) {
        // Need to wait here.
        pthread_cond_wait(&_RTI.sent_start_time, &_RTI.rti_mutex);
    }

    // Forward the message.
    int destination_socket = _RTI.federates[federate_id].socket;
    if (_RTI.tracing_enabled) {
        tracepoint_RTI_to_federate(send_PORT_ABS, federate_id, &tag);
    }
    write_to_socket_errexit(destination_socket, message_size + 1, buffer,
            "RTI failed to forward message to federate %d.", federate_id);

    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void handle_timed_message(federate_t* sending_federate, unsigned char* buffer) {
    size_t header_size = 1 + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t) + sizeof(int64_t) + sizeof(uint32_t);
    // Read the header, minus the first byte which has already been read.
    read_from_socket_errexit(sending_federate->socket, header_size - 1, &(buffer[1]), "RTI failed to read the timed message header from remote federate.");
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

    LF_PRINT_LOG("RTI received message from federate %d for federate %u port %u with intended tag "
            PRINTF_TAG ". Forwarding.",
            sending_federate->id, federate_id, reactor_port_id,
            intended_tag.time - lf_time_start(), intended_tag.microstep);

    read_from_socket_errexit(sending_federate->socket, bytes_to_read, &(buffer[header_size]),
                     "RTI failed to read timed message from federate %d.", federate_id);
    size_t bytes_read = bytes_to_read + header_size;
    // Following only works for string messages.
    // LF_PRINT_DEBUG("Message received by RTI: %s.", buffer + header_size);

    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_TAGGED_MSG, sending_federate->id, &intended_tag);
    }

    // Need to acquire the mutex lock to ensure that the thread handling
    // messages coming from the socket connected to the destination does not
    // issue a TAG before this message has been forwarded.
    pthread_mutex_lock(&_RTI.rti_mutex);

    // If the destination federate is no longer connected, issue a warning
    // and return.
    if (_RTI.federates[federate_id].state == NOT_CONNECTED) {
        pthread_mutex_unlock(&_RTI.rti_mutex);
        lf_print_warning("RTI: Destination federate %d is no longer connected. Dropping message.",
                federate_id);
        LF_PRINT_LOG("Fed status: next_event (%lld, %d), "
                "completed (%lld, %d), "
                "last_granted (%lld, %d), "
                "last_provisionally_granted (%lld, %d).",
                _RTI.federates[federate_id].next_event.time - start_time,
                _RTI.federates[federate_id].next_event.microstep,
                _RTI.federates[federate_id].completed.time - start_time,
                _RTI.federates[federate_id].completed.microstep,
                _RTI.federates[federate_id].last_granted.time - start_time,
                _RTI.federates[federate_id].last_granted.microstep,
                _RTI.federates[federate_id].last_provisionally_granted.time - start_time,
                _RTI.federates[federate_id].last_provisionally_granted.microstep
        );
        return;
    }

    // Forward the message or message chunk.
    int destination_socket = _RTI.federates[federate_id].socket;

    LF_PRINT_DEBUG(
        "RTI forwarding message to port %d of federate %hu of length %zu.",
        reactor_port_id,
        federate_id,
        length
    );

    // Record this in-transit message in federate's in-transit message queue.
    if (lf_tag_compare(_RTI.federates[federate_id].completed, intended_tag) < 0) {
        // Add a record of this message to the list of in-transit messages to this federate.
        add_in_transit_message_record(
            _RTI.federates[federate_id].in_transit_message_tags,
            intended_tag
        );
        LF_PRINT_DEBUG(
            "RTI: Adding a message with tag " PRINTF_TAG " to the list of in-transit messages for federate %d.",
            intended_tag.time - lf_time_start(),
            intended_tag.microstep,
            federate_id
        );
    } else {
        lf_print_error(
            "RTI: Federate %d has already completed tag " PRINTF_TAG
            ", but there is an in-transit message with tag " PRINTF_TAG " from federate %hu. "
            "This is going to cause an STP violation under centralized coordination.",
            federate_id,
            _RTI.federates[federate_id].completed.time - lf_time_start(),
            _RTI.federates[federate_id].completed.microstep,
            intended_tag.time - lf_time_start(),
            intended_tag.microstep,
            sending_federate->id
        );
        // FIXME: Drop the federate?
    }

    // Need to make sure that the destination federate's thread has already
    // sent the starting MSG_TYPE_TIMESTAMP message.
    while (_RTI.federates[federate_id].state == PENDING) {
        // Need to wait here.
        pthread_cond_wait(&_RTI.sent_start_time, &_RTI.rti_mutex);
    }

    if (_RTI.tracing_enabled) {
        tracepoint_RTI_to_federate(send_TAGGED_MSG, federate_id, &intended_tag);
    }

    write_to_socket_errexit(destination_socket, bytes_read, buffer,
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
        read_from_socket_errexit(sending_federate->socket, bytes_to_read, buffer,
                "RTI failed to read message chunks.");
        total_bytes_read += bytes_to_read;

        // FIXME: a mutex needs to be held for this so that other threads
        // do not write to destination_socket and cause interleaving. However,
        // holding the _RTI.rti_mutex might be very expensive. Instead, each outgoing
        // socket should probably have its own mutex.
        write_to_socket_errexit(destination_socket, bytes_to_read, buffer,
                "RTI failed to send message chunks.");
    }

    update_federate_next_event_tag_locked(federate_id, intended_tag);

    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void handle_logical_tag_complete(federate_t* fed) {
    unsigned char buffer[sizeof(int64_t) + sizeof(uint32_t)];
    read_from_socket_errexit(fed->socket, sizeof(int64_t) + sizeof(uint32_t), buffer,
            "RTI failed to read the content of the logical tag complete from federate %d.", fed->id);

    // FIXME: Consolidate this message with NET to get NMR (Next Message Request).
    // Careful with handling startup and shutdown.
    pthread_mutex_lock(&_RTI.rti_mutex);

    fed->completed = extract_tag(buffer);
    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_LTC, fed->id, &(fed->completed));
    }

    LF_PRINT_LOG("RTI received from federate %d the Logical Tag Complete (LTC) (%lld, %u).",
                fed->id, fed->completed.time - start_time, fed->completed.microstep);


    // See if we can remove any of the recorded in-transit messages for this.
    clean_in_transit_message_record_up_to_tag(fed->in_transit_message_tags, fed->completed);

    // Check downstream federates to see whether they should now be granted a TAG.
    for (int i = 0; i < fed->num_downstream; i++) {
        federate_t* downstream = &_RTI.federates[fed->downstream[i]];
        send_advance_grant_if_safe(downstream);
        bool* visited = (bool*)calloc(_RTI.number_of_federates, sizeof(bool)); // Initializes to 0.
        send_downstream_advance_grants_if_safe(downstream, visited);
        free(visited);
    }

    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void handle_next_event_tag(federate_t* fed) {
    unsigned char buffer[sizeof(int64_t) + sizeof(uint32_t)];
    read_from_socket_errexit(fed->socket, sizeof(int64_t) + sizeof(uint32_t), buffer,
            "RTI failed to read the content of the next event tag from federate %d.", fed->id);

    // Acquire a mutex lock to ensure that this state does not change while a
    // message is in transport or being used to determine a TAG.
    pthread_mutex_lock(&_RTI.rti_mutex); // FIXME: Instead of using a mutex,
                                         // it might be more efficient to use a
                                         // select() mechanism to read and process
                                         // federates' buffers in an orderly fashion.


    tag_t intended_tag = extract_tag(buffer);
    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_NET, fed->id, &intended_tag);
    }
    LF_PRINT_LOG("RTI received from federate %d the Next Event Tag (NET) " PRINTF_TAG,
        fed->id, intended_tag.time - start_time,
        intended_tag.microstep);
    update_federate_next_event_tag_locked(
        fed->id,
        intended_tag
    );
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

/////////////////// STOP functions ////////////////////

/**
 * Boolean used to prevent the RTI from sending the
 * MSG_TYPE_STOP_GRANTED message multiple times.
 */
bool _lf_rti_stop_granted_already_sent_to_federates = false;

void _lf_rti_broadcast_stop_time_to_federates_already_locked() {
    if (_lf_rti_stop_granted_already_sent_to_federates == true) {
        return;
    }
    // Reply with a stop granted to all federates
    unsigned char outgoing_buffer[MSG_TYPE_STOP_GRANTED_LENGTH];
    ENCODE_STOP_GRANTED(outgoing_buffer, _RTI.max_stop_tag.time, _RTI.max_stop_tag.microstep);

    // Iterate over federates and send each the message.
    for (int i = 0; i < _RTI.number_of_federates; i++) {
        if (_RTI.federates[i].state == NOT_CONNECTED) {
            continue;
        }
        if (lf_tag_compare(_RTI.federates[i].next_event, _RTI.max_stop_tag) >= 0) {
            // Need the next_event to be no greater than the stop tag.
            _RTI.federates[i].next_event = _RTI.max_stop_tag;
        }
        if (_RTI.tracing_enabled) {
            tracepoint_RTI_to_federate(send_STOP_GRN, _RTI.federates[i].id, &_RTI.max_stop_tag);
        }
        write_to_socket_errexit(_RTI.federates[i].socket, MSG_TYPE_STOP_GRANTED_LENGTH, outgoing_buffer,
                "RTI failed to send MSG_TYPE_STOP_GRANTED message to federate %d.", _RTI.federates[i].id);
    }

    LF_PRINT_LOG("RTI sent to federates MSG_TYPE_STOP_GRANTED with tag (%lld, %u).",
                _RTI.max_stop_tag.time - start_time,
                _RTI.max_stop_tag.microstep);
    _lf_rti_stop_granted_already_sent_to_federates = true;
}

void mark_federate_requesting_stop(federate_t* fed) {
    if (!fed->requested_stop) {
        // Assume that the federate
        // has requested stop
        _RTI.num_feds_handling_stop++;
        fed->requested_stop = true;
    }
    if (_RTI.num_feds_handling_stop == _RTI.number_of_federates) {
        // We now have information about the stop time of all
        // federates.
        _lf_rti_broadcast_stop_time_to_federates_already_locked();
    }
}

void handle_stop_request_message(federate_t* fed) {
    LF_PRINT_DEBUG("RTI handling stop_request from federate %d.", fed->id);

    size_t bytes_to_read = MSG_TYPE_STOP_REQUEST_LENGTH - 1;
    unsigned char buffer[bytes_to_read];
    read_from_socket_errexit(fed->socket, bytes_to_read, buffer,
            "RTI failed to read the MSG_TYPE_STOP_REQUEST payload from federate %d.", fed->id);

    // Acquire a mutex lock to ensure that this state does change while a
    // message is in transport or being used to determine a TAG.
    pthread_mutex_lock(&_RTI.rti_mutex);

    // Check whether we have already received a stop_tag
    // from this federate
    if (_RTI.federates[fed->id].requested_stop) {
        // Ignore this request
        pthread_mutex_unlock(&_RTI.rti_mutex);
        return;
    }

    // Extract the proposed stop tag for the federate
    tag_t proposed_stop_tag = extract_tag(buffer);

    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_STOP_REQ, fed->id, &proposed_stop_tag);
    }

    // Update the maximum stop tag received from federates
    if (lf_tag_compare(proposed_stop_tag, _RTI.max_stop_tag) > 0) {
        _RTI.max_stop_tag = proposed_stop_tag;
    }

    LF_PRINT_LOG("RTI received from federate %d a MSG_TYPE_STOP_REQUEST message with tag (%lld, %u).",
            fed->id, proposed_stop_tag.time - start_time, proposed_stop_tag.microstep);

    // If this federate has not already asked
    // for a stop, add it to the tally.
    mark_federate_requesting_stop(fed);

    if (_RTI.num_feds_handling_stop == _RTI.number_of_federates) {
        // We now have information about the stop time of all
        // federates. This is extremely unlikely, but it can occur
        // all federates call lf_request_stop() at the same tag.
        pthread_mutex_unlock(&_RTI.rti_mutex);
        return;
    }
    // Forward the stop request to all other federates that have not
    // also issued a stop request.
    unsigned char stop_request_buffer[MSG_TYPE_STOP_REQUEST_LENGTH];
    ENCODE_STOP_REQUEST(stop_request_buffer, _RTI.max_stop_tag.time, _RTI.max_stop_tag.microstep);

    // Iterate over federates and send each the MSG_TYPE_STOP_REQUEST message
    // if we do not have a stop_time already for them.
    for (int i = 0; i < _RTI.number_of_federates; i++) {
        if (_RTI.federates[i].id != fed->id && _RTI.federates[i].requested_stop == false) {
            if (_RTI.federates[i].state == NOT_CONNECTED) {
                mark_federate_requesting_stop(&_RTI.federates[i]);
                continue;
            }
            if (_RTI.tracing_enabled) {
                tracepoint_RTI_to_federate(send_STOP_REQ, _RTI.federates[i].id, &_RTI.max_stop_tag);
            }
            write_to_socket_errexit(_RTI.federates[i].socket, MSG_TYPE_STOP_REQUEST_LENGTH, stop_request_buffer,
                    "RTI failed to forward MSG_TYPE_STOP_REQUEST message to federate %d.", _RTI.federates[i].id);
            if (_RTI.tracing_enabled) {
                tracepoint_RTI_to_federate(send_STOP_REQ, _RTI.federates[i].id, &_RTI.max_stop_tag);
            }
        }
    }
    LF_PRINT_LOG("RTI forwarded to federates MSG_TYPE_STOP_REQUEST with tag (%lld, %u).",
                _RTI.max_stop_tag.time - start_time,
                _RTI.max_stop_tag.microstep);
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void handle_stop_request_reply(federate_t* fed) {
    size_t bytes_to_read = MSG_TYPE_STOP_REQUEST_REPLY_LENGTH - 1;
    unsigned char buffer_stop_time[bytes_to_read];
    read_from_socket_errexit(fed->socket, bytes_to_read, buffer_stop_time,
            "RTI failed to read the reply to MSG_TYPE_STOP_REQUEST message from federate %d.", fed->id);

    tag_t federate_stop_tag = extract_tag(buffer_stop_time);

    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_STOP_REQ_REP, fed->id, &federate_stop_tag);
    }

    LF_PRINT_LOG("RTI received from federate %d STOP reply tag (%lld, %u).", fed->id,
            federate_stop_tag.time - start_time,
            federate_stop_tag.microstep);

    // Acquire the mutex lock so that we can change the state of the RTI
    pthread_mutex_lock(&_RTI.rti_mutex);
    // If the federate has not requested stop before, count the reply
    if (lf_tag_compare(federate_stop_tag, _RTI.max_stop_tag) > 0) {
        _RTI.max_stop_tag = federate_stop_tag;
    }
    mark_federate_requesting_stop(fed);
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

//////////////////////////////////////////////////

void handle_address_query(uint16_t fed_id) {
    // Use buffer both for reading and constructing the reply.
    // The length is what is needed for the reply.
    unsigned char buffer[sizeof(int32_t)];
    ssize_t bytes_read = read_from_socket(_RTI.federates[fed_id].socket, sizeof(uint16_t), (unsigned char*)buffer);
    if (bytes_read == 0) {
        lf_print_error_and_exit("Failed to read address query.");
    }
    uint16_t remote_fed_id = extract_uint16(buffer);

    if (_RTI.tracing_enabled){
        tracepoint_RTI_from_federate(receive_ADR_QR, fed_id, NULL);
    }

    LF_PRINT_DEBUG("RTI received address query from %d for %d.", fed_id, remote_fed_id);

    // NOTE: server_port initializes to -1, which means the RTI does not know
    // the port number because it has not yet received an MSG_TYPE_ADDRESS_ADVERTISEMENT message
    // from this federate. In that case, it will respond by sending -1.

    // Encode the port number.
    encode_int32(_RTI.federates[remote_fed_id].server_port, (unsigned char*)buffer);
    // Send the port number (which could be -1).
    write_to_socket_errexit(_RTI.federates[fed_id].socket, sizeof(int32_t), (unsigned char*)buffer,
                        "Failed to write port number to socket of federate %d.", fed_id);

    // Send the server IP address to federate.
    write_to_socket_errexit(_RTI.federates[fed_id].socket, sizeof(_RTI.federates[remote_fed_id].server_ip_addr),
                        (unsigned char *)&_RTI.federates[remote_fed_id].server_ip_addr,
                        "Failed to write ip address to socket of federate %d.", fed_id);

    if (_RTI.federates[remote_fed_id].server_port != -1) {
        LF_PRINT_DEBUG("Replied to address query from federate %d with address %s:%d.",
                fed_id, _RTI.federates[remote_fed_id].server_hostname, _RTI.federates[remote_fed_id].server_port);
    }
}

void handle_address_ad(uint16_t federate_id) {
    // Read the port number of the federate that can be used for physical
    // connections to other federates
    int32_t server_port = -1;
    unsigned char buffer[sizeof(int32_t)];
    ssize_t bytes_read = read_from_socket(_RTI.federates[federate_id].socket, sizeof(int32_t), (unsigned char *)buffer);

    if (bytes_read < (ssize_t)sizeof(int32_t)) {
        LF_PRINT_DEBUG("Error reading port data from federate %d.", _RTI.federates[federate_id].id);
        // Leave the server port at -1, which means "I don't know".
        return;
    }

    server_port = extract_int32(buffer);

    assert(server_port < 65536);

    pthread_mutex_lock(&_RTI.rti_mutex);
    _RTI.federates[federate_id].server_port = server_port;
    if (_RTI.tracing_enabled) {
        tracepoint_RTI_from_federate(receive_ADR_AD, federate_id, NULL);
    }
     LF_PRINT_LOG("Received address advertisement from federate %d.", federate_id);
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void handle_timestamp(federate_t *my_fed) {
    unsigned char buffer[sizeof(int64_t)];
    // Read bytes from the socket. We need 8 bytes.
    ssize_t bytes_read = read_from_socket(my_fed->socket, sizeof(int64_t), (unsigned char*)&buffer);
    if (bytes_read < (ssize_t)sizeof(int64_t)) {
        lf_print_error("ERROR reading timestamp from federate %d.\n", my_fed->id);
    }

    int64_t timestamp = swap_bytes_if_big_endian_int64(*((int64_t *)(&buffer)));
    if (_RTI.tracing_enabled) {
        tag_t tag = {.time = timestamp, .microstep = 0};
        tracepoint_RTI_from_federate(receive_TIMESTAMP, my_fed->id, &tag);
    }
    LF_PRINT_LOG("RTI received timestamp message: %lld.", timestamp);

    pthread_mutex_lock(&_RTI.rti_mutex);
    _RTI.num_feds_proposed_start++;
    if (timestamp > _RTI.max_start_time) {
        _RTI.max_start_time = timestamp;
    }
    if (_RTI.num_feds_proposed_start == _RTI.number_of_federates) {
        // All federates have proposed a start time.
        pthread_cond_broadcast(&_RTI.received_start_times);
    } else {
        // Some federates have not yet proposed a start time.
        // wait for a notification.
        while (_RTI.num_feds_proposed_start < _RTI.number_of_federates) {
            // FIXME: Should have a timeout here?
            pthread_cond_wait(&_RTI.received_start_times, &_RTI.rti_mutex);
        }
    }

    pthread_mutex_unlock(&_RTI.rti_mutex);

    // Send back to the federate the maximum time plus an offset on a TIMESTAMP
    // message.
    unsigned char start_time_buffer[MSG_TYPE_TIMESTAMP_LENGTH];
    start_time_buffer[0] = MSG_TYPE_TIMESTAMP;
    // Add an offset to this start time to get everyone starting together.
    start_time = _RTI.max_start_time + DELAY_START;
    encode_int64(swap_bytes_if_big_endian_int64(start_time), &start_time_buffer[1]);

    if (_RTI.tracing_enabled) {
        tag_t tag = {.time = start_time, .microstep = 0};
        tracepoint_RTI_to_federate(send_TIMESTAMP, my_fed->id, &tag);
    }
    ssize_t bytes_written = write_to_socket(
        my_fed->socket, MSG_TYPE_TIMESTAMP_LENGTH,
        start_time_buffer
    );
    if (bytes_written < MSG_TYPE_TIMESTAMP_LENGTH) {
        lf_print_error("Failed to send the starting time to federate %d.", my_fed->id);
    }

    pthread_mutex_lock(&_RTI.rti_mutex);
    // Update state for the federate to indicate that the MSG_TYPE_TIMESTAMP
    // message has been sent. That MSG_TYPE_TIMESTAMP message grants time advance to
    // the federate to the start time.
    my_fed->state = GRANTED;
    pthread_cond_broadcast(&_RTI.sent_start_time);
    LF_PRINT_LOG("RTI sent start time %lld to federate %d.", start_time, my_fed->id);
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void send_physical_clock(unsigned char message_type, federate_t* fed, socket_type_t socket_type) {
    if (fed->state == NOT_CONNECTED) {
        lf_print_warning("Clock sync: RTI failed to send physical time to federate %d. Socket not connected.\n",
                fed->id);
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
        ssize_t bytes_written = sendto(_RTI.socket_descriptor_UDP, buffer, 1 + sizeof(int64_t), 0,
                                (struct sockaddr*)&fed->UDP_addr, sizeof(fed->UDP_addr));
        if (bytes_written < (ssize_t)sizeof(int64_t) + 1) {
            lf_print_warning("Clock sync: RTI failed to send physical time to federate %d: %s\n",
                        fed->id,
                        strerror(errno));
            return;
        }
    } else if (socket_type == TCP) {
        LF_PRINT_DEBUG("Clock sync:  RTI sending TCP message type %u.", buffer[0]);
        write_to_socket_errexit(fed->socket, 1 + sizeof(int64_t), buffer,
                        "Clock sync: RTI failed to send physical time to federate %d: %s.",
                        fed->id,
                        strerror(errno));
    }
    LF_PRINT_DEBUG("Clock sync: RTI sent PHYSICAL_TIME_SYNC_MESSAGE with timestamp %lld to federate %d.",
                 current_physical_time,
                 fed->id);
}

void handle_physical_clock_sync_message(federate_t* my_fed, socket_type_t socket_type) {
    // Lock the mutex to prevent interference between sending the two
    // coded probe messages.
    pthread_mutex_lock(&_RTI.rti_mutex);
    // Reply with a T4 type message
    send_physical_clock(MSG_TYPE_CLOCK_SYNC_T4, my_fed, socket_type);
    // Send the corresponding coded probe immediately after,
    // but only if this is a UDP channel.
    if (socket_type == UDP) {
        send_physical_clock(MSG_TYPE_CLOCK_SYNC_CODED_PROBE, my_fed, socket_type);
    }
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void* clock_synchronization_thread(void* noargs) {

    // Wait until all federates have been notified of the start time.
    // FIXME: Use lf_ version of this when merged with master.
    pthread_mutex_lock(&_RTI.rti_mutex);
    while (_RTI.num_feds_proposed_start < _RTI.number_of_federates) {
        pthread_cond_wait(&_RTI.received_start_times, &_RTI.rti_mutex);
    }
    pthread_mutex_unlock(&_RTI.rti_mutex);

    // Wait until the start time before starting clock synchronization.
    // The above wait ensures that start_time has been set.
    interval_t ns_to_wait = start_time - lf_time_physical();

    if (ns_to_wait > 0LL) {
        struct timespec wait_time = {ns_to_wait / BILLION, ns_to_wait % BILLION};
        struct timespec rem_time;
        nanosleep(&wait_time, &rem_time);
    }

    // Initiate a clock synchronization every _RTI.clock_sync_period_ns
    struct timespec sleep_time = {(time_t) _RTI.clock_sync_period_ns / BILLION,
                                  _RTI.clock_sync_period_ns % BILLION};
    struct timespec remaining_time;

    bool any_federates_connected = true;
    while (any_federates_connected) {
        // Sleep
        nanosleep(&sleep_time, &remaining_time); // Can be interrupted
        any_federates_connected = false;
        for (int fed = 0; fed < _RTI.number_of_federates; fed++) {
            if (_RTI.federates[fed].state == NOT_CONNECTED) {
                // FIXME: We need better error handling here, but clock sync failure
                // should not stop execution.
                lf_print_error("Clock sync failed with federate %d. Not connected.", _RTI.federates[fed].id);
                // mark_federate_requesting_stop(&_RTI.federates[fed]);
                continue;
            } else if (!_RTI.federates[fed].clock_synchronization_enabled) {
                continue;
            }
            // Send the RTI's current physical time to the federate
            // Send on UDP.
            LF_PRINT_DEBUG("RTI sending T1 message to initiate clock sync round.");
            send_physical_clock(MSG_TYPE_CLOCK_SYNC_T1, &_RTI.federates[fed], UDP);

            // Listen for reply message, which should be T3.
            size_t message_size = 1 + sizeof(int32_t);
            unsigned char buffer[message_size];
            // Maximum number of messages that we discard before giving up on this cycle.
            // If the T3 message from this federate does not arrive and we keep receiving
            // other messages, then give up on this federate and move to the next federate.
            int remaining_attempts = 5;
            while (remaining_attempts > 0) {
                remaining_attempts--;
                int bytes_read = read_from_socket(_RTI.socket_descriptor_UDP, message_size, buffer);
                // If any errors occur, either discard the message or the clock sync round.
                if (bytes_read == message_size) {
                    if (buffer[0] == MSG_TYPE_CLOCK_SYNC_T3) {
                        int32_t fed_id = extract_int32(&(buffer[1]));
                        // Check that this message came from the correct federate.
                        if (fed_id != _RTI.federates[fed].id) {
                            // Message is from the wrong federate. Discard the message.
                            lf_print_warning("Clock sync: Received T3 message from federate %d, "
                                    "but expected one from %d. Discarding message.",
                                    fed_id, _RTI.federates[fed].id);
                            continue;
                        }
                        LF_PRINT_DEBUG("Clock sync: RTI received T3 message from federate %d.", fed_id);
                        handle_physical_clock_sync_message(&_RTI.federates[fed_id], UDP);
                        break;
                    } else {
                        // The message is not a T3 message. Discard the message and
                        // continue waiting for the T3 message. This is possibly a message
                        // from a previous cycle that was discarded.
                        lf_print_warning("Clock sync: Unexpected UDP message %u. Expected %u from federate %d. "
                                "Discarding message.",
                                buffer[0],
                                MSG_TYPE_CLOCK_SYNC_T3,
                                _RTI.federates[fed].id);
                        continue;
                    }
                } else {
                    lf_print_warning("Clock sync: Read from UDP socket failed: %s. "
                            "Skipping clock sync round for federate %d.",
                            strerror(errno),
                            _RTI.federates[fed].id);
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

void handle_federate_resign(federate_t *my_fed) {
    // Nothing more to do. Close the socket and exit.
    pthread_mutex_lock(&_RTI.rti_mutex);
    if (_RTI.tracing_enabled) {
        // Extract the tag, for tracing purposes
        size_t header_size = 1 + sizeof(tag_t);
        unsigned char buffer[header_size];
        // Read the header, minus the first byte which has already been read.
        read_from_socket_errexit(my_fed->socket, header_size - 1, &(buffer[1]),
                                 "RTI failed to read the timed message header from remote federate.");
        // Extract the tag sent by the resigning federate
        tag_t tag = extract_tag(&(buffer[1]));
        tracepoint_RTI_from_federate(receive_RESIGN, my_fed->id, &tag);
    }

    my_fed->state = NOT_CONNECTED;
    // FIXME: The following results in spurious error messages.
    // mark_federate_requesting_stop(my_fed);

    // Indicate that there will no further events from this federate.
    my_fed->next_event = FOREVER_TAG;

    // According to this: https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket,
    // the close should happen when receiving a 0 length message from the other end.
    // Here, we just signal the other side that no further writes to the socket are
    // forthcoming, which should result in the other end getting a zero-length reception.
    shutdown(my_fed->socket, SHUT_WR);
    // Do not close because this results in an error on the other side rather than
    // an orderly shutdown.
    // close(my_fed->socket); //  from unistd.h

    lf_print("Federate %d has resigned.", my_fed->id);

    // Check downstream federates to see whether they should now be granted a TAG.
    // To handle cycles, need to create a boolean array to keep
    // track of which upstream federates have been visited.
    bool* visited = (bool*)calloc(_RTI.number_of_federates, sizeof(bool)); // Initializes to 0.
    send_downstream_advance_grants_if_safe(my_fed, visited);
    free(visited);

    pthread_mutex_unlock(&_RTI.rti_mutex);
}

void* federate_thread_TCP(void* fed) {
    federate_t* my_fed = (federate_t*)fed;

    // Buffer for incoming messages.
    // This does not constrain the message size because messages
    // are forwarded piece by piece.
    unsigned char buffer[FED_COM_BUFFER_SIZE];

    // Listen for messages from the federate.
    while (my_fed->state != NOT_CONNECTED) {
        // Read no more than one byte to get the message type.
        ssize_t bytes_read = read_from_socket(my_fed->socket, 1, buffer);
        if (bytes_read < 1) {
            // Socket is closed
            lf_print_warning("RTI: Socket to federate %d is closed. Exiting the thread.", my_fed->id);
            my_fed->state = NOT_CONNECTED;
            my_fed->socket = -1;
            // FIXME: We need better error handling here, but this is probably not the right thing to do.
            // mark_federate_requesting_stop(my_fed);
            break;
        }
        LF_PRINT_DEBUG("RTI: Received message type %u from federate %d.", buffer[0], my_fed->id);
        switch(buffer[0]) {
            case MSG_TYPE_TIMESTAMP:
                handle_timestamp(my_fed);
                break;
            case MSG_TYPE_ADDRESS_QUERY:
                handle_address_query(my_fed->id);
                break;
            case MSG_TYPE_ADDRESS_ADVERTISEMENT:
                handle_address_ad(my_fed->id);
                break;
            case MSG_TYPE_TAGGED_MESSAGE:
                handle_timed_message(my_fed, buffer);
                break;
            case MSG_TYPE_RESIGN:
                handle_federate_resign(my_fed);
                return NULL;
                break;
            case MSG_TYPE_NEXT_EVENT_TAG:
                handle_next_event_tag(my_fed);
                break;
            case MSG_TYPE_LOGICAL_TAG_COMPLETE:
                handle_logical_tag_complete(my_fed);
                break;
            case MSG_TYPE_STOP_REQUEST:
                handle_stop_request_message(my_fed); // FIXME: Reviewed until here.
                                                     // Need to also look at
                                                     // send_advance_grant_if_safe()
                                                     // and send_downstream_advance_grants_if_safe()
                break;
            case MSG_TYPE_STOP_REQUEST_REPLY:
                handle_stop_request_reply(my_fed);
                break;
            case MSG_TYPE_PORT_ABSENT:
                handle_port_absent_message(my_fed, buffer);
                break;
            default:
                lf_print_error("RTI received from federate %d an unrecognized TCP message type: %u.", my_fed->id, buffer[0]);
                if (_RTI.tracing_enabled) {
                    tracepoint_RTI_from_federate(receive_UNIDENTIFIED, my_fed->id, NULL);
                }
        }
    }

    // Nothing more to do. Close the socket and exit.
    close(my_fed->socket); //  from unistd.h

    return NULL;
}

void send_reject(int socket_id, unsigned char error_code) {
    LF_PRINT_DEBUG("RTI sending MSG_TYPE_REJECT.");
    unsigned char response[2];
    response[0] = MSG_TYPE_REJECT;
    response[1] = error_code;
    // NOTE: Ignore errors on this response.
    write_to_socket_errexit(socket_id, 2, response, "RTI failed to write MSG_TYPE_REJECT message on the socket.");
    // Close the socket.
    close(socket_id);
}

int32_t receive_and_check_fed_id_message(int socket_id, struct sockaddr_in* client_fd) {
    // Buffer for message ID, federate ID, and federation ID length.
    size_t length = 1 + sizeof(uint16_t) + 1; // Message ID, federate ID, length of fedration ID.
    unsigned char buffer[length];

    // Read bytes from the socket. We need 4 bytes.
    // FIXME: This should not exit with error but rather should just reject the connection.
    read_from_socket_errexit(socket_id, length, buffer, "RTI failed to read from accepted socket.");

    uint16_t fed_id = _RTI.number_of_federates; // Initialize to an invalid value.

    // First byte received is the message type.
    if (buffer[0] != MSG_TYPE_FED_IDS) {
        if(buffer[0] == MSG_TYPE_P2P_SENDING_FED_ID || buffer[0] == MSG_TYPE_P2P_TAGGED_MESSAGE) {
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
        if (_RTI.tracing_enabled){
            tracepoint_RTI_to_federate(send_REJECT, fed_id, NULL);
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
        // FIXME: This should not exit on error, but rather just reject the connection.
        read_from_socket_errexit(socket_id, federation_id_length,
                            (unsigned char*)federation_id_received,
                            "RTI failed to read federation id from federate %d.", fed_id);

        // Terminate the string with a null.
        federation_id_received[federation_id_length] = 0;

        LF_PRINT_DEBUG("RTI received federation ID: %s.", federation_id_received);

        if (_RTI.tracing_enabled) {
            tracepoint_RTI_from_federate(receive_FED_ID, fed_id, NULL);
        }
        // Compare the received federation ID to mine.
        if (strncmp(_RTI.federation_id, federation_id_received, federation_id_length) != 0) {
            // Federation IDs do not match. Send back a MSG_TYPE_REJECT message.
            lf_print_error("WARNING: Federate from another federation %s attempted to connect to RTI in federation %s.\n",
                    federation_id_received,
                    _RTI.federation_id);
            if (_RTI.tracing_enabled) {
                    tracepoint_RTI_to_federate(send_REJECT, fed_id, NULL);
            }
            send_reject(socket_id, FEDERATION_ID_DOES_NOT_MATCH);
            return -1;
        } else {
            if (fed_id >= _RTI.number_of_federates) {
                // Federate ID is out of range.
                lf_print_error("RTI received federate ID %d, which is out of range.", fed_id);
                if (_RTI.tracing_enabled){
                    tracepoint_RTI_to_federate(send_REJECT, fed_id, NULL);
                }
                send_reject(socket_id, FEDERATE_ID_OUT_OF_RANGE);
                return -1;
            } else {
                if (_RTI.federates[fed_id].state != NOT_CONNECTED) {
                    lf_print_error("RTI received duplicate federate ID: %d.", fed_id);
                    if (_RTI.tracing_enabled) {
                        tracepoint_RTI_to_federate(send_REJECT, fed_id, NULL);
                    }
                    send_reject(socket_id, FEDERATE_ID_IN_USE);
                    return -1;
                }
            }
        }
    }

    // The MSG_TYPE_FED_IDS message has the right federation ID.
    // Assign the address information for federate.
    // The IP address is stored here as an in_addr struct (in .server_ip_addr) that can be useful
    // to create sockets and can be efficiently sent over the network.
    // First, convert the sockaddr structure into a sockaddr_in that contains an internet address.
    struct sockaddr_in* pV4_addr = client_fd;
    // Then extract the internet address (which is in IPv4 format) and assign it as the federate's socket server
    _RTI.federates[fed_id].server_ip_addr = pV4_addr->sin_addr;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    // Create the human readable format and copy that into
    // the .server_hostname field of the federate.
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &_RTI.federates[fed_id].server_ip_addr, str, INET_ADDRSTRLEN );
    strncpy (_RTI.federates[fed_id].server_hostname, str, INET_ADDRSTRLEN);

    LF_PRINT_DEBUG("RTI got address %s from federate %d.", _RTI.federates[fed_id].server_hostname, fed_id);
#endif
    _RTI.federates[fed_id].socket = socket_id;

    // Set the federate's state as pending
    // because it is waiting for the start time to be
    // sent by the RTI before beginning its execution.
    _RTI.federates[fed_id].state = PENDING;

    LF_PRINT_DEBUG("RTI responding with MSG_TYPE_ACK to federate %d.", fed_id);
    // Send an MSG_TYPE_ACK message.
    unsigned char ack_message = MSG_TYPE_ACK;
    if (_RTI.tracing_enabled) {
        tracepoint_RTI_to_federate(send_ACK, fed_id, NULL);
    }
    write_to_socket_errexit(socket_id, 1, &ack_message,
            "RTI failed to write MSG_TYPE_ACK message to federate %d.", fed_id);

    return (int32_t)fed_id;
}

int receive_connection_information(int socket_id, uint16_t fed_id) {
    LF_PRINT_DEBUG("RTI waiting for MSG_TYPE_NEIGHBOR_STRUCTURE from federate %d.", fed_id);
    unsigned char connection_info_header[MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE];
    read_from_socket_errexit(
        socket_id,
        MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE,
        connection_info_header,
        "RTI failed to read MSG_TYPE_NEIGHBOR_STRUCTURE message header from federate %d.",
        fed_id
    );

    if (connection_info_header[0] != MSG_TYPE_NEIGHBOR_STRUCTURE) {
        lf_print_error("RTI was expecting a MSG_TYPE_UDP_PORT message from federate %d. Got %u instead. "
                "Rejecting federate.", fed_id, connection_info_header[0]);
        send_reject(socket_id, UNEXPECTED_MESSAGE);
        return 0;
    } else {
        // Read the number of upstream and downstream connections
        _RTI.federates[fed_id].num_upstream = extract_int32(&(connection_info_header[1]));
        _RTI.federates[fed_id].num_downstream = extract_int32(&(connection_info_header[1 + sizeof(int32_t)]));
        LF_PRINT_DEBUG(
            "RTI got %d upstreams and %d downstreams from federate %d.",
            _RTI.federates[fed_id].num_upstream,
            _RTI.federates[fed_id].num_downstream,
            fed_id);

        // Allocate memory for the upstream and downstream pointers
        _RTI.federates[fed_id].upstream = (int*)malloc(sizeof(federate_t*) * _RTI.federates[fed_id].num_upstream);
        _RTI.federates[fed_id].downstream = (int*)malloc(sizeof(federate_t*) * _RTI.federates[fed_id].num_downstream);

        // Allocate memory for the upstream delay pointers
        _RTI.federates[fed_id].upstream_delay =
            (interval_t*)malloc(
                sizeof(interval_t) * _RTI.federates[fed_id].num_upstream
            );

        size_t connections_info_body_size = ((sizeof(uint16_t) + sizeof(int64_t)) *
            _RTI.federates[fed_id].num_upstream) + (sizeof(uint16_t) * _RTI.federates[fed_id].num_downstream);
        unsigned char* connections_info_body = (unsigned char*)malloc(connections_info_body_size);
        read_from_socket_errexit(
            socket_id,
            connections_info_body_size,
            connections_info_body,
            "RTI failed to read MSG_TYPE_NEIGHBOR_STRUCTURE message body from federate %d.",
            fed_id
        );

        // Keep track of where we are in the buffer
        size_t message_head = 0;
        // First, read the info about upstream federates
        for (int i=0; i<_RTI.federates[fed_id].num_upstream; i++) {
            _RTI.federates[fed_id].upstream[i] = extract_uint16(&(connections_info_body[message_head]));
            message_head += sizeof(uint16_t);
            _RTI.federates[fed_id].upstream_delay[i] = extract_int64(&(connections_info_body[message_head]));
            message_head += sizeof(int64_t);
        }

        // Next, read the info about downstream federates
        for (int i=0; i<_RTI.federates[fed_id].num_downstream; i++) {
            _RTI.federates[fed_id].downstream[i] = extract_uint16(&(connections_info_body[message_head]));
            message_head += sizeof(uint16_t);
        }

        free(connections_info_body);
        return 1;
    }
}

int receive_udp_message_and_set_up_clock_sync(int socket_id, uint16_t fed_id) {
    // Read the MSG_TYPE_UDP_PORT message from the federate regardless of the status of
    // clock synchronization. This message will tell the RTI whether the federate
    // is doing clock synchronization, and if it is, what port to use for UDP.
    LF_PRINT_DEBUG("RTI waiting for MSG_TYPE_UDP_PORT from federate %d.", fed_id);
    unsigned char response[1 + sizeof(uint16_t)];
    read_from_socket_errexit(socket_id, 1 + sizeof(uint16_t) , response,
            "RTI failed to read MSG_TYPE_UDP_PORT message from federate %d.", fed_id);
    if (response[0] != MSG_TYPE_UDP_PORT) {
        lf_print_error("RTI was expecting a MSG_TYPE_UDP_PORT message from federate %d. Got %u instead. "
                "Rejecting federate.", fed_id, response[0]);
        send_reject(socket_id, UNEXPECTED_MESSAGE);
        return 0;
    } else {
        if (_RTI.clock_sync_global_status >= clock_sync_init) {// If no initial clock sync, no need perform initial clock sync.
            uint16_t federate_UDP_port_number = extract_uint16(&(response[1]));

            LF_PRINT_DEBUG("RTI got MSG_TYPE_UDP_PORT %u from federate %d.", federate_UDP_port_number, fed_id);

            // A port number of UINT16_MAX means initial clock sync should not be performed.
            if (federate_UDP_port_number != UINT16_MAX) {
                // Perform the initialization clock synchronization with the federate.
                // Send the required number of messages for clock synchronization
                for (int i=0; i < _RTI.clock_sync_exchanges_per_interval; i++) {
                    // Send the RTI's current physical time T1 to the federate.
                    send_physical_clock(MSG_TYPE_CLOCK_SYNC_T1, &_RTI.federates[fed_id], TCP);

                    // Listen for reply message, which should be T3.
                    size_t message_size = 1 + sizeof(int32_t);
                    unsigned char buffer[message_size];
                    read_from_socket_errexit(socket_id, message_size, buffer,
                            "Socket to federate %d unexpectedly closed.", fed_id);
                    if (buffer[0] == MSG_TYPE_CLOCK_SYNC_T3) {
                        int32_t fed_id = extract_int32(&(buffer[1]));
                        assert(fed_id > -1);
                        assert(fed_id < 65536);
                        LF_PRINT_DEBUG("RTI received T3 clock sync message from federate %d.", fed_id);
                        handle_physical_clock_sync_message(&_RTI.federates[fed_id], TCP);
                    } else {
                        lf_print_error("Unexpected message %u from federate %d.", buffer[0], fed_id);
                        send_reject(socket_id, UNEXPECTED_MESSAGE);
                        return 0;
                    }
                }
                LF_PRINT_DEBUG("RTI finished initial clock synchronization with federate %d.", fed_id);
            }
            if (_RTI.clock_sync_global_status >= clock_sync_on) { // If no runtime clock sync, no need to set up the UDP port.
                    if (federate_UDP_port_number > 0) {
                        // Initialize the UDP_addr field of the federate struct
                        _RTI.federates[fed_id].UDP_addr.sin_family = AF_INET;
                        _RTI.federates[fed_id].UDP_addr.sin_port = htons(federate_UDP_port_number);
                        _RTI.federates[fed_id].UDP_addr.sin_addr = _RTI.federates[fed_id].server_ip_addr;
                    }
            } else {
                    // Disable clock sync after initial round.
                    _RTI.federates[fed_id].clock_synchronization_enabled = false;
            }
        } else { // No clock synchronization at all.
            // Clock synchronization is universally disabled via the clock-sync command-line parameter
            // (-c off was passed to the RTI).
            // Note that the federates are still going to send a MSG_TYPE_UDP_PORT message but with a payload (port) of -1.
            _RTI.federates[fed_id].clock_synchronization_enabled = false;
        }
    }
    return 1;
}

#ifdef __RTI_AUTH__
bool authenticate_federate(int socket) {
    // Buffer for message type and federation RTI nonce.
    size_t message_length = 1 + NONCE_LENGTH;
    unsigned char rti_hello_buffer[message_length];
    rti_hello_buffer[0] = MSG_TYPE_RTI_NONCE;
    unsigned char rti_nonce[NONCE_LENGTH];
    RAND_bytes(rti_nonce, NONCE_LENGTH);
    memcpy(rti_hello_buffer + 1, rti_nonce, NONCE_LENGTH);
    // Send RTI hello with RTI's random nonce.
    write_to_socket(socket, message_length, rti_hello_buffer);

    // Check HMAC of received FED_RESPONSE message.
    size_t hmac_length = SHA256_HMAC_LENGTH;
    size_t federation_id_length = strnlen(_RTI.federation_id, 255);
    size_t fed_id_length = sizeof(uint16_t);

    unsigned char received[1 + NONCE_LENGTH + fed_id_length + hmac_length];
    read_from_socket_errexit(socket, 1 + NONCE_LENGTH + fed_id_length + hmac_length, received, "Failed to read RTI response.");
    if (received[0] != MSG_TYPE_FED_RESPONSE) {
        lf_print_error("Received unexpected response %u from the FED (see net_common.h).",
                received[0]);
        return false;
    }

    // Create tag to compare to received tag.
    unsigned char buf_to_check[1 + fed_id_length + NONCE_LENGTH];
    buf_to_check[0] = MSG_TYPE_FED_RESPONSE;
    memcpy(&buf_to_check[1], &received[1 + NONCE_LENGTH], fed_id_length);
    memcpy(&buf_to_check[1 + fed_id_length], rti_nonce, NONCE_LENGTH);
    unsigned char rti_tag[hmac_length];
    HMAC(EVP_sha256(), _RTI.federation_id, federation_id_length, buf_to_check, 1 + fed_id_length + NONCE_LENGTH,
         rti_tag, &hmac_length);

    // Compare received tag and created tag.
    if (memcmp(&received[1 + fed_id_length + NONCE_LENGTH], rti_tag, hmac_length) != 0) {
        // Federation IDs do not match. Send back a HMAC_DOES_NOT_MATCH message.
        lf_print_warning("HMAC authentication failed. Rejecting the federate.");
        send_reject(socket, HMAC_DOES_NOT_MATCH);
        return false;
    }
    else{
        LF_PRINT_LOG("HMAC verified.");
        // HMAC tag is created with MSG_TYPE and received federate nonce.
        unsigned char mac_buf[1 + NONCE_LENGTH];
        mac_buf[0] = MSG_TYPE_RTI_RESPONSE;
        memcpy(&mac_buf[1], &received[1], NONCE_LENGTH);
        // Buffer for message type and HMAC tag.
        unsigned char sender[1 + hmac_length];
        sender[0] = MSG_TYPE_RTI_RESPONSE;
        HMAC(EVP_sha256(), _RTI.federation_id, federation_id_length, mac_buf, 1 + NONCE_LENGTH,
             &sender[1], &hmac_length);
        write_to_socket(socket, 1 + hmac_length, sender);
        return true;
    }
}
#endif

void connect_to_federates(int socket_descriptor) {
    for (int i = 0; i < _RTI.number_of_federates; i++) {
        // Wait for an incoming connection request.
        struct sockaddr client_fd;
        uint32_t client_length = sizeof(client_fd);
        // The following blocks until a federate connects.
        int socket_id = -1;
        while(1) {
            socket_id = accept(_RTI.socket_descriptor_TCP, &client_fd, &client_length);
            if (socket_id >= 0) {
                // Got a socket
                break;
            } else if (socket_id < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
                lf_print_error_and_exit("RTI failed to accept the socket. %s.", strerror(errno));
            } else {
                // Try again
                lf_print_warning("RTI failed to accept the socket. %s. Trying again.", strerror(errno));
                continue;
            }
        }

        // Send RTI hello when RTI -a option is on.
        #ifdef __RTI_AUTH__
        if (_RTI.authentication_enabled) {
            if (!authenticate_federate(socket_id)) {
                lf_print_warning("RTI failed to authenticate the incoming federate.");
                // Ignore the federate that failed authentication.
                i--;
                continue;
            }
        }
        #endif
        
        // The first message from the federate should contain its ID and the federation ID.
        int32_t fed_id = receive_and_check_fed_id_message(socket_id, (struct sockaddr_in*)&client_fd);
        if (fed_id >= 0
                && receive_connection_information(socket_id, (uint16_t)fed_id)
                && receive_udp_message_and_set_up_clock_sync(socket_id, (uint16_t)fed_id)) {

            // Create a thread to communicate with the federate.
            // This has to be done after clock synchronization is finished
            // or that thread may end up attempting to handle incoming clock
            // synchronization messages.
            pthread_create(&(_RTI.federates[fed_id].thread_id), NULL, federate_thread_TCP, &(_RTI.federates[fed_id]));

        } else {
            // Received message was rejected. Try again.
            i--;
        }
    }
    // All federates have connected.
    LF_PRINT_DEBUG("All federates have connected to RTI.");

    if (_RTI.clock_sync_global_status >= clock_sync_on) {
        // Create the thread that performs periodic PTP clock synchronization sessions
        // over the UDP channel, but only if the UDP channel is open and at least one
        // federate is performing runtime clock synchronization.
        bool clock_sync_enabled = false;
        for (int i = 0; i < _RTI.number_of_federates; i++) {
            if (_RTI.federates[i].clock_synchronization_enabled) {
                clock_sync_enabled = true;
                break;
            }
        }
        if (_RTI.final_port_UDP != UINT16_MAX && clock_sync_enabled) {
            pthread_create(&_RTI.clock_thread, NULL, clock_synchronization_thread, NULL);
        }
    }
}

void* respond_to_erroneous_connections(void* nothing) {
    while (true) {
        // Wait for an incoming connection request.
        struct sockaddr client_fd;
        uint32_t client_length = sizeof(client_fd);
        // The following will block until either a federate attempts to connect
        // or close(_RTI.socket_descriptor_TCP) is called.
        int socket_id = accept(_RTI.socket_descriptor_TCP, &client_fd, &client_length);
        if (socket_id < 0) return NULL;

        if (_RTI.all_federates_exited) {
            return NULL;
        }

        lf_print_error("RTI received an unexpected connection request. Federation is running.");
        unsigned char response[2];
        response[0] = MSG_TYPE_REJECT;
        response[1] = FEDERATION_ID_DOES_NOT_MATCH;
        // Ignore errors on this response.
        write_to_socket_errexit(socket_id, 2, response,
                 "RTI failed to write FEDERATION_ID_DOES_NOT_MATCH to erroneous incoming connection.");
        // Close the socket.
        close(socket_id);
    }
    return NULL;
}

void initialize_federate(uint16_t id) {
    _RTI.federates[id].id = id;
    _RTI.federates[id].socket = -1;      // No socket.
    _RTI.federates[id].clock_synchronization_enabled = true;
    _RTI.federates[id].completed = NEVER_TAG;
    _RTI.federates[id].last_granted = NEVER_TAG;
    _RTI.federates[id].last_provisionally_granted = NEVER_TAG;
    _RTI.federates[id].next_event = NEVER_TAG;
    _RTI.federates[id].in_transit_message_tags = initialize_in_transit_message_q();
    _RTI.federates[id].state = NOT_CONNECTED;
    _RTI.federates[id].upstream = NULL;
    _RTI.federates[id].upstream_delay = NULL;
    _RTI.federates[id].num_upstream = 0;
    _RTI.federates[id].downstream = NULL;
    _RTI.federates[id].num_downstream = 0;
    _RTI.federates[id].mode = REALTIME;
    strncpy(_RTI.federates[id].server_hostname ,"localhost", INET_ADDRSTRLEN);
    _RTI.federates[id].server_ip_addr.s_addr = 0;
    _RTI.federates[id].server_port = -1;
    _RTI.federates[id].requested_stop = false;
}

int32_t start_rti_server(uint16_t port) {
    int32_t specified_port = port;
    if (port == 0) {
        // Use the default starting port.
        port = STARTING_PORT;
    }
    lf_initialize_clock();
    // Create the TCP socket server
    _RTI.socket_descriptor_TCP = create_server(specified_port, port, TCP);
    lf_print("RTI: Listening for federates.");
    // Create the UDP socket server
    // Try to get the _RTI.final_port_TCP + 1 port
    if (_RTI.clock_sync_global_status >= clock_sync_on) {
        _RTI.socket_descriptor_UDP = create_server(specified_port, _RTI.final_port_TCP + 1, UDP);
    }
    return _RTI.socket_descriptor_TCP;
}

void wait_for_federates(int socket_descriptor) {
    // Wait for connections from federates and create a thread for each.
    connect_to_federates(socket_descriptor);

    // All federates have connected.
    lf_print("RTI: All expected federates have connected. Starting execution.");

    // The socket server will not continue to accept connections after all the federates
    // have joined.
    // In case some other federation's federates are trying to join the wrong
    // federation, need to respond. Start a separate thread to do that.
    pthread_t responder_thread;
    pthread_create(&responder_thread, NULL, respond_to_erroneous_connections, NULL);

    // Wait for federate threads to exit.
    void* thread_exit_status;
    for (int i = 0; i < _RTI.number_of_federates; i++) {
        lf_print("RTI: Waiting for thread handling federate %d.", _RTI.federates[i].id);
        pthread_join(_RTI.federates[i].thread_id, &thread_exit_status);
        free_in_transit_message_q(_RTI.federates[i].in_transit_message_tags);
        lf_print("RTI: Federate %d thread exited.", _RTI.federates[i].id);
    }

    _RTI.all_federates_exited = true;

    // Shutdown and close the socket so that the accept() call in
    // respond_to_erroneous_connections returns. That thread should then
    // check _RTI.all_federates_exited and it should exit.
    if (shutdown(socket_descriptor, SHUT_RDWR)) {
        LF_PRINT_LOG("On shut down TCP socket, received reply: %s", strerror(errno));
    }
    close(socket_descriptor);

    /************** FIXME: The following is probably not needed.
    The above shutdown and close should do the job.

    // NOTE: Apparently, closing the socket will not necessarily
    // cause the respond_to_erroneous_connections accept() call to return,
    // so instead, we connect here so that it can check the _RTI.all_federates_exited
    // variable.

    // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
    int tmp_socket = socket(AF_INET , SOCK_STREAM , 0);
    // If creating the socket fails, assume the thread has already exited.
    if (tmp_socket >= 0) {
        struct hostent *server = gethostbyname("localhost");
        if (server != NULL) {
            // Server file descriptor.
            struct sockaddr_in server_fd;
            // Zero out the server_fd struct.
            bzero((char *) &server_fd, sizeof(server_fd));
            // Set up the server_fd fields.
            server_fd.sin_family = AF_INET;    // IPv4
            bcopy((char *)server->h_addr,
                 (char *)&server_fd.sin_addr.s_addr,
                 server->h_length);
            // Convert the port number from host byte order to network byte order.
            server_fd.sin_port = htons(_RTI.final_port_TCP);
            connect(
                tmp_socket,
                (struct sockaddr *)&server_fd,
                sizeof(server_fd));
            close(tmp_socket);
        }
    }

    // NOTE: In all common TCP/IP stacks, there is a time period,
    // typically between 30 and 120 seconds, called the TIME_WAIT period,
    // before the port is released after this close. This is because
    // the OS is preventing another program from accidentally receiving
    // duplicated packets intended for this program.
    close(socket_descriptor);
    */

    if (_RTI.socket_descriptor_UDP > 0) {
        if (shutdown(_RTI.socket_descriptor_UDP, SHUT_RDWR)) {
            LF_PRINT_LOG("On shut down UDP socket, received reply: %s", strerror(errno));
        }
        close(_RTI.socket_descriptor_UDP);
    }
}

void usage(int argc, const char* argv[]) {
    printf("\nCommand-line arguments: \n\n");
    printf("  -i, --id <n>\n");
    printf("   The ID of the federation that this RTI will control.\n\n");
    printf("  -n, --number_of_federates <n>\n");
    printf("   The number of federates in the federation that this RTI will control.\n\n");
    printf("  -p, --port <n>\n");
    printf("   The port number to use for the RTI. Must be larger than 0 and smaller than %d. Default is %d.\n\n", UINT16_MAX, STARTING_PORT);
    printf("  -c, --clock_sync [off|init|on] [period <n>] [exchanges-per-interval <n>]\n");
    printf("   The status of clock synchronization for this federate.\n");
    printf("       - off: Clock synchronization is off.\n");
    printf("       - init (default): Clock synchronization is done only during startup.\n");
    printf("       - on: Clock synchronization is done both at startup and during the execution.\n");
    printf("   Relevant parameters that can be set: \n");
    printf("       - period <n>(in nanoseconds): Controls how often a clock synchronization attempt is made\n");
    printf("          (period in nanoseconds, default is 5 msec). Only applies to 'on'.\n");
    printf("       - exchanges-per-interval <n>: Controls the number of messages that are exchanged for each\n");
    printf("          clock sync attempt (default is 10). Applies to 'init' and 'on'.\n\n");
    printf("  -a, --auth Turn on HMAC authentication options.\n\n");
    printf("  -t, --tracing Turn on tracing.\n\n");

    printf("Command given:\n");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n\n");
}

int process_clock_sync_args(int argc, const char* argv[]) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "off") == 0) {
            _RTI.clock_sync_global_status = clock_sync_off;
            printf("RTI: Clock sync: off\n");
        } else if (strcmp(argv[i], "init") == 0 || strcmp(argv[i], "initial") == 0) {
            _RTI.clock_sync_global_status = clock_sync_init;
            printf("RTI: Clock sync: init\n");
        } else if (strcmp(argv[i], "on") == 0) {
            _RTI.clock_sync_global_status = clock_sync_on;
            printf("RTI: Clock sync: on\n");
        } else if (strcmp(argv[i], "period") == 0) {
            if (_RTI.clock_sync_global_status != clock_sync_on) {
                fprintf(stderr, "Error: clock sync period can only be set if --clock-sync is set to on.\n");
                usage(argc, argv);
                i++;
                continue; // Try to parse the rest of the arguments as clock sync args.
            } else if (argc < i + 2) {
                fprintf(stderr, "Error: clock sync period needs a time (in nanoseconds) argument.\n");
                usage(argc, argv);
                continue;
            }
            i++;
            long long period_ns = strtoll(argv[i], NULL, 10);
            if (period_ns == 0LL || period_ns == LLONG_MAX || period_ns == LLONG_MIN) {
                fprintf(stderr, "Error: clock sync period value is invalid.\n");
                continue; // Try to parse the rest of the arguments as clock sync args.
            }
            _RTI.clock_sync_period_ns = (int64_t)period_ns;
            printf("RTI: Clock sync period: %lld\n", (long long int)_RTI.clock_sync_period_ns);
        } else if (strcmp(argv[i], "exchanges-per-interval") == 0) {
            if (_RTI.clock_sync_global_status != clock_sync_on && _RTI.clock_sync_global_status != clock_sync_init) {
                fprintf(stderr, "Error: clock sync exchanges-per-interval can only be set if\n");
                fprintf(stderr, "--clock-sync is set to on or init.\n");
                usage(argc, argv);
                continue; // Try to parse the rest of the arguments as clock sync args.
            } else if (argc < i + 2) {
                fprintf(stderr, "Error: clock sync exchanges-per-interval needs an integer argument.\n");
                usage(argc, argv);
                continue; // Try to parse the rest of the arguments as clock sync args.
            }
            i++;
            long exchanges = (long)strtol(argv[i], NULL, 10);
            if (exchanges == 0L || exchanges == LONG_MAX ||  exchanges == LONG_MIN) {
                 fprintf(stderr, "Error: clock sync exchanges-per-interval value is invalid.\n");
                 continue; // Try to parse the rest of the arguments as clock sync args.
             }
            _RTI.clock_sync_exchanges_per_interval = (int32_t)exchanges; // FIXME: Loses numbers on 64-bit machines
            printf("RTI: Clock sync exchanges per interval: %d\n", _RTI.clock_sync_exchanges_per_interval);
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
                fprintf(stderr, "Error: --id needs a string argument.\n");
                usage(argc, argv);
                return 0;
            }
            i++;
            printf("RTI: Federation ID: %s\n", argv[i]);
            _RTI.federation_id = argv[i];
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--number_of_federates") == 0) {
            if (argc < i + 2) {
                fprintf(stderr, "Error: --number_of_federates needs an integer argument.\n");
                usage(argc, argv);
                return 0;
            }
            i++;
            long num_federates = strtol(argv[i], NULL, 10);
            if (num_federates == 0L || num_federates == LONG_MAX ||  num_federates == LONG_MIN) {
                fprintf(stderr, "Error: --number_of_federates needs a valid positive integer argument.\n");
                usage(argc, argv);
                return 0;
            }
            _RTI.number_of_federates = (int32_t)num_federates; // FIXME: Loses numbers on 64-bit machines
            printf("RTI: Number of federates: %d\n", _RTI.number_of_federates);
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (argc < i + 2) {
                fprintf(
                    stderr,
                    "Error: --port needs a short unsigned integer argument ( > 0 and < %d).\n",
                    UINT16_MAX
                );
                usage(argc, argv);
                return 0;
            }
            i++;
            uint32_t RTI_port = (uint32_t)strtoul(argv[i], NULL, 10);
            if (RTI_port <= 0 || RTI_port >= UINT16_MAX) {
                fprintf(
                    stderr,
                    "Error: --port needs a short unsigned integer argument ( > 0 and < %d).\n",
                    UINT16_MAX
                );
                usage(argc, argv);
                return 0;
            }
            _RTI.user_specified_port = (uint16_t)RTI_port;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--clock_sync") == 0) {
            if (argc < i + 2) {
               fprintf(stderr, "Error: --clock-sync needs off|init|on.\n");
               usage(argc, argv);
               return 0;
           }
           i++;
           i += process_clock_sync_args((argc-i), &argv[i]);
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--auth") == 0) {
            _RTI.authentication_enabled = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tracing") == 0) {
            _RTI.tracing_enabled = true;
        } else if (strcmp(argv[i], " ") == 0) {
            // Tolerate spaces
            continue;
        }  else {
           fprintf(stderr, "Error: Unrecognized command-line argument: %s\n", argv[i]);
           usage(argc, argv);
           return 0;
       }
    }
    if (_RTI.number_of_federates == 0) {
        fprintf(stderr, "Error: --number_of_federates needs a valid positive integer argument.\n");
        usage(argc, argv);
        return 0;
    }
    return 1;
}
