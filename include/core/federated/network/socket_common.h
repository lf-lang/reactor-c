#ifndef SOCKET_COMMON_H
#define SOCKET_COMMON_H

#include <netinet/in.h>  // IPPROTO_TCP, IPPROTO_UDP
#include <netinet/tcp.h> // TCP_NODELAY

#include "tag.h"

/**
 * The timeout time in ns for TCP operations.
 * Default value is 10 secs.
 */
#define TCP_TIMEOUT_TIME SEC(10)

/**
 * The timeout time in ns for UDP operations.
 * Default value is 1 sec.
 */
#define UDP_TIMEOUT_TIME SEC(1)

/**
 * Maximum number of port addresses that a federate will try to connect to the RTI on.
 * If you are using automatic ports begining at DEFAULT_PORT, this puts an upper bound
 * on the number of RTIs that can be running on the same host.
 */
#define MAX_NUM_PORT_ADDRESSES 16

/**
 * Time to wait before re-attempting to bind to a port.
 * When a process closes, the network stack typically waits between 30 and 120
 * seconds before releasing the port.  This is to allow for delayed packets so
 * that a new process does not receive packets from a previous process.
 * Here, we limit the retries to 60 seconds.
 */
#define PORT_BIND_RETRY_INTERVAL SEC(1)

/**
 * Number of attempts to bind to a port before giving up.
 */
#define PORT_BIND_RETRY_LIMIT 60

/**
 * Default port number for the RTI.
 * Unless a specific port has been specified by the LF program in the "at"
 * for the RTI or on the command line, when the RTI starts up, it will attempt
 * to open a socket server on this port.
 */
#define RTI_DEFAULT_PORT 15045u

#define RTI_DEFAULT_UDP_PORT 15061u

typedef struct socket_priv_t {
  int port; // my port number
  int socket_descriptor;
  uint16_t user_specified_port;

  // The connected other side's info.
  char server_hostname[INET_ADDRSTRLEN]; // Human-readable IP address and
  int32_t server_port;                   // port number of the socket server of the federate
                                         // if it has any incoming direct connections from other federates.
                                         // The port number will be -1 if there is no server or if the
                                         // RTI has not been informed of the port number.
  struct in_addr server_ip_addr;         // Information about the IP address of the socket
                                         // server of the federate.

  struct sockaddr_in UDP_addr; // The UDP address for the federate.
} socket_priv_t;

socket_priv_t* TCP_socket_priv_init();
void TCP_socket_close(socket_priv_t* priv);
void TCP_socket_open(socket_priv_t* priv);


int create_real_time_tcp_socket_errexit();
// TODO: Check if it's fine to just use int. It's just an enum. Can't use server_type_t because not including netdriver.h
int create_TCP_server(socket_priv_t* priv, int server_type, uint16_t port);

int connect_to_socket(int sock, char* hostname, int port, uint16_t user_specified_port);

#endif /* SOCKET_COMMON_H */