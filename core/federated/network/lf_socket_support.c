
#include "lf_socket_support.h"

#include <arpa/inet.h> /* htons */
#include <errno.h>
#include <linux/if.h> /* IFNAMSIZ */
#include <netinet/ether.h>
#include <netinet/in.h>   // IPPROTO_TCP, IPPROTO_UDP
#include <netinet/tcp.h>  // TCP_NODELAY
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
// #include "net_util.h"

static socket_priv_t *get_priv(netdrv_t *drv) {
    if (!drv) {
        lf_print_error_and_exit("Falied get socket_priv_t.");
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

static void socket_close(netdrv_t *drv) {
    if (!drv){
        return;
    }
    socket_priv_t *priv = get_priv(drv);
    if (priv->socket_descriptor > 0) {
        shutdown(priv->socket_descriptor, SHUT_RDWR);
        close(priv->socket_descriptor);
        priv->socket_descriptor = -1;
    }
}

// static ssize_t socket_read(netdrv_t *drv, size_t num_bytes, unsigned char* buffer, char* format, ...) {
//     // if (!drv) {
//     //     return -1;
//     // }
//     // socket_priv_t *priv = get_priv(drv);
//     // if (priv->timeout_us > 0) {
//     //     int res = -1;
//     //     do {
//     //         res = recv(priv->sock, buffer, size, MSG_TRUNC);
//     //     } while (res > 0);
//     //     return res;
//     // }
//     // return recv(priv->sock, buffer, size, MSG_TRUNC);
// }

// static ssize_t socket_write(netdrv_t *drv, size_t num_bytes, unsigned char* buffer) {
//     if (!drv) {
//         return -1;
//     }
//     socket_priv_t *priv = get_priv(drv);
//     return send(priv->sock, buffer, size, MSG_DONTWAIT);
// }

netdrv_t *netdrv_init() {
    // TODO: Should it be malloc? To support different network stacks operate simulatneously?
    netdrv_t *drv = malloc(sizeof(*drv) + sizeof(socket_priv_t));  // Don't need to call malloc() twice.
    if (!drv) {                                                    // check if malloc worked.
        lf_print_error_and_exit("Falied to malloc netdrv_t.");
    }
    memset(drv, 0, sizeof(netdrv_t));

    socket_priv_t *priv = get_priv(drv);
    priv->port = 0;
    priv->socket_descriptor = 0;
    priv->user_specified_port = 0;

    // drv->open = socket_open;
    drv->close = socket_close;
    // drv->read = socket_read;
    // drv->write = socket_write;
    // drv->get_priv = get_priv;
    return drv;
}

/**
 * @brief Create an IPv4 TCP socket with Nagle's algorithm disabled
 * (TCP_NODELAY) and Delayed ACKs disabled (TCP_QUICKACK). Exits application
 * on any error.
 *
 * @return The socket ID (a file descriptor).
 */
static int net_create_real_time_tcp_socket_errexit() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        lf_print_error_and_exit("Could not open TCP socket. Err=%d", sock);
    }
    // Disable Nagle's algorithm which bundles together small TCP messages to
    //  reduce network traffic
    // TODO: Re-consider if we should do this, and whether disabling delayed ACKs
    //  is enough.
    int flag = 1;
    int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

    if (result < 0) {
        lf_print_error_and_exit("Failed to disable Nagle algorithm on socket server.");
    }

// Disable delayed ACKs. Only possible on Linux
#if defined(PLATFORM_Linux)
    result = setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(int));

    if (result < 0) {
        lf_print_error_and_exit("Failed to disable Nagle algorithm on socket server.");
    }
#endif

    return sock;
}

