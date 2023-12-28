/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Soroush Bateni (soroush@utdallas.edu)
 *
 * @section LICENSE
Copyright (c) 2020, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 * Utility functions for a federate in a federated execution.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>           // For sqrtl() and powl
#include <stdarg.h>         // Defines va_list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>         // Defines memcpy()
#include <time.h>           // Defines nanosleep()
#include <netinet/in.h>     // IPPROTO_TCP, IPPROTO_UDP 
#include <netinet/tcp.h>    // TCP_NODELAY 

#include "net_util.h"
#include "util.h"

// Define socket functions only for federated execution.
#ifdef FEDERATED
#include <unistd.h>     // Defines read(), write(), and close()

#ifndef NUMBER_OF_FEDERATES
#define NUMBER_OF_FEDERATES 1
#endif

/** Number of nanoseconds to sleep before retrying a socket read. */
#define SOCKET_READ_RETRY_INTERVAL 1000000

// Mutex lock held while performing socket close operations.
// A deadlock can occur if two threads simulataneously attempt to close the same socket.
lf_mutex_t socket_mutex;

int create_real_time_tcp_socket_errexit() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        lf_print_error_system_failure("Could not open TCP socket.");
    }
    // Disable Nagle's algorithm which bundles together small TCP messages to
    // reduce network traffic.
    // TODO: Re-consider if we should do this, and whether disabling delayed ACKs
    // is enough.
    int flag = 1;
    int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    
    if (result < 0) {
        lf_print_error_system_failure("Failed to disable Nagle algorithm on socket server.");
    }
    
#if defined(PLATFORM_Linux)
    // Disable delayed ACKs. Only possible on Linux
        result = setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(int));
        
        if (result < 0) {
            lf_print_error_system_failure("Failed to disable Nagle algorithm on socket server.");
        }
#endif // Linux
    
    return sock;
}

