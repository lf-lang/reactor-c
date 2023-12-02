
#include <netinet/ether.h>
#include <linux/if.h>		/* IFNAMSIZ */
#include <arpa/inet.h>		/* htons */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#include "util.h"
#include "lf_socket_support.h"
// #include "net_util.h"

static socket_priv_t *get_priv(net_drv_t *drv)
{
	if (!drv)
		return NULL;
	return (socket_priv_t *)(drv+1);
}

// static int socket_init(net_drv_t *drv)
// {
// 	socket_priv_t *priv = get_priv(drv);
// 	priv->sock = socket(AF_PACKET, SOCK_RAW, htons(priv->proto));
// 	if (priv->sock < 0)
// 		return -1;

// 	/* If Rx, timeout is a good thing */
// 	if (priv->timeout_us > 0) {
// 		struct timeval tv = {
// 			.tv_sec = priv->timeout_us / 1e6,
// 			.tv_usec = priv->timeout_us % 1000000,
// 		};
// 		if (setsockopt(priv->sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == -1) {
// 			printf("%s(): Could not set timeout on socket (%d): %s\n",
// 				__func__, priv->sock, strerror(errno));
// 			close(priv->sock);
// 			return -1;
// 		}
// 	}
// 	/* bind to device, ... */

// 	printf("Socket created\n");
// 	return 0;
// }

// static void socket_close(net_drv_t *drv)
// {
// 	if (!drv)
// 		return;
// 	socket_priv_t *priv = get_priv(drv);
// 	if (priv->sock > 0) {
// 		close(priv->sock);
// 		priv->sock = -1;
// 	}
// }

// static int socket_recv(net_drv_t *drv, void * buffer, int size)
// {
// 	if (!drv)
// 		return -1;
// 	socket_priv_t *priv = get_priv(drv);
// 	if (priv->timeout_us > 0) {
// 		int res = -1;
// 		do {
// 			res = recv(priv->sock, buffer, size, MSG_TRUNC);
// 		} while (res > 0);
// 		return res;
// 	}
// 	return recv(priv->sock, buffer, size, MSG_TRUNC);
// }

// static int socket_send(net_drv_t *drv, void * buffer, int size)
// {
// 	if (!drv)
// 		return -1;
// 	socket_priv_t *priv = get_priv(drv);
// 	return send(priv->sock, buffer, size, MSG_DONTWAIT);
// }

net_drv_t * socket_create(int protocol) {
    //TODO: Should it be malloc? To support different network stacks operate simulatneously?
	net_drv_t *drv = malloc(sizeof(*drv) + sizeof(socket_priv_t)); //Don't need to call malloc() twice.
	if (!drv) {// check if malloc worked.
		lf_print_error_and_exit("Falied to malloc net_drv_t.");
    }
	memset(drv, 0, sizeof(*drv));

	socket_priv_t *priv = get_priv(drv); //drv+1 return.
	priv->proto = protocol;

	// drv->init = socket_init;
	// drv->close = socket_close;
	// drv->read = socket_recv;
	// drv->write = socket_send;
	return drv;
}
