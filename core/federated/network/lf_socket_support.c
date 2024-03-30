#include <arpa/inet.h> /* htons */
#include <errno.h>
#include <linux/if.h> /* IFNAMSIZ */
#include <netinet/ether.h>
#include <netinet/in.h>  // IPPROTO_TCP, IPPROTO_UDP
#include <netinet/tcp.h> // TCP_NODELAY
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"

// static void socket_close(netdrv_t* drv) {
//   socket_priv_t* priv = (socket_priv_t*)drv->priv;
//   if (priv->socket_descriptor > 0) {
//     shutdown(priv->socket_descriptor, SHUT_RDWR);
//     close(priv->socket_descriptor);
//     priv->socket_descriptor = -1;
//   }
// }
// static void socket_open(netdrv_t* drv) {
//   socket_priv_t* priv = (socket_priv_t*)drv->priv;
//   priv->socket_descriptor = create_real_time_tcp_socket_errexit();
// }

// static void socket_open(netdrv_t* drv);
// static void socket_close(netdrv_t* drv);

void TCP_open(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  TCP_socket_open(priv);
}

void TCP_close(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  TCP_socket_close(priv);
}

netdrv_t* netdrv_init() {
  netdrv_t* drv = malloc(sizeof(netdrv_t));
  if (!drv) {
    lf_print_error_and_exit("Falied to malloc netdrv_t.");
  }
  memset(drv, 0, sizeof(netdrv_t));
  drv->open = TCP_open;
  drv->close = TCP_close;
  // drv->read = socket_read;
  // drv->write = socket_write;
  drv->read_remaining_bytes = 0;

  // Initialize priv.
  socket_priv_t* priv = TCP_socket_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
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

void netdrv_free(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  // free(priv); // Already freed on socket close()
  free(drv);
}

// This only creates TCP servers not UDP.
int create_server(netdrv_t* drv, int server_type, uint16_t port) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  return create_TCP_server(priv, server_type, port);
}

/**
 * 1. initializes other side's netdrv.
 * 2. Establishes communication session.

**/
netdrv_t* establish_communication_session(netdrv_t* my_netdrv) {
  netdrv_t* ret_netdrv = netdrv_init();
  socket_priv_t* my_priv = (socket_priv_t*)my_netdrv->priv;
  socket_priv_t* ret_priv = (socket_priv_t*)ret_netdrv->priv;
  // Wait for an incoming connection request.
  struct sockaddr client_fd;
  uint32_t client_length = sizeof(client_fd);
  // The following blocks until a client connects.
  while (1) {
    ret_priv->socket_descriptor = accept(my_priv->socket_descriptor, &client_fd, &client_length);
    if (ret_priv->socket_descriptor >= 0) {
      // Got a socket
      break;
    } else if (ret_priv->socket_descriptor < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
      lf_print_error_and_exit("Failed to accept the socket. %s. ret_priv->socket_descriptor = %d", strerror(errno),
                              ret_priv->socket_descriptor);
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
  ret_priv->server_ip_addr = pV4_addr->sin_addr;
  return ret_netdrv;
}

int netdrv_connect(netdrv_t* drv) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;

  struct addrinfo hints;
  struct addrinfo* result;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       /* Allow IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* Stream socket */
  hints.ai_protocol = IPPROTO_TCP; /* TCP protocol */
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_flags = AI_NUMERICSERV; /* Allow only numeric port numbers */

  // Convert port number to string.
  char str[6];
  sprintf(str, "%u", priv->server_port);

  // Get address structure matching hostname and hints criteria, and
  // set port to the port number provided in str. There should only
  // ever be one matching address structure, and we connect to that.
  if (getaddrinfo(priv->server_hostname, (const char*)&str, &hints, &result)) {
    lf_print_error_and_exit("No host matching given hostname: %s", priv->server_hostname);
  }

  int ret = connect(priv->socket_descriptor, result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);
  return ret;
}

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  ssize_t bytes_read = recv(priv->socket_descriptor, result, 1, MSG_DONTWAIT | MSG_PEEK);
  if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    return 0;
  else
    return bytes_read;
}

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  if (priv->socket_descriptor < 0) {
    // Socket is not open.
    errno = EBADF;
    return -1;
  }
  ssize_t bytes_written = 0;
  va_list args;
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

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  int bytes_written = write_to_netdrv(drv, num_bytes, buffer);
  if (bytes_written <= 0) {
    // Write failed.
    // Netdrv has probably been closed from the other side.
    // Shut down and close the netdrv from this side.
    drv->close(drv);
  }
  return bytes_written;
}

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {
  va_list args;
  int bytes_written = write_to_netdrv_close_on_error(drv, num_bytes, buffer);
  if (bytes_written <= 0) {
    // Write failed.
    if (mutex != NULL) {
      lf_mutex_unlock(mutex);
    }
    if (format != NULL) {
      lf_print_error_system_failure(format, args);
    } else {
      lf_print_error("Failed to write to socket. Closing it.");
    }
  }
}
// TODO: Fix return.
ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  ssize_t bytes_read = read_from_netdrv(drv, buffer, buffer_length);
  if (bytes_read <= 0) {
    drv->close(drv);
    return -1;
  }
  return bytes_read;
}

// TODO: FIX return
void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {
  va_list args;
  ssize_t bytes_read = read_from_netdrv_close_on_error(drv, buffer, buffer_length);
  if (bytes_read <= 0) {
    // Read failed.
    if (mutex != NULL) {
      lf_mutex_unlock(mutex);
    }
    if (format != NULL) {
      lf_print_error_system_failure(format, args);
    } else {
      lf_print_error_system_failure("Failed to read from netdrv.");
    }
  }
}

typedef enum {
  HEADER_READ,
  READ_MSG_TYPE_FED_IDS,
  READ_MSG_TYPE_NEIGHBOR_STRUCTURE,
  READ_MSG_TYPE_TAGGED_MESSAGE,
  KEEP_READING,
  FINISH_READ
} read_state_t;

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
  case MSG_TYPE_LATEST_TAG_COMPLETE:
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
  default:
    *bytes_to_read = 0;
    // Error handling?
    *state = FINISH_READ;
    lf_print_error_system_failure("Undefined message header. Terminating system.");
  }
}

// TODO: DONGHA: ADD buffer_length checking.
//  Returns the total bytes read.
ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
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
    while (bytes_to_read > 0) {
      // TODO: Check buffer_length.
      bytes_read = read(priv->socket_descriptor, buffer + total_bytes_read, bytes_to_read);
      if (bytes_read < 0 &&                                              // If)  Error has occurred,
          retry_count++ < NUM_SOCKET_RETRIES &&                          // there are left retry counts,
          (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) { // and the error code was these three,
        // Print warning, sleep for a short time, and retry.
        lf_print_warning("Reading from socket failed. Will try again.");
        lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
        continue;
      } else if (bytes_read <= 0) {
        // An error occurred without those three error codes.
        // https://stackoverflow.com/questions/42188128/does-reading-from-a-socket-wait-or-get-eof
        // bytes_read == 0 means disconnected.
        return -1;
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
