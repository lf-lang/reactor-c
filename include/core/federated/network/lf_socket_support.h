#ifndef LF_SOCKET_SUPPORT_H
#define LF_SOCKET_SUPPORT_H

#include "net_util.h"

typedef struct socket_priv_t {
	int sock;
	int proto;
    int port;
    int user_specified_port;
} socket_priv_t;

netdrv_t * socket_init(int protocol);

#endif // LF_SOCKET_SUPPORT_H
