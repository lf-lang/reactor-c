#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"
#include "lf_socket_support.h"

static void handle_header_read(unsigned char* buffer, size_t* bytes_to_read, int* state);

netdrv_t* initialize_netdrv(int my_federate_id, const char* federation_id) {
  netdrv_t* drv = initialize_common_netdrv(my_federate_id, federation_id);

  // Initialize priv.
  socket_priv_t* priv = TCP_socket_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
}

void close_netdrv(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  TCP_socket_close(priv);
  free(priv);
  free(drv);
}

// This only creates TCP servers not UDP.
int create_listener(netdrv_t* drv, server_type_t server_type, uint16_t port) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  return create_TCP_server(priv, (int)server_type, port);
}

/**
 * 1. initializes other side's netdrv.
 * 2. Establishes communication session.

**/
netdrv_t* establish_communication_session(netdrv_t* listener_netdrv) {
  // -2 is for uninitialized value.
  netdrv_t* connector_nedrv = initialize_netdrv(-2, listener_netdrv->federation_id);
  socket_priv_t* listener_priv = (socket_priv_t*)listener_netdrv->priv;
  socket_priv_t* connector_priv = (socket_priv_t*)connector_nedrv->priv;
  // Wait for an incoming connection request.
  struct sockaddr client_fd;
  uint32_t client_length = sizeof(client_fd);
  // The following blocks until a client connects.
  while (1) {
    connector_priv->socket_descriptor = accept(listener_priv->socket_descriptor, &client_fd, &client_length);
    if (connector_priv->socket_descriptor >= 0) {
      // Got a socket
      break;
    } else if (connector_priv->socket_descriptor < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
      lf_print_error_and_exit("Failed to accept the socket. %s. connector_priv->socket_descriptor = %d",
                              strerror(errno), connector_priv->socket_descriptor);
    } else {
      // Try again
      lf_print_warning("Failed to accept the socket. %s. Trying again.", strerror(errno));
      continue;
    }
  }

  // TODO: DONGHA
  // Get the IP address of the other accepting client. This is used in two cases.
  // 1) Decentralized coordination - handle_address_query() - Sends the port number and address of the federate.
  // 2) Clock synchronization - send_physical_clock - Send through UDP.
  struct sockaddr_in* pV4_addr = (struct sockaddr_in*)&client_fd;
  connector_priv->server_ip_addr = pV4_addr->sin_addr;
  return connector_nedrv;
}

void create_connector(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  TCP_socket_open(priv);
}

int connect_to_netdrv(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  int ret =
      connect_to_socket(priv->socket_descriptor, priv->server_hostname, priv->server_port, priv->user_specified_port);
  return ret;
}

/**
 * Write the specified number of bytes to the specified socket from the
 * specified buffer. If an error occurs, return -1 and set errno to indicate
 * the cause of the error. If the write succeeds, return 0.
 * This function repeats the attempt until the specified number of bytes
 * have been written or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by
 * DELAY_BETWEEN_SOCKET_RETRIES.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return The number of bytes written.
 */

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  if (priv->socket_descriptor < 0) {
    // Socket is not open.
    errno = EBADF;
    return -1;
  }
  ssize_t bytes_written = 0;
  while (bytes_written < (ssize_t)num_bytes) {
    ssize_t more = write(priv->socket_descriptor, buffer + bytes_written, num_bytes - (size_t)bytes_written);
    if (more <= 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
      // The error codes EAGAIN or EWOULDBLOCK indicate
      // that we should try again (@see man errno).
      // The error code EINTR means the system call was interrupted before completing.
      LF_PRINT_DEBUG("Writing to socket was blocked. Will try again.");
      lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
      continue;
    } else if (more < 0) {
      // A more serious error occurred.
      return -1;
    }
    bytes_written += more;
  }
  return bytes_written;
}