void create_net_server(netdrv_t *drv, netdrv_type_t netdrv_type) {
    socket_priv_t *priv = get_priv(drv);

    // Timeout time for the communications of the server
    struct timeval timeout_time = {
            .tv_sec = TCP_TIMEOUT_TIME / BILLION,
            .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000
    };
    // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
    priv->socket_descriptor = -1;
    if (netdrv_type == RTI) {
        priv->socket_descriptor = net_create_real_time_tcp_socket_errexit();
    } else if (netdrv_type == CLOCKSYNC) {
        priv->socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // Set the appropriate timeout time
        timeout_time = (struct timeval){
                .tv_sec = UDP_TIMEOUT_TIME / BILLION,
                .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000
        };
    }
    if (priv->socket_descriptor < 0) {
        lf_print_error_and_exit("Failed to create RTI socket.");
    }

    // Set the option for this socket to reuse the same address
    int true_variable = 1; // setsockopt() requires a reference to the value assigned to an option
    if (setsockopt(
            priv->socket_descriptor,
            SOL_SOCKET,
            SO_REUSEADDR,
            &true_variable,
            sizeof(int32_t)) < 0) {
        lf_print_error("RTI failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
    }
    // Set the timeout on the socket so that read and write operations don't block for too long
    if (setsockopt(
            priv->socket_descriptor,
            SOL_SOCKET,
            SO_RCVTIMEO,
            (const char *)&timeout_time,
            sizeof(timeout_time)) < 0) {
        lf_print_error("RTI failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
    }
    if (setsockopt(
            priv->socket_descriptor,
            SOL_SOCKET,
            SO_SNDTIMEO,
            (const char *)&timeout_time,
            sizeof(timeout_time)) < 0) {
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
    bzero((char *)&server_fd, sizeof(server_fd));

    uint16_t port = 0; //TODO: Need to bring specified port. Not working currently.
    uint16_t specified_port = port;
    if (specified_port == 0) port = DEFAULT_PORT;

    server_fd.sin_family = AF_INET;          // IPv4
    server_fd.sin_addr.s_addr = INADDR_ANY;  // All interfaces, 0.0.0.0.
    // Convert the port number from host byte order to network byte order.
    server_fd.sin_port = htons(port);

    int result = bind(
        priv->socket_descriptor,
        (struct sockaddr *)&server_fd,
        sizeof(server_fd));

    // Try repeatedly to bind to a port. If no specific port is specified, then
    // increment the port number each time.

    int count = 1;
    while (result != 0 && count++ < PORT_BIND_RETRY_LIMIT) {
        if (specified_port == 0) {
            lf_print_warning("RTI failed to get port %d.", port);
            port++;
            if (port >= DEFAULT_PORT + MAX_NUM_PORT_ADDRESSES) port = DEFAULT_PORT;
            lf_print_warning("RTI will try again with port %d.", port);
            server_fd.sin_port = htons(port);
            // Do not sleep.
        } else {
            lf_print("RTI failed to get port %d. Will try again.", port);
            lf_sleep(PORT_BIND_RETRY_INTERVAL);
        }
        result = bind(
            priv->socket_descriptor,
            (struct sockaddr *)&server_fd,
            sizeof(server_fd));
    }
    if (result != 0) {
        lf_print_error_and_exit("Failed to bind the RTI socket. Port %d is not available. ", port);
    }
    char *type = "TCP";
    if (netdrv_type == CLOCKSYNC) {
        type = "UDP";
    }
    // lf_print("RTI using %s port %d for federation %s.", type, port, rti_remote->federation_id); //TODO: How to bring federation_id?

    if (netdrv_type == RTI) {
        priv->port = port;

        // Enable listening for socket connections.
        // The second argument is the maximum number of queued socket requests,
        // which according to the Mac man page is limited to 128.
        listen(priv->socket_descriptor, 128);
    } else if (netdrv_type == CLOCKSYNC) {
        priv->port = port;
        // No need to listen on the UDP socket
    }
}

void close_netdrvs(netdrv_t *rti_netdrv, netdrv_t *clock_netdrv) {
    socket_priv_t *rti_priv = get_priv(rti_netdrv);
    socket_priv_t *clock_priv = get_priv(clock_netdrv);
    // Shutdown and close the socket so that the accept() call in
    // respond_to_erroneous_connections returns. That thread should then
    // check rti->all_federates_exited and it should exit.
    if (shutdown(rti_priv->socket_descriptor, SHUT_RDWR)) {
        LF_PRINT_LOG("On shut down TCP socket, received reply: %s", strerror(errno));
    }
    // NOTE: In all common TCP/IP stacks, there is a time period,
    // typically between 30 and 120 seconds, called the TIME_WAIT period,
    // before the port is released after this close. This is because
    // the OS is preventing another program from accidentally receiving
    // duplicated packets intended for this program.
    close(rti_priv->socket_descriptor);

    if (clock_priv->socket_descriptor > 0) {
        if (shutdown(clock_priv->socket_descriptor, SHUT_RDWR)) {
            LF_PRINT_LOG("On shut down UDP socket, received reply: %s", strerror(errno));
        }
        close(clock_priv->socket_descriptor);
    }
}

netdrv_t *accept_connection(netdrv_t *rti_netdrv) {
    netdrv_t *fed_netdrv = netdrv_init();
    socket_priv_t *rti_priv = get_priv(rti_netdrv);
    socket_priv_t *fed_priv = get_priv(fed_netdrv);
    // Wait for an incoming connection request.
    struct sockaddr client_fd;
    uint32_t client_length = sizeof(client_fd);
    // The following blocks until a federate connects.
    while (1) {
        fed_priv->socket_descriptor = accept(rti_priv->socket_descriptor, &client_fd, &client_length);
        if (fed_priv->socket_descriptor >= 0) {
            // Got a socket
            break;
        } else if (fed_priv->socket_descriptor < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
            lf_print_error_and_exit("RTI failed to accept the socket. %s.", strerror(errno));
        } else {
            // Try again
            lf_print_warning("RTI failed to accept the socket. %s. Trying again.", strerror(errno));
            continue;
        }
    }
    // Assign the address information for federate.
    // The IP address is stored here as an in_addr struct (in .server_ip_addr) that can be useful
    // to create sockets and can be efficiently sent over the network.
    // First, convert the sockaddr structure into a sockaddr_in that contains an internet address.
    struct sockaddr_in *pV4_addr = (struct sockaddr_in *)&client_fd;
    // Then extract the internet address (which is in IPv4 format) and assign it as the federate's socket server
    fed_priv->server_ip_addr = pV4_addr->sin_addr;
    return fed_netdrv;
}

int read_from_netdrv(netdrv_t *drv, size_t num_bytes, unsigned char* buffer) {
    socket_priv_t *priv = get_priv(drv);
    if (priv->socket_descriptor < 0) {
        // Socket is not open.
        errno = EBADF;
        return -1;
    }
    ssize_t bytes_read = 0;
    int retry_count = 0;
    while (bytes_read < (ssize_t)num_bytes) {
        ssize_t more = read(priv->socket_descriptor, buffer + bytes_read, num_bytes - (size_t)bytes_read);
        if(more < 0 && retry_count++ < NUM_SOCKET_RETRIES 
                && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            // Those error codes set by the socket indicates
            // that we should try again (@see man errno).
            lf_print_warning("Reading from socket failed. Will try again.");
            lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
            continue;
        } else if (more < 0) {
            // A more serious error occurred.
            return -1;
        } else if (more == 0) {
            // EOF received.
            return 1;
        }
        bytes_read += more;
    }
    return 0;    
}
int read_from_netdrv_close_on_error(netdrv_t *drv, size_t num_bytes, unsigned char* buffer) {
    socket_priv_t *priv = get_priv(drv);
    assert(&priv->socket_descriptor);
    int read_failed = read_from_netdrv(drv, num_bytes, buffer);
    if (read_failed) {
        // Read failed.
        // Socket has probably been closed from the other side.
        // Shut down and close the socket from this side.
        shutdown(priv->socket_descriptor, SHUT_RDWR);
        close(priv->socket_descriptor);
        // Mark the socket closed.
        priv->socket_descriptor = -1;
        return -1;
    }
    return 0;
}
void read_from_netdrv_fail_on_error(
		netdrv_t *drv,
		size_t num_bytes,
		unsigned char* buffer,
		lf_mutex_t* mutex,
		char* format, ...) {
    socket_priv_t *priv = get_priv(drv);
    va_list args;
    assert(&priv->socket_descriptor);
    int read_failed = read_from_netdrv_close_on_error(drv, num_bytes, buffer);
    if (read_failed) {
        // Read failed.
        if (mutex != NULL) {
            lf_mutex_unlock(mutex);
        }
        if (format != NULL) {
            lf_print_error_system_failure(format, args);
        } else {
            lf_print_error_system_failure("Failed to read from socket.");
        }
    }
}

ssize_t peek_from_netdrv(netdrv_t *drv, unsigned char* result) {
    socket_priv_t *priv = get_priv(drv);
    ssize_t bytes_read = recv(priv->socket_descriptor, result, 1, MSG_DONTWAIT | MSG_PEEK);
    if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
    else return bytes_read;
}

int write_to_netdrv(netdrv_t *drv, size_t num_bytes, unsigned char* buffer) {
    socket_priv_t *priv = get_priv(drv);
    if (priv->socket_descriptor < 0) {
        // Socket is not open.
        errno = EBADF;
        return -1;
    }
    ssize_t bytes_written = 0;
    va_list args;
    while (bytes_written < (ssize_t)num_bytes) {
        ssize_t more = write(priv->socket_descriptor, buffer + bytes_written, num_bytes - (size_t)bytes_written);
        if (more <= 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            // The error codes EAGAIN or EWOULDBLOCK indicate
            // that we should try again (@see man errno).
            // The error code EINTR means the system call was interrupted before completing.
            LF_PRINT_DEBUG("Writing to socket was blocked. Will try again.");
            lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
            continue;
        } else if (more < 0) {
            // A more serious error occurred.
            return -1;
        }
        bytes_written += more;
    }
    return 0;
}

int write_to_netdrv_close_on_error(netdrv_t *drv, size_t num_bytes, unsigned char* buffer) {
    socket_priv_t *priv = get_priv(drv);
    assert(&priv->socket_descriptor);
    int result = write_to_netdrv(drv, num_bytes, buffer);
    if (result) {
        // Write failed.
        // Netdrv has probably been closed from the other side.
        // Shut down and close the netdrv from this side.
        drv->close(drv);
    }
    return result;
}

void write_to_netdrv_fail_on_error(
		netdrv_t *drv,
		size_t num_bytes,
		unsigned char* buffer,
		lf_mutex_t* mutex,
		char* format, ...) {
    va_list args;
    socket_priv_t *priv = get_priv(drv);
    assert(&priv->socket_descriptor);
    int result = write_to_netdrv_close_on_error(drv, num_bytes, buffer);
    if (result) {
        // Write failed.
        if (mutex != NULL) {
            lf_mutex_unlock(mutex);
        }
        if (format != NULL) {
            lf_print_error_system_failure(format, args);
        } else {
            lf_print_error("Failed to write to socket. Closing it.");
        }
    }
}