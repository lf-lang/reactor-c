#ifndef LF_SOCKET_SUPPORT_H
#define LF_SOCKET_SUPPORT_H

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
 * Default starting port number for the RTI and federates' socket server.
 * Unless a specific port has been specified by the LF program in the "at"
 * for the RTI, when the federates start up, they will attempt
 * to open a socket server
 * on this port, and, if this fails, increment the port number and
 * try again. The number of increments is limited by PORT_RANGE_LIMIT.
 * FIXME: Clarify what happens if a specific port has been given in "at".
 */
#define STARTING_PORT 15045u

/**
 * Number of ports to try to connect to. Unless the LF program specifies
 * a specific port number to use, the RTI or federates will attempt to start
 * a socket server on port STARTING_PORT. If that port is not available (e.g.,
 * another RTI is running or has recently exited), then it will try the
 * next port, STARTING_PORT+1, and keep incrementing the port number up to this
 * limit. If no port between STARTING_PORT and STARTING_PORT + PORT_RANGE_LIMIT
 * is available, then the RTI or the federate will fail to start. This number, therefore,
 * limits the number of RTIs and federates that can be simultaneously
 * running on any given machine without assigning specific port numbers.
 */
#define PORT_RANGE_LIMIT 1024

typedef struct socket_priv_t {
    int port;
    int socket_descriptor;
	int proto;
	uint16_t user_specified_port;
} socket_priv_t;

netdrv_t * netdrv_init();

#endif // LF_SOCKET_SUPPORT_H
