#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"
#include "lf_sst_support.h"

const char* sst_config_path;
const char* RTI_config_path;

static sst_priv_t* sst_priv_init();
static void var_length_int_to_num(unsigned char* buf, unsigned int buf_length, unsigned int* num,
                                  unsigned int* var_len_int_buf_size);

netdrv_t* initialize_netdrv(int federate_id, const char* federation_id) {
  netdrv_t* drv = malloc(sizeof(netdrv_t));
  if (!drv) {
    lf_print_error_and_exit("Falied to malloc netdrv_t.");
  }
  memset(drv, 0, sizeof(netdrv_t));
  drv->read_remaining_bytes = 0;

  // Initialize priv.
  sst_priv_t* sst_priv = sst_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)sst_priv;
  return drv;
}
void close_netdrv(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  if (sst_priv->socket_priv != NULL) {
    TCP_socket_close(sst_priv->socket_priv);
  } else {
    lf_print_error("Trying to close TCP socket not existing.");
  }
}

// Port will be NULL on MQTT.
int create_server(netdrv_t* drv, int server_type, uint16_t port) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  SST_ctx_t* ctx = init_SST(RTI_config_path);
  sst_priv->sst_ctx = ctx;
  return create_TCP_server(sst_priv->socket_priv, server_type, port);
}

netdrv_t* establish_communication_session(netdrv_t* my_netdrv) {
  netdrv_t* ret_netdrv = initialize_netdrv(-2, my_netdrv->federation_id);
  sst_priv_t* my_priv = (sst_priv_t*)my_netdrv->priv;
  sst_priv_t* ret_priv = (sst_priv_t*)ret_netdrv->priv;

  // Wait for an incoming connection request.
  struct sockaddr client_fd;
  uint32_t client_length = sizeof(client_fd);
  // The following blocks until a client connects.
  while (1) {
    ret_priv->socket_priv->socket_descriptor =
        accept(my_priv->socket_priv->socket_descriptor, &client_fd, &client_length);
    if (ret_priv->socket_priv->socket_descriptor >= 0) {
      // Got a socket
      break;
    } else if (ret_priv->socket_priv->socket_descriptor < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
      lf_print_error_and_exit("Failed to accept the socket. %s. ret_priv->socket_priv->socket_descriptor = %d",
                              strerror(errno), ret_priv->socket_priv->socket_descriptor);
    } else {
      // Try again
      lf_print_warning("Failed to accept the socket. %s. Trying again.", strerror(errno));
      continue;
    }
  }

  session_key_list_t* s_key_list = init_empty_session_key_list();
  SST_session_ctx_t* session_ctx =
      server_secure_comm_setup(my_priv->sst_ctx, ret_priv->socket_priv->socket_descriptor, s_key_list);
  free_session_key_list_t(s_key_list);
  ret_priv->session_ctx = session_ctx;

  // TODO: DONGHA
  // Get the IP address of the other accepting client. This is used in two cases.
  // 1) Decentralized coordination - handle_address_query() - Sends the port number and address of the federate.
  // 2) Clock synchronization - send_physical_clock - Send through UDP.
  struct sockaddr_in* pV4_addr = (struct sockaddr_in*)&client_fd;
  ret_priv->socket_priv->server_ip_addr = pV4_addr->sin_addr;
  return ret_netdrv;
}

void create_client(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  SST_ctx_t* ctx = init_SST((const char*)sst_config_path);

  sst_priv->sst_ctx = ctx;
}