int read_from_socket(int socket, size_t num_bytes, unsigned char* buffer) {
    if (socket < 0) {
        // Socket is not open.
        errno = EBADF;
        return -1;
    }
    ssize_t bytes_read = 0;
    int retry_count = 0;
    while (bytes_read < (ssize_t)num_bytes) {
        ssize_t more = read(socket, buffer + bytes_read, num_bytes - (size_t)bytes_read);
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

int read_from_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer) {
    assert(socket);
    int read_failed = read_from_socket(*socket, num_bytes, buffer);
    if (read_failed) {
        // Read failed.
        // Socket has probably been closed from the other side.
        // Shut down and close the socket from this side.
        shutdown(*socket, SHUT_RDWR);
        close(*socket);
        // Mark the socket closed.
        *socket = -1;
        return -1;
    }
    return 0;
}

void read_from_socket_fail_on_error(
		int* socket,
		size_t num_bytes,
		unsigned char* buffer,
		lf_mutex_t* mutex,
		char* format, ...) {
    va_list args;
    assert(socket);
    int read_failed = read_from_socket_close_on_error(socket, num_bytes, buffer);
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

ssize_t peek_from_socket(int socket, unsigned char* result) {
    ssize_t bytes_read = recv(socket, result, 1, MSG_DONTWAIT | MSG_PEEK);
    if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
    else return bytes_read;
}

int write_to_socket(int socket, size_t num_bytes, unsigned char* buffer) {
    if (socket < 0) {
        // Socket is not open.
        errno = EBADF;
        return -1;
    }
    ssize_t bytes_written = 0;
    va_list args;
    while (bytes_written < (ssize_t)num_bytes) {
        ssize_t more = write(socket, buffer + bytes_written, num_bytes - (size_t)bytes_written);
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

int write_to_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer) {
    assert(socket);
    int result = write_to_socket(*socket, num_bytes, buffer);
    if (result) {
        // Write failed.
        // Socket has probably been closed from the other side.
        // Shut down and close the socket from this side.
        shutdown(*socket, SHUT_RDWR);
        close(*socket);
        // Mark the socket closed.
        *socket = -1;
    }
    return result;
}

void write_to_socket_fail_on_error(
		int* socket,
		size_t num_bytes,
		unsigned char* buffer,
		lf_mutex_t* mutex,
		char* format, ...) {
    va_list args;
    assert(socket);
    int result = write_to_socket_close_on_error(socket, num_bytes, buffer);
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

#endif // FEDERATED

// Below are more generally useful functions.

void encode_int64(int64_t data, unsigned char* buffer) {
    // This strategy is fairly brute force, but it avoids potential
    // alignment problems.
    int shift = 0;
    for(size_t i = 0; i < sizeof(int64_t); i++) {
        buffer[i] = (unsigned char)((data & (0xffLL << shift)) >> shift);
        shift += 8;
    }
}

void encode_int32(int32_t data, unsigned char* buffer) {
    // This strategy is fairly brute force, but it avoids potential
    // alignment problems.  Note that this assumes an int32_t is four bytes.
    buffer[0] = (unsigned char)(data & 0xff);
    buffer[1] = (unsigned char)((data & 0xff00) >> 8);
    buffer[2] = (unsigned char)((data & 0xff0000) >> 16);
    buffer[3] = (unsigned char)((data & (int32_t)0xff000000) >> 24);
}

void encode_uint32(uint32_t data, unsigned char* buffer) {
    // This strategy is fairly brute force, but it avoids potential
    // alignment problems.  Note that this assumes a uint32_t is four bytes.
    buffer[0] = (unsigned char)(data & 0xff);
    buffer[1] = (unsigned char)((data & 0xff00) >> 8);
    buffer[2] = (unsigned char)((data & 0xff0000) >> 16);
    buffer[3] = (unsigned char)((data & (uint32_t)0xff000000) >> 24);
}

void encode_uint16(uint16_t data, unsigned char* buffer) {
    // This strategy is fairly brute force, but it avoids potential
    // alignment problems. Note that this assumes a short is two bytes.
    buffer[0] = (unsigned char)(data & 0xff);
    buffer[1] = (unsigned char)((data & 0xff00) >> 8);
}

int host_is_big_endian() {
    static int host = 0;
    union {
        uint32_t uint;
        unsigned char c[sizeof(uint32_t)];
    } x;
    if (host == 0) {
        // Determine the endianness of the host by setting the low-order bit.
        x.uint = 0x01;
        host = (x.c[3] == 0x01) ? HOST_BIG_ENDIAN : HOST_LITTLE_ENDIAN;
    }
    return (host == HOST_BIG_ENDIAN);
}

int32_t swap_bytes_if_big_endian_int32(int32_t src) {
    union {
        int32_t uint;
        unsigned char c[sizeof(int32_t)];
    } x;
    if (!host_is_big_endian()) return src;
    // printf("DEBUG: Host is little endian.\n");
    x.uint = src;
    // printf("DEBUG: Before swapping bytes: %lld.\n", x.ull);
    unsigned char c;
    // Swap bytes.
    c = x.c[0]; x.c[0] = x.c[3]; x.c[3] = c;
    c = x.c[1]; x.c[1] = x.c[2]; x.c[2] = c;
    // printf("DEBUG: After swapping bytes: %lld.\n", x.ull);
    return x.uint;
}

uint32_t swap_bytes_if_big_endian_uint32(uint32_t src) {
    union {
        uint32_t uint;
        unsigned char c[sizeof(uint32_t)];
    } x;
    if (!host_is_big_endian()) return src;
    // printf("DEBUG: Host is little endian.\n");
    x.uint = src;
    // printf("DEBUG: Before swapping bytes: %lld.\n", x.ull);
    unsigned char c;
    // Swap bytes.
    c = x.c[0]; x.c[0] = x.c[3]; x.c[3] = c;
    c = x.c[1]; x.c[1] = x.c[2]; x.c[2] = c;
    // printf("DEBUG: After swapping bytes: %lld.\n", x.ull);
    return x.uint;
}

int64_t swap_bytes_if_big_endian_int64(int64_t src) {
    union {
        int64_t ull;
        unsigned char c[sizeof(int64_t)];
    } x;
    if (!host_is_big_endian()) return src;
    // printf("DEBUG: Host is little endian.\n");
    x.ull = src;
    // printf("DEBUG: Before swapping bytes: %lld.\n", x.ull);
    unsigned char c;
    // Swap bytes.
    c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
    c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
    c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
    c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;
    // printf("DEBUG: After swapping bytes: %lld.\n", x.ull);
    return x.ull;
}

uint16_t swap_bytes_if_big_endian_uint16(uint16_t src) {
    union {
        uint16_t uint;
        unsigned char c[sizeof(uint16_t)];
    } x;
    if (!host_is_big_endian()) return src;
    // printf("DEBUG: Host is little endian.\n");
    x.uint = src;
    // printf("DEBUG: Before swapping bytes: %lld.\n", x.ull);
    unsigned char c;
    // Swap bytes.
    c = x.c[0]; x.c[0] = x.c[1]; x.c[1] = c;
    // printf("DEBUG: After swapping bytes: %lld.\n", x.ull);
    return x.uint;
}

int32_t extract_int32(unsigned char* bytes) {
    // Use memcpy to prevent possible alignment problems on some processors.
    union {
        int32_t uint;
        unsigned char c[sizeof(int32_t)];
    } result;
    memcpy(&result.c, bytes, sizeof(int32_t));
    return swap_bytes_if_big_endian_int32(result.uint);
}

uint32_t extract_uint32(unsigned char* bytes) {
    // Use memcpy to prevent possible alignment problems on some processors.
    union {
        uint32_t uint;
        unsigned char c[sizeof(uint32_t)];
    } result;
    memcpy(&result.c, bytes, sizeof(uint32_t));
    return swap_bytes_if_big_endian_uint32(result.uint);
}

int64_t extract_int64(unsigned char* bytes) {
    // Use memcpy to prevent possible alignment problems on some processors.
    union {
        int64_t ull;
        unsigned char c[sizeof(int64_t)];
    } result;
    memcpy(&result.c, bytes, sizeof(int64_t));
    return swap_bytes_if_big_endian_int64(result.ull);
}

uint16_t extract_uint16(unsigned char* bytes) {
    // Use memcpy to prevent possible alignment problems on some processors.
    union {
        uint16_t ushort;
        unsigned char c[sizeof(uint16_t)];
    } result;
    memcpy(&result.c, bytes, sizeof(uint16_t));
    return swap_bytes_if_big_endian_uint16(result.ushort);
}

#ifdef FEDERATED

void extract_header(
        unsigned char* buffer,
        uint16_t* port_id,
        uint16_t* federate_id,
        size_t* length
) {
    // The first two bytes are the ID of the destination reactor.
    *port_id = extract_uint16(buffer);

    // The next two bytes are the ID of the destination federate.
    *federate_id = extract_uint16(&(buffer[sizeof(uint16_t)]));

    // printf("DEBUG: Message for port %d of federate %d.\n", *port_id, *federate_id);

    // The next four bytes are the message length.
    int32_t local_length_signed = extract_int32(&(buffer[sizeof(uint16_t) + sizeof(uint16_t)]));
    if (local_length_signed < 0) {
        lf_print_error_and_exit(
            "Received an invalid message length (%d) from federate %d.",
            local_length_signed,
            *federate_id
        );
    }
    *length = (size_t)local_length_signed;

    // printf("DEBUG: Federate receiving message to port %d to federate %d of length %d.\n", port_id, federate_id, length);
}

void extract_timed_header(
        unsigned char* buffer,
        uint16_t* port_id,
        uint16_t* federate_id,
        size_t* length,
		tag_t* tag
) {
	extract_header(buffer, port_id, federate_id, length);

    tag_t temporary_tag = extract_tag(
        &(buffer[sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int32_t)])
    );
    tag->time = temporary_tag.time;
    tag->microstep = temporary_tag.microstep;
}

tag_t extract_tag(
    unsigned char* buffer
) {
    tag_t tag;
    tag.time = extract_int64(buffer);
    tag.microstep = extract_uint32(&(buffer[sizeof(int64_t)]));

    return tag;
}

void encode_tag(
    unsigned char* buffer,
    tag_t tag
){
    encode_int64(tag.time, buffer);
    encode_uint32(tag.microstep, &(buffer[sizeof(int64_t)]));
}

bool match_regex(const char* str, char* regex) {
    regex_t regex_compiled;
    regmatch_t group;
    bool valid = false;

    if (regcomp(&regex_compiled, regex, REG_EXTENDED)) {
        lf_print_error("Could not compile regex to parse RTI address");
        return valid;
    }

    // regexec returns 0 when a match is found.
    if (regexec(&regex_compiled, str, 1, &group, 0) == 0) {
        valid = true;
    }
    regfree(&regex_compiled);
    return valid;
}

bool validate_port(char* port) {
    // magic number 6 since port range is [0, 65535]
    int port_len = strnlen(port, 6);
    if (port_len < 1 || port_len > 5) {
        return false;
    }

    for (int i = 0; i < port_len; i++) {
        if (!isdigit(port[i])) {
            return false;
        }
    }
    int port_number = atoi(port);
    return port_number >= 0 && port_number <= 65535;
}

bool validate_host(const char* host) {
    // regex taken from LFValidator.xtend
    char* ipv4_regex = "((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])";
    char* host_or_FQN_regex = "^([a-z0-9]+(-[a-z0-9]+)*)|(([a-z0-9]+(-[a-z0-9]+)*\\.)+[a-z]{2,})$";
    return match_regex(host, ipv4_regex) || match_regex(host, host_or_FQN_regex);
}

bool validate_user(const char* user) {
    // regex taken from LFValidator.xtend
    char* username_regex = "^[a-z_]([a-z0-9_-]{0,31}|[a-z0-9_-]{0,30}\\$)$";
    return match_regex(user, username_regex);
}

bool extract_match_group(const char* rti_addr, char* dest, regmatch_t group, 
        int max_len, int min_len, const char* err_msg) {
    size_t size = group.rm_eo - group.rm_so;
    if (size > max_len || size < min_len) {
        lf_print_error("%s", err_msg);
        return false;
    }
    strncpy(dest, &rti_addr[group.rm_so], size);
    dest[size] = '\0';
    return true;
}

bool extract_match_groups(const char* rti_addr, char** rti_addr_strs, bool** rti_addr_flags, regmatch_t* group_array,
        int* gids, int* max_lens, int* min_lens, const char** err_msgs) {
    for (int i = 0; i < 3; i++) {
        if (group_array[gids[i]].rm_so != -1) {
            if (!extract_match_group(rti_addr, rti_addr_strs[i], group_array[gids[i]], max_lens[i], min_lens[i], err_msgs[i])) {
                return false;
            } else {
                *rti_addr_flags[i] = true;
            }
        }
    }
    return true;
}

void extract_rti_addr_info(const char* rti_addr, rti_addr_info_t* rti_addr_info) {
    const char* regex_str = "(([a-zA-Z0-9_-]{1,254})@)?([a-zA-Z0-9.]{1,255})(:([0-9]{1,5}))?";
    size_t max_groups = 6;
    // The group indices of each field of interest in the regex.
    int user_gid = 2, host_gid = 3, port_gid = 5;
    int gids[3] = {user_gid, host_gid, port_gid};
    char* rti_addr_strs[3] = {rti_addr_info->rti_user_str, rti_addr_info->rti_host_str, rti_addr_info->rti_port_str};
    bool* rti_addr_flags[3] = {&rti_addr_info->has_user, &rti_addr_info->has_host, &rti_addr_info->has_port};
    int max_lens[3] = {255, 255, 5};
    int min_lens[3] = {1, 1, 1};
    const char* err_msgs[3] = {"User name must be between 1 to 255 characters long.",
                         "Host must be between 1 to 255 characters long.",
                         "Port must be between 1 to 5 characters long."};

    regex_t regex_compiled;
    regmatch_t group_array[max_groups];

    if (regcomp(&regex_compiled, regex_str, REG_EXTENDED)) {
        lf_print_error("Could not compile regex to parse RTI address");
        return;
    }

    if (regexec(&regex_compiled, rti_addr, max_groups, group_array, 0) == 0) {
        // Check for matched username. group_array[0] is the entire matched string.
        for (int i = 1; i < max_groups; i++) {
            // Annoyingly, the rm_so and rm_eo fields are long long on some platforms and int on others.
            // To suppress warnings, cast to long long
            LF_PRINT_DEBUG("runtime rti_addr regex: so: %lld   eo: %lld\n",
                    (long long)group_array[i].rm_so,
                    (long long)group_array[i].rm_eo);
        }
        if (!extract_match_groups(rti_addr, rti_addr_strs, rti_addr_flags, group_array, gids, max_lens, min_lens, err_msgs)) {
            memset(rti_addr_info, 0, sizeof(rti_addr_info_t));
        }
    }
    regfree(&regex_compiled);
}
#endif
