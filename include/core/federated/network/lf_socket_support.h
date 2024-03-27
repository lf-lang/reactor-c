#ifndef LF_SOCKET_SUPPORT_H
#define LF_SOCKET_SUPPORT_H

#include "socket_common.h"

char* get_host_name(netdrv_t* drv);
int32_t get_my_port(netdrv_t* drv);
int32_t get_port(netdrv_t* drv);
struct in_addr* get_ip_addr(netdrv_t* drv);

void set_host_name(netdrv_t* drv, const char* hostname);
void set_port(netdrv_t* drv, int port);
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr);

void netdrv_free(netdrv_t* drv);


int netdrv_connect(netdrv_t* drv);

/**
 * Without blocking, peek at the specified socket and, if there is
 * anything on the queue, put its first byte at the specified address and return 1.
 * If there is nothing on the queue, return 0, and if an error occurs,
 * return -1.
 * @param socket The socket ID.
 * @param result Pointer to where to put the first byte available on the socket.
 */
ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result);

#endif // LF_SOCKET_SUPPORT_H
