#ifndef LF_SOCKET_SUPPORT_H
#define LF_SOCKET_SUPPORT_H

#include <netinet/in.h>     // IPPROTO_TCP, IPPROTO_UDP 
#include <netinet/tcp.h>    // TCP_NODELAY 

#include "net_util.h"

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
#define DEFAULT_PORT 15045u

typedef struct socket_priv_t {
    int port;
    int socket_descriptor;
	int proto;
	uint16_t user_specified_port;

    char server_hostname[INET_ADDRSTRLEN]; // Human-readable IP address and
    int32_t server_port;    // port number of the socket server of the federate
                            // if it has any incoming direct connections from other federates.
                            // The port number will be -1 if there is no server or if the
                            // RTI has not been informed of the port number.
    struct in_addr server_ip_addr; // Information about the IP address of the socket
                                // server of the federate.
} socket_priv_t;

netdrv_t * netdrv_init();

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
void create_net_server(netdrv_t *drv, netdrv_type_t netdrv_type);
int create_real_time_tcp_socket_errexit();
void close_netdrvs(netdrv_t *rti_netdrv, netdrv_t *clock_netdrv);
netdrv_t *accept_connection(netdrv_t * rti_netdrv);

#endif // LF_SOCKET_SUPPORT_H
