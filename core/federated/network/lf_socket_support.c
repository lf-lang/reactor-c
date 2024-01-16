
#include "lf_socket_support.h"

#include <arpa/inet.h> /* htons */
#include <errno.h>
#include <linux/if.h> /* IFNAMSIZ */
#include <netinet/ether.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "util.h"
// #include "net_util.h"

static socket_priv_t *get_priv(netdrv_t *drv) {
    if (!drv) {
        lf_print_error_and_exit("Falied to malloc netdrv_t.");
        return NULL;
    }
    return (socket_priv_t *)(drv + 1);
}

static int socket_open(netdrv_t *drv) {
    // socket_priv_t *priv = get_priv(drv);
    // priv->sock = socket(AF_PACKET, SOCK_RAW, htons(priv->proto));
    // if (priv->sock < 0)
    //     return -1;

    // /* If Rx, timeout is a good thing */
    // if (priv->timeout_us > 0) {
    //     struct timeval tv = {
    //         .tv_sec = priv->timeout_us / 1e6,
    //         .tv_usec = priv->timeout_us % 1000000,
    //     };
    //     if (setsockopt(priv->sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) == -1) {
    //         printf("%s(): Could not set timeout on socket (%d): %s\n",
    //                __func__, priv->sock, strerror(errno));
    //         close(priv->sock);
    //         return -1;
    //     }
    // }
    // /* bind to device, ... */

    // printf("Socket created\n");
    // return 0;
}

// static void socket_close(netdrv_t *drv) {
//     if (!drv)
//         return;
//     socket_priv_t *priv = get_priv(drv);
//     if (priv->sock > 0) {
//         close(priv->sock);
//         priv->sock = -1;
//     }
// }

// static int socket_recv(netdrv_t *drv, void *buffer, int size) {
//     if (!drv)
//         return -1;
//     socket_priv_t *priv = get_priv(drv);
//     if (priv->timeout_us > 0) {
//         int res = -1;
//         do {
//             res = recv(priv->sock, buffer, size, MSG_TRUNC);
//         } while (res > 0);
//         return res;
//     }
//     return recv(priv->sock, buffer, size, MSG_TRUNC);
// }

// static int socket_send(netdrv_t *drv, void *buffer, int size) {
//     if (!drv)
//         return -1;
//     socket_priv_t *priv = get_priv(drv);
//     return send(priv->sock, buffer, size, MSG_DONTWAIT);
// }

netdrv_t *socket_init(int protocol) {
    // TODO: Should it be malloc? To support different network stacks operate simulatneously?
    netdrv_t *drv = malloc(sizeof(*drv) + sizeof(socket_priv_t));  // Don't need to call malloc() twice.
    if (!drv) {                                                    // check if malloc worked.
        lf_print_error_and_exit("Falied to malloc netdrv_t.");
    }
    memset(drv, 0, sizeof(netdrv_t));

    socket_priv_t *priv = get_priv(drv);  // drv+1 return.
    priv->proto = protocol;

    drv->open = socket_open;
    // drv->close = socket_close;
    // drv->read = socket_recv;
    // drv->write = socket_send;
    return drv;
}

void create_net_server(netdrv_t *drv){
    // Timeout time for the communications of the server
    struct timeval timeout_time = {.tv_sec = TCP_TIMEOUT_TIME / BILLION, .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000};
    // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
    int socket_descriptor = -1;
    if (socket_type == TCP) {
        socket_descriptor = create_real_time_tcp_socket_errexit();
    } else if (socket_type == UDP) {
        socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // Set the appropriate timeout time
        timeout_time = (struct timeval){.tv_sec = UDP_TIMEOUT_TIME / BILLION, .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000};
    }
    if (socket_descriptor < 0) {
        lf_print_error_and_exit("Failed to create RTI socket.");
    }

    // Set the option for this socket to reuse the same address
    int true_variable = 1; // setsockopt() requires a reference to the value assigned to an option
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &true_variable, sizeof(int32_t)) < 0) {
        lf_print_error("RTI failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
    }
    // Set the timeout on the socket so that read and write operations don't block for too long
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
        lf_print_error("RTI failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
    }
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
        lf_print_error("RTI failed to set SO_SNDTIMEO option on the socket: %s.", strerror(errno));
    }

    /*
     * The following used to permit reuse of a port that an RTI has previously
     * used that has not been released. We no longer do this, but instead
     * increment the port number until an available port is found.

    // SO_REUSEPORT (since Linux 3.9)
    //       Permits multiple AF_INET or AF_INET6 sockets to be bound to an
    //       identical socket address.  This option must be set on each
    //       socket (including the first socket) prior to calling bind(2)
    //       on the socket.  To prevent port hijacking, all of the
    //       processes binding to the same address must have the same
    //       effective UID.  This option can be employed with both TCP and
    //       UDP sockets.

    int reuse = 1;
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR,
            (const char*)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    #ifdef SO_REUSEPORT
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEPORT,
            (const char*)&reuse, sizeof(reuse)) < 0)  {
        perror("setsockopt(SO_REUSEPORT) failed");
    }
    #endif
    */

    // Server file descriptor.
    struct sockaddr_in server_fd;
    // Zero out the server address structure.
    bzero((char *) &server_fd, sizeof(server_fd));

    server_fd.sin_family = AF_INET;            // IPv4
    server_fd.sin_addr.s_addr = INADDR_ANY;    // All interfaces, 0.0.0.0.
    // Convert the port number from host byte order to network byte order.
    server_fd.sin_port = htons(port);

    int result = bind(
            socket_descriptor,
            (struct sockaddr *) &server_fd,
            sizeof(server_fd));

    // If the binding fails with this port and no particular port was specified
    // in the LF program, then try the next few ports in sequence.
    while (result != 0
            && specified_port == 0
            && port >= STARTING_PORT
            && port <= STARTING_PORT + PORT_RANGE_LIMIT) {
        lf_print("RTI failed to get port %d. Trying %d.", port, port + 1);
        port++;
        server_fd.sin_port = htons(port);
        result = bind(
                socket_descriptor,
                (struct sockaddr *) &server_fd,
                sizeof(server_fd));
    }
    if (result != 0) {
        if (specified_port == 0) {
            lf_print_error_and_exit("Failed to bind the RTI socket. Cannot find a usable port. "
                    "Consider increasing PORT_RANGE_LIMIT in net_common.h.");
        } else {
            lf_print_error_and_exit("Failed to bind the RTI socket. Specified port is not available. "
                    "Consider leaving the port unspecified");
        }
    }
    char* type = "TCP";
    if (socket_type == UDP) {
        type = "UDP";
    }
    lf_print("RTI using %s port %d for federation %s.", type, port, rti_remote->federation_id);

    if (socket_type == TCP) {
        rti_remote->final_port_TCP = port;
        // Enable listening for socket connections.
        // The second argument is the maximum number of queued socket requests,
        // which according to the Mac man page is limited to 128.
        listen(socket_descriptor, 128);
    } else if (socket_type == UDP) {
        rti_remote->final_port_UDP = port;
        // No need to listen on the UDP socket
    }

    return socket_descriptor;
}
