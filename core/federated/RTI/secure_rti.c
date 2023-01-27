/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni
 * @author Dongha Kim
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

#include "rti.c"
#include "secure_rti.h"
#include "../sst-c-api/c_api.h"

/**
 * Handle a port absent message being received from a federate via the RTI.
 *
 * This function assumes the caller does not hold the mutex.
 */
void handle_port_absent_message(federate_t* sending_federate, unsigned char* buffer) {
    size_t message_size = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int64_t) + sizeof(uint32_t);

    read_from_socket_errexit(sending_federate->socket, message_size, &(buffer[1]),
                            " RTI failed to read port absent message from federate %u.",
                            sending_federate->id);

    // Need to acquire the mutex lock to ensure that the thread handling
    // messages coming from the socket connected to the destination does not
    // issue a TAG before this message has been forwarded.
    pthread_mutex_lock(&_RTI.rti_mutex);

    uint16_t reactor_port_id = extract_uint16(&(buffer[1]));
    uint16_t federate_id = extract_uint16(&(buffer[1 + sizeof(uint16_t)]));

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
    write_to_socket_errexit(destination_socket, message_size + 1, buffer,
            "RTI failed to forward message to federate %d.", federate_id);

    pthread_mutex_unlock(&_RTI.rti_mutex);
}