int connect_to_netdrv(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  session_key_list_t* s_key_list = get_session_key(sst_priv->sst_ctx, NULL);
  // Does not increases RTI port number.
  SST_session_ctx_t* session_ctx = secure_connect_to_server(&s_key_list->s_key[0], sst_priv->sst_ctx);
  sst_priv->session_ctx = session_ctx;
  return 1;
}

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  if (buffer[0] == MSG_TYPE_FAILED) {
    // Just return.
    return 0;
  }
  return send_secure_message((char*)buffer, num_bytes, sst_priv->session_ctx);
}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  unsigned char sst_buffer[1024];
  ssize_t bytes_read = 0;
  unsigned int temp_length = 10;

  if (sst_priv->session_ctx->sock < 0) {
    return -1;
  }

  // Read 10 bytes first.
  bytes_read = read(sst_priv->session_ctx->sock, sst_buffer, temp_length);
  if (bytes_read == 0) {
    // Connection closed.
    return 0;
  } else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    // Add retry.
  }
  unsigned int payload_length; // Length of the payload of SST.
  unsigned int var_length_buf_size;
  // This fills payload_length and var_length_buf_size.
  var_length_int_to_num(sst_buffer + sizeof(unsigned char), bytes_read, &payload_length, &var_length_buf_size);
  unsigned int bytes_to_read = payload_length - (temp_length - (sizeof(unsigned char) + var_length_buf_size));

  unsigned int second_read = 0;
  unsigned int more = 0;
  while (second_read != bytes_to_read) {
    more = read(sst_priv->session_ctx->sock, sst_buffer + temp_length, bytes_to_read);
    second_read += more;
    bytes_read += second_read;
  }

  unsigned int decrypted_buffer_length;
  unsigned char* decrypted_buffer =
      return_decrypted_buf(sst_buffer, bytes_read, &decrypted_buffer_length, sst_priv->session_ctx);
  // Returned SEQ_NUM_BUFFER(8) + decrypted_buffer;
  // Doing this because it should be freed.
  memcpy(buffer, decrypted_buffer + 8, decrypted_buffer_length - 8);
  free(decrypted_buffer);
  return decrypted_buffer_length;
}

// void netdrv_free(netdrv_t* drv) {
//   sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
//   // free(priv); // Already freed on socket close()
//   free_SST_ctx_t(sst_priv->sst_ctx);
//   free(drv);
// }

char* get_host_name(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return sst_priv->socket_priv->server_hostname;
}
int32_t get_my_port(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return sst_priv->socket_priv->port;
}
int32_t get_port(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return (sst_priv->socket_priv == NULL) ? -1 : sst_priv->socket_priv->server_port;
}
//
struct in_addr* get_ip_addr(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return &sst_priv->socket_priv->server_ip_addr;
}
void set_host_name(netdrv_t* drv, const char* hostname) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  memcpy(sst_priv->socket_priv->server_hostname, hostname, INET_ADDRSTRLEN);
}
void set_port(netdrv_t* drv, int port) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  sst_priv->socket_priv->server_port = port;
}

void set_specified_port(netdrv_t* drv, int port) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  sst_priv->socket_priv->user_specified_port = port;
}

// Unused.
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  sst_priv->socket_priv->server_ip_addr = ip_addr;
}

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {}

// ------------------Helper Functions------------------ //

void lf_set_sst_config_path(const char* config_path) { sst_config_path = config_path; }
void lf_set_rti_sst_config_path(const char* config_path) { RTI_config_path = config_path; }

static sst_priv_t* sst_priv_init() {
  sst_priv_t* sst_priv = malloc(sizeof(sst_priv_t));
  if (!sst_priv) {
    lf_print_error_and_exit("Falied to malloc sst_priv_t.");
  }
  memset(sst_priv, 0, sizeof(sst_priv_t));
  sst_priv->socket_priv = TCP_socket_priv_init();
  return sst_priv;
}

static void var_length_int_to_num(unsigned char* buf, unsigned int buf_length, unsigned int* num,
                                  unsigned int* var_len_int_buf_size) {
  *num = 0;
  *var_len_int_buf_size = 0;
  for (int i = 0; i < buf_length; i++) {
    *num |= (buf[i] & 127) << (7 * i);
    if ((buf[i] & 128) == 0) {
      *var_len_int_buf_size = i + 1;
      break;
    }
  }
}