/**
 * Read the specified number of bytes from the specified socket into the specified buffer.
 * If an error occurs during this reading, return -1 and set errno to indicate
 * the cause of the error. If the read succeeds in reading the specified number of bytes,
 * return 0. If an EOF occurs before reading the specified number of bytes, return 1.
 * This function repeats the read attempt until the specified number of bytes
 * have been read, an EOF is read, or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by DELAY_BETWEEN_SOCKET_RETRIES.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @return 0 for success, 1 for EOF, and -1 for an rerror.
 */
ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  if (drv == NULL) {
    lf_print_warning("Netdriver is closed, returning -1.");
    return -1;
  }
  socket_priv_t* priv = (socket_priv_t*)drv->priv;

  size_t bytes_to_read;        // The bytes to read in future.
  ssize_t bytes_read = 0;      // The bytes that was read by a single read() function.
  size_t total_bytes_read = 0; // The total bytes that have been read, and will be the return of the read_from drv.
  int retry_count;
  int state;
  // Check if socket_descriptor is open.
  if (priv->socket_descriptor < 0) {
    // Socket is not open.
    errno = EBADF;
    return -1;
  }
  // First, check if there are remaining bytes.
  // If there are remaining bytes, it reads as long as it can (buffer_length).
  // Then it becomes KEEP_READING state.
  if (drv->read_remaining_bytes > 0) {
    bytes_to_read = (drv->read_remaining_bytes > buffer_length) ? buffer_length : drv->read_remaining_bytes;
    state = KEEP_READING;
  } else {
    // If there are no left bytes to read, it reads the header byte.
    bytes_to_read = 1; // read header
    state = HEADER_READ;
  }

  for (;;) {
    retry_count = 0;
    while (bytes_to_read > 0 && bytes_to_read <= buffer_length) {
      bytes_read = read(priv->socket_descriptor, buffer + total_bytes_read, bytes_to_read);
      if (bytes_read < 0 &&                                              // If)  Error has occurred,
          retry_count++ < NUM_SOCKET_RETRIES &&                          // there are left retry counts,
          (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) { // and the error code was these three,
        // Print warning, sleep for a short time, and retry.
        lf_print_warning("Reading from socket failed. Will try again.");
        lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
        continue;
      } else if (bytes_read < 0) {
        return -1;
      } else if (bytes_read == 0) {
        // An error occurred without those three error codes.
        // https://stackoverflow.com/questions/42188128/does-reading-from-a-socket-wait-or-get-eof
        // bytes_read == 0 means disconnected.
        return 0;
      }
      bytes_to_read -= bytes_read;
      total_bytes_read += bytes_read;
    }

    switch (state) {
    case HEADER_READ:
      handle_header_read(buffer, &bytes_to_read, &state);
      break;

    case READ_MSG_TYPE_FED_IDS:;
      size_t federation_id_length = (size_t)buffer[1 + sizeof(uint16_t)];
      bytes_to_read = federation_id_length;
      state = FINISH_READ;
      break;
    case READ_MSG_TYPE_NEIGHBOR_STRUCTURE:;
      int num_upstream = extract_int32(buffer + 1);
      int num_downstream = extract_int32(buffer + 1 + sizeof(int32_t));
      bytes_to_read = ((sizeof(uint16_t) + sizeof(int64_t)) * num_upstream) + (sizeof(uint16_t) * num_downstream);
      state = FINISH_READ;
      break;
    case READ_MSG_TYPE_TAGGED_MESSAGE:;
      size_t length = (size_t)extract_uint32(buffer + 1 + sizeof(uint16_t) + sizeof(uint16_t));
      if (length > buffer_length - total_bytes_read) {
        bytes_to_read = buffer_length - total_bytes_read;
        drv->read_remaining_bytes = length - bytes_to_read;
      } else {
        bytes_to_read = length;
      }
      state = FINISH_READ;
      break;
    case KEEP_READING:
      drv->read_remaining_bytes -= total_bytes_read;
      return total_bytes_read;
    case FINISH_READ:
      return total_bytes_read;
    }
  }
}

char* get_host_name(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  return priv->server_hostname;
}
int32_t get_my_port(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  return priv->port;
}
int32_t get_port(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  return (priv == NULL) ? -1 : priv->server_port;
}
//
struct in_addr* get_ip_addr(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  return &priv->server_ip_addr;
}
void set_host_name(netdrv_t* drv, const char* hostname) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  memcpy(priv->server_hostname, hostname, INET_ADDRSTRLEN);
}
void set_port(netdrv_t* drv, int port) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  priv->server_port = port;
}
void set_specified_port(netdrv_t* drv, int port) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  priv->user_specified_port = port;
}

// Unused.
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  priv->server_ip_addr = ip_addr;
}

void set_clock_netdrv(netdrv_t* clock_drv, netdrv_t* rti_drv, uint16_t port_num) {
  socket_priv_t* priv_clock = (socket_priv_t*)(clock_drv);
  socket_priv_t* priv_rti = (socket_priv_t*)(rti_drv);
  priv_clock->UDP_addr.sin_family = AF_INET;
  priv_clock->UDP_addr.sin_port = htons(port_num);
  priv_clock->UDP_addr.sin_addr = priv_rti->server_ip_addr;
}

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  ssize_t bytes_read = recv(priv->socket_descriptor, result, 1, MSG_DONTWAIT | MSG_PEEK);
  if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    return 0;
  else
    return bytes_read;
}

void set_target_id(netdrv_t* drv, int federate_id) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  if (federate_id == 0) {
  } // JUST TO PASS COMPILER.
}

// ------------------Helper Functions------------------ //

