#ifndef LF_SOCKET_SUPPORT_H
#define LF_SOCKET_SUPPORT_H

#include "net_util.h"

typedef struct socket_priv_t {
	int sock;
	int proto;
} socket_priv_t;

net_drv_t * socket_create(int protocol);

#endif // LF_SOCKET_SUPPORT_H