/**
 * Handle a timed message being received from a federate by the RTI to relay to another federate.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param sending_federate The sending federate.
 * @param buffer The buffer to read into (the first byte is already there).
 */
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

    LF_PRINT_LOG("RTI received message from federate %d for federate %u port %u with intended tag (%ld, %u). Forwarding.",
            sending_federate->id, federate_id, reactor_port_id,
            intended_tag.time - lf_time_start(), intended_tag.microstep);

    read_from_socket_errexit(sending_federate->socket, bytes_to_read, &(buffer[header_size]),
                     "RTI failed to read timed message from federate %d.", federate_id);
    size_t bytes_read = bytes_to_read + header_size;
    // Following only works for string messages.
    // LF_PRINT_DEBUG("Message received by RTI: %s.", buffer + header_size);

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
        "RTI forwarding message to port %d of federate %d of length %d.",
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
            "RTI: Adding a message with tag (%ld, %u) to the list of in-transit messages for federate %d.",
            intended_tag.time - lf_time_start(),
            intended_tag.microstep,
            federate_id
        );
    } else {
        lf_print_error(
            "RTI: Federate %d has already completed tag (%ld, %u) "
            "but there is an in-transit message with tag (%ld, %u) from federate %d. "
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

/**
 * Handle a logical tag complete (LTC) message. @see
 * MSG_TYPE_LOGICAL_TAG_COMPLETE in rti.h.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate that has completed a logical tag.
 */
void handle_logical_tag_complete(federate_t* fed) {
    unsigned char buffer[sizeof(int64_t) + sizeof(uint32_t)];
    read_from_socket_errexit(fed->socket, sizeof(int64_t) + sizeof(uint32_t), buffer,
            "RTI failed to read the content of the logical tag complete from federate %d.", fed->id);

    // FIXME: Consolidate this message with NET to get NMR (Next Message Request).
    // Careful with handling startup and shutdown.
    pthread_mutex_lock(&_RTI.rti_mutex);

    fed->completed = extract_tag(buffer);

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

/**
 * Handle a next event tag (NET) message. @see MSG_TYPE_NEXT_EVENT_TAG in rti.h.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate sending a NET message.
 */
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
    LF_PRINT_LOG("RTI received from federate %d the Next Event Tag (NET) (%ld, %u).",
        fed->id, fed->next_event.time - start_time,
        fed->next_event.microstep);
    update_federate_next_event_tag_locked(
        fed->id,
        intended_tag
    );
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

/**
 * Handle a MSG_TYPE_STOP_REQUEST message.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate sending a MSG_TYPE_STOP_REQUEST message.
 */
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
            write_to_socket_errexit(_RTI.federates[i].socket, MSG_TYPE_STOP_REQUEST_LENGTH, stop_request_buffer,
                    "RTI failed to forward MSG_TYPE_STOP_REQUEST message to federate %d.", _RTI.federates[i].id);
        }
    }
    LF_PRINT_LOG("RTI forwarded to federates MSG_TYPE_STOP_REQUEST with tag (%lld, %u).",
                _RTI.max_stop_tag.time - start_time,
                _RTI.max_stop_tag.microstep);
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

/**
 * Handle a MSG_TYPE_STOP_REQUEST_REPLY message.
 *
 * This function assumes the caller does not hold the mutex.
 *
 * @param fed The federate replying the MSG_TYPE_STOP_REQUEST
 */
void handle_stop_request_reply(federate_t* fed) {
    size_t bytes_to_read = MSG_TYPE_STOP_REQUEST_REPLY_LENGTH - 1;
    unsigned char buffer_stop_time[bytes_to_read];
    read_from_socket_errexit(fed->socket, bytes_to_read, buffer_stop_time,
            "RTI failed to read the reply to MSG_TYPE_STOP_REQUEST message from federate %d.", fed->id);

    tag_t federate_stop_tag = extract_tag(buffer_stop_time);

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
void handle_address_query(uint16_t fed_id) {
    // Use buffer both for reading and constructing the reply.
    // The length is what is needed for the reply.
    unsigned char buffer[sizeof(int32_t)];
    ssize_t bytes_read = read_from_socket(_RTI.federates[fed_id].socket, sizeof(uint16_t), (unsigned char*)buffer);
    if (bytes_read == 0) {
        lf_print_error_and_exit("Failed to read address query.");
    }
    uint16_t remote_fed_id = extract_uint16(buffer);

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
     LF_PRINT_LOG("Received address advertisement from federate %d.", federate_id);
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

/**
 * A function to handle timestamp messages.
 * This function assumes the caller does not hold the mutex.
 */
void handle_timestamp(federate_t *my_fed) {
    unsigned char buffer[sizeof(int64_t)];
    // Read bytes from the socket. We need 8 bytes.
    ssize_t bytes_read = read_from_socket(my_fed->socket, sizeof(int64_t), (unsigned char*)&buffer);
    if (bytes_read < (ssize_t)sizeof(int64_t)) {
        lf_print_error("ERROR reading timestamp from federate %d.\n", my_fed->id);
    }

    int64_t timestamp = swap_bytes_if_big_endian_int64(*((int64_t *)(&buffer)));
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
    // FIXME: re-acquire the lock.
    pthread_cond_broadcast(&_RTI.sent_start_time);
    LF_PRINT_LOG("RTI sent start time %lld to federate %d.", start_time, my_fed->id);
    pthread_mutex_unlock(&_RTI.rti_mutex);
}

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
void handle_federate_resign(federate_t *my_fed) {
    // Nothing more to do. Close the socket and exit.
    pthread_mutex_lock(&_RTI.rti_mutex);
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

/**
 * Thread handling TCP communication with a federate.
 * @param fed A pointer to the federate's struct that has the
 *  socket descriptor for the federate.
 */
void* secure_federate_thread_TCP(void* secure_fed) {
    secure_fed_t* my_secure_fed = (secure_fed_t*)secure_fed;

    // Buffer for incoming messages.
    // This does not constrain the message size because messages
    // are forwarded piece by piece.
    unsigned char buffer[FED_COM_BUFFER_SIZE];

    // Listen for messages from the federate.
    while (my_secure_fed->fed->state != NOT_CONNECTED) {
        ssize_t sst_bytes_read = read_from_socket(my_secure_fed->fed->socket, FED_COM_BUFFER_SIZE, buffer); //TODO: input buffer size?
        unsigned char *decrypted_buf = return_decrypted_buf(buffer, sst_bytes_read, my_secure_fed->session_ctx);

        FILE * fileDescriptor = fmemopen(decrypted_buf, sizeof(decrypted_buf), "r");
        //  Change FILE pointer to file descriptor
        my_secure_fed->fed->socket = fileno(fileDescriptor);
        //TODO: Error handling is not applied. Need to change.

        // Read no more than one byte to get the message type.
        ssize_t bytes_read = read_from_socket(my_secure_fed->fed->socket, 1, buffer);
        if (bytes_read < 1) {
            // Socket is closed
            lf_print_warning("RTI: Socket to federate %d is closed. Exiting the thread.", my_secure_fed->fed->id);
            my_secure_fed->fed->state = NOT_CONNECTED;
            my_secure_fed->fed->socket = -1;
            // FIXME: We need better error handling here, but this is probably not the right thing to do.
            // mark_federate_requesting_stop(my_secure_fed->fed);
            break;
        }
        LF_PRINT_DEBUG("RTI: Received message type %u from federate %d.", buffer[0], my_secure_fed->fed->id);
        switch(buffer[0]) {
            case MSG_TYPE_TIMESTAMP:
                handle_timestamp(my_secure_fed->fed);
                break;
            case MSG_TYPE_ADDRESS_QUERY:
                handle_address_query(my_secure_fed->fed->id);
                break;
            case MSG_TYPE_ADDRESS_ADVERTISEMENT:
                handle_address_ad(my_secure_fed->fed->id);
                break;
            case MSG_TYPE_TAGGED_MESSAGE:
                handle_timed_message(my_secure_fed->fed, buffer);
                break;
            case MSG_TYPE_RESIGN:
                handle_federate_resign(my_secure_fed->fed);
                return NULL;
                break;
            case MSG_TYPE_NEXT_EVENT_TAG:
                handle_next_event_tag(my_secure_fed->fed);
                break;
            case MSG_TYPE_LOGICAL_TAG_COMPLETE:
                handle_logical_tag_complete(my_secure_fed->fed);
                break;
            case MSG_TYPE_STOP_REQUEST:
                handle_stop_request_message(my_secure_fed->fed); // FIXME: Reviewed until here.
                                                     // Need to also look at
                                                     // send_advance_grant_if_safe()
                                                     // and send_downstream_advance_grants_if_safe()
                break;
            case MSG_TYPE_STOP_REQUEST_REPLY:
                handle_stop_request_reply(my_secure_fed->fed);
                break;
            case MSG_TYPE_PORT_ABSENT:
                handle_port_absent_message(my_secure_fed->fed, buffer);
                break;
            default:
                lf_print_error("RTI received from federate %d an unrecognized TCP message type: %u.", my_secure_fed->fed->id, buffer[0]);
        }
    }

    // Nothing more to do. Close the socket and exit.
    close(my_secure_fed->fed->socket); //  from unistd.h

    return NULL;
}

/**
 * Wait for one incoming connection request from each federate,
 * and upon receiving it, create a thread to communicate with
 * that federate. Return when all federates have connected.
 * @param socket_descriptor The socket on which to accept connections.
 */
void secure_connect_to_federates(int socket_descriptor) {
    // Initialize SST setting read form sst_config.
    SST_ctx_t *ctx = init_SST(_RTI.sst_config_path);
    // Initialize an empty session key list.
    INIT_SESSION_KEY_LIST(s_key_list);
    secure_fed_t secure_fed[_RTI.number_of_federates];

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
        // The first message from the federate should contain its ID and the federation ID.
        int32_t fed_id = receive_and_check_fed_id_message(socket_id, (struct sockaddr_in*)&client_fd);
        if (fed_id >= 0
                && receive_connection_information(socket_id, (uint16_t)fed_id)
                && receive_udp_message_and_set_up_clock_sync(socket_id, (uint16_t)fed_id)) {
            // Wait for the federates get session keys from the Auth.
            // The RTI will get requests for communication from the federates by session key id.
            // Then RTI will request the corresponding session key to the Auth.
            SST_session_ctx_t *session_ctx = server_secure_comm_setup(ctx, socket_id, &s_key_list);
            secure_fed[i].session_ctx = session_ctx;
            secure_fed[i].fed = &_RTI.federates[fed_id]; //TODO: Right poiting? Need debug.
            // Create a thread to communicate with the federate.
            // This has to be done after clock synchronization is finished
            // or that thread may end up attempting to handle incoming clock
            // synchronization messages.
            pthread_create(&(_RTI.federates[fed_id].thread_id), NULL, &secure_federate_thread_TCP, (void *)&secure_fed[i]); //TODO: Need debug.

            //TODO: Erase below.
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

/**
 * Start the runtime infrastructure (RTI) interaction with the federates
 * and wait for the federates to exit.
 * @param socket_descriptor The socket descriptor returned by start_rti_server().
 */
void secure_wait_for_federates(int socket_descriptor) {
    // Wait for connections from federates and create a thread for each.
    secure_connect_to_federates(socket_descriptor);

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

int main(int argc, char* argv[]) {
    if (!process_args(argc, argv)) {
        // Processing command-line arguments failed.
        return -1;
    }
    printf("Starting RTI for %d federates in federation ID %s\n", _RTI.number_of_federates, _RTI.federation_id);
    assert(_RTI.number_of_federates < UINT16_MAX);
    _RTI.federates = (federate_t*)calloc(_RTI.number_of_federates, sizeof(federate_t));
    for (uint16_t i = 0; i < _RTI.number_of_federates; i++) {
        initialize_federate(i); //TODO: Need to add config path.
    }
    int socket_descriptor = start_rti_server(_RTI.user_specified_port);
    secure_wait_for_federates(socket_descriptor);
    printf("RTI is exiting.\n");
    return 0;
}