static void handle_header_read(unsigned char* buffer, size_t* bytes_to_read, int* state) {
  switch (buffer[0]) {
  case MSG_TYPE_REJECT: // 1 +1
    *bytes_to_read = 1;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_ACK: // 1
    *bytes_to_read = 0;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_UDP_PORT: // 1 + sizeof(uint16_t) = 3
    *bytes_to_read = sizeof(uint16_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_FED_IDS: // 1 + sizeof(uint16_t) + 1 + federation_id
    *bytes_to_read = sizeof(uint16_t) + 1;
    *state = READ_MSG_TYPE_FED_IDS;
    break;
  case MSG_TYPE_FED_NONCE: // 1 + sizeof(uint16_t) + NONCE_LENGTH(8)
    *bytes_to_read = sizeof(uint16_t) + NONCE_LENGTH;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_RTI_RESPONSE: // 1 + NONCE_LENGTH(8) + SHA256_HMAC_LENGTH(32)
    *bytes_to_read = NONCE_LENGTH + SHA256_HMAC_LENGTH;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_FED_RESPONSE: // 1 + SHA256_HMAC_LENGTH(32)
    *bytes_to_read = SHA256_HMAC_LENGTH;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_TIMESTAMP: // 1+sizeof(int64_t)
    *bytes_to_read = sizeof(int64_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_RESIGN:
    *bytes_to_read = 0;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_TAGGED_MESSAGE:
    *bytes_to_read = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t) + sizeof(int64_t) + sizeof(uint32_t);
    *state = READ_MSG_TYPE_TAGGED_MESSAGE;
    break;
  case MSG_TYPE_NEXT_EVENT_TAG:
    *bytes_to_read = sizeof(int64_t) + sizeof(uint32_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_TAG_ADVANCE_GRANT:
    *bytes_to_read = sizeof(instant_t) + sizeof(microstep_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_PROVISIONAL_TAG_ADVANCE_GRANT:
    *bytes_to_read = sizeof(instant_t) + sizeof(microstep_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_LATEST_TAG_CONFIRMED:
    *bytes_to_read = sizeof(int64_t) + sizeof(uint32_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_STOP_REQUEST:
    *bytes_to_read = MSG_TYPE_STOP_REQUEST_LENGTH - 1;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_STOP_REQUEST_REPLY:
    *bytes_to_read = MSG_TYPE_STOP_REQUEST_REPLY_LENGTH - 1;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_STOP_GRANTED:
    *bytes_to_read = MSG_TYPE_STOP_GRANTED_LENGTH - 1;
    *state = FINISH_READ;
    break;
  case MSG_TYPE_ADDRESS_QUERY:
    *bytes_to_read = sizeof(uint16_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_ADDRESS_QUERY_REPLY:
    *bytes_to_read = sizeof(int32_t) + sizeof(struct in_addr);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_ADDRESS_ADVERTISEMENT:
    *bytes_to_read = sizeof(int32_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_P2P_SENDING_FED_ID: // 1 /////////TODO: CHECK!!!!!!!
    *bytes_to_read = sizeof(uint16_t) + 1;
    *state = READ_MSG_TYPE_FED_IDS;
    break;
  case MSG_TYPE_P2P_MESSAGE:
    *bytes_to_read = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t);
    *state = READ_MSG_TYPE_TAGGED_MESSAGE;
    break;
  case MSG_TYPE_P2P_TAGGED_MESSAGE:
    *bytes_to_read = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t) + sizeof(instant_t) + sizeof(microstep_t);
    *state = READ_MSG_TYPE_TAGGED_MESSAGE;
    break;
  case MSG_TYPE_CLOCK_SYNC_T1:
    *bytes_to_read = sizeof(instant_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_CLOCK_SYNC_T3:
    *bytes_to_read = sizeof(int32_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_CLOCK_SYNC_T4:
    *bytes_to_read = sizeof(instant_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_CLOCK_SYNC_CODED_PROBE:
    *bytes_to_read = sizeof(int64_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_PORT_ABSENT:
    *bytes_to_read = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int64_t) + sizeof(uint32_t);
    *state = FINISH_READ;
    break;
  case MSG_TYPE_NEIGHBOR_STRUCTURE:
    *bytes_to_read = MSG_TYPE_NEIGHBOR_STRUCTURE_HEADER_SIZE - 1;
    *state = READ_MSG_TYPE_NEIGHBOR_STRUCTURE;
    break;
  case MSG_TYPE_FAILED:
    *bytes_to_read = 0;
    *state = FINISH_READ;
    break;
  default:
    *bytes_to_read = 0;
    // Error handling?
    *state = FINISH_READ;
    lf_print_error_system_failure("Undefined message header. Terminating system.");
  }
}
