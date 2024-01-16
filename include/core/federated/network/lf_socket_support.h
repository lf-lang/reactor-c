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

typedef struct socket_priv_t {
	int sock;
	int proto;
    int port;
    int user_specified_port;
} socket_priv_t;

netdrv_t * socket_init(int protocol);

#endif // LF_SOCKET_SUPPORT_H
