/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
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
 * Header file for network utility functions for Lingua Franca programs.
 * Note that these functions do not acquire any mutexes. To use them,
 * you must ensure either that only one thread ever sends on each socket
 * and one thread receives on each socket (these two can be the same thread)
 * or that the caller handles mutual exclusion to prevent more than one thread
 * from accessing the socket at a time.
 */

#ifndef NET_UTIL_H
#define NET_UTIL_H

#ifdef PLATFORM_ARDUINO
#error To be implemented. No support for federation on Arduino yet.
#else
#include <sys/socket.h>
#include <regex.h>
#endif

#include <sys/types.h>
#include <stdbool.h>

#include "low_level_platform.h"
#include "tag.h"

#define NUM_SOCKET_RETRIES 10
#define DELAY_BETWEEN_SOCKET_RETRIES MSEC(100)

#define HOST_LITTLE_ENDIAN 1
#define HOST_BIG_ENDIAN 2

/** 
 * Return true (1) if the host is big endian. Otherwise,
 * return false.
 */
int host_is_big_endian(void);

#ifdef FEDERATED

/**
 * Mutex protecting socket close operations.
 */
extern lf_mutex_t socket_mutex;

/**
 * @brief Create an IPv4 TCP socket with Nagle's algorithm disabled
 * (TCP_NODELAY) and Delayed ACKs disabled (TCP_QUICKACK). Exits application
 * on any error.
 *
 * @return The socket ID (a file descriptor).
 */
int create_real_time_tcp_socket_errexit();

/**
 * Read the specified number of bytes from the specified socket into the specified buffer.
 * If an error occurs during this reading, return -1 and set errno to indicate
 * the cause of the error. If the read succeeds in reading the specified number of bytes,
 * return 0. If an EOF occurs before reading the specified number of bytes, return 1.
 * This function repeats the read attempt until the specified number of bytes
 * have been read, an EOF is read, or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by DELAY_BETWEEN_SOCKET_RETRIES.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @return 0 for success, 1 for EOF, and -1 for an error.
 */
int read_from_socket(int socket, size_t num_bytes, unsigned char* buffer);

/**
 * Read the specified number of bytes to the specified socket using read_from_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int read_from_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer);

/**
 * Read the specified number of bytes from the specified socket into the
 * specified buffer. If a disconnect or an EOF occurs during this
 * reading, then if format is non-null, report an error and exit.
 * If the mutex argument is non-NULL, release the mutex before exiting.
 * If format is null, then report the error, but do not exit.
 * This function takes a formatted string and additional optional arguments
 * similar to printf(format, ...) that is appended to the error messages.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @param format A printf-style format string, followed by arguments to
 *  fill the string, or NULL to not exit with an error message.
 * @return The number of bytes read, or 0 if an EOF is received, or
 *  a negative number for an error.
 */
void read_from_socket_fail_on_error(
		int* socket,
		size_t num_bytes,
		unsigned char* buffer,
		lf_mutex_t* mutex,
		char* format, ...);

/**
 * Without blocking, peek at the specified socket and, if there is
 * anything on the queue, put its first byte at the specified address and return 1.
 * If there is nothing on the queue, return 0, and if an error occurs,
 * return -1.
 * @param socket The socket ID.
 * @param result Pointer to where to put the first byte available on the socket.
 */
ssize_t peek_from_socket(int socket, unsigned char* result);

/**
 * Write the specified number of bytes to the specified socket from the
 * specified buffer. If an error occurs, return -1 and set errno to indicate
 * the cause of the error. If the write succeeds, return 0.
 * This function repeats the attempt until the specified number of bytes
 * have been written or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by
 * DELAY_BETWEEN_SOCKET_RETRIES.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_socket(int socket, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using write_to_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using
 * write_to_socket_close_on_error and exit with an error code if an error occurs.
 * If the mutex argument is non-NULL, release the mutex before exiting.  If the
 * format argument is non-null, then use it an any additional arguments to form
 * the error message using printf conventions. Otherwise, print a generic error
 * message.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A format string for error messages, followed by any number of
 *  fields that will be used to fill the format string as in printf, or NULL
 *  to print a generic error message.
 */
void write_to_socket_fail_on_error(
		int* socket,
		size_t num_bytes,
		unsigned char* buffer,
		lf_mutex_t* mutex,
		char* format, ...);

#endif // FEDERATED

/**
 * Write the specified data as a sequence of bytes starting
 * at the specified address. This encodes the data in little-endian
 * order (lowest order byte first).
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_int64(int64_t data, unsigned char* buffer);

/** 
 * Write the specified data as a sequence of bytes starting
 * at the specified address. This encodes the data in little-endian
 * order (lowest order byte first). This works for int32_t.
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_int32(int32_t data, unsigned char* buffer);

/**
 * Write the specified data as a sequence of bytes starting
 * at the specified address. This encodes the data in little-endian
 * order (lowest order byte first). This works for uint32_t.
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_uint32(uint32_t data, unsigned char* buffer);

/** 
 * Write the specified data as a sequence of bytes starting
 * at the specified address. This encodes the data in little-endian
 * order (lowest order byte first).
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_uint16(uint16_t data, unsigned char* buffer);

/** 
 * If this host is little endian, then reverse the order of
 * the bytes of the argument. Otherwise, return the argument
 * unchanged. This can be used to convert the argument to
 * network order (big endian) and then back again.
 * Network transmissions, by convention, are big endian,
 * meaning that the high-order byte is sent first.
 * But many platforms, including my Mac, are little endian,
 * meaning that the low-order byte is first in memory.
 * @param src The argument to convert.
 */
int32_t swap_bytes_if_big_endian_int32(int32_t src);

/** 
 * If this host is little endian, then reverse the order of
 * the bytes of the argument. Otherwise, return the argument
 * unchanged. This can be used to convert the argument to
 * network order (big endian) and then back again.
 * Network transmissions, by convention, are big endian,
 * meaning that the high-order byte is sent first.
 * But many platforms, including my Mac, are little endian,
 * meaning that the low-order byte is first in memory.
 * @param src The argument to convert.
 */
int64_t swap_bytes_if_big_endian_int64(int64_t src);

/** 
 * If this host is little endian, then reverse the order of
 * the bytes of the argument. Otherwise, return the argument
 * unchanged. This can be used to convert the argument to
 * network order (big endian) and then back again.
 * Network transmissions, by convention, are big endian,
 * meaning that the high-order byte is sent first.
 * But many platforms, including my Mac, are little endian,
 * meaning that the low-order byte is first in memory.
 * @param src The argument to convert.
 */
uint16_t swap_bytes_if_big_endian_uint16(uint16_t src);

/**
 * This will swap the order of the bytes if this machine is big endian.
 * @param bytes The address of the start of the sequence of bytes.
 */
int32_t extract_int32(unsigned char* bytes);

/**
 * This will swap the order of the bytes if this machine is big endian.
 * @param bytes The address of the start of the sequence of bytes.
 */
int64_t extract_int64(unsigned char* bytes);

/** 
 * Extract an uint16_t from the specified byte sequence.
 * This will swap the order of the bytes if this machine is big endian.
 * @param bytes The address of the start of the sequence of bytes.
 */
uint16_t extract_uint16(unsigned char* bytes);

#ifdef FEDERATED

/**
 * Extract the core header information that all messages between
 * federates share. The core header information is two bytes with
 * the ID of the destination port, two bytes with the ID of the destination
 * federate, and four bytes with the length of the message.
 * @param buffer The buffer to read from.
 * @param port_id The place to put the port ID.
 * @param federate_id The place to put the federate ID.
 * @param length The place to put the length.
 */
void extract_header(
        unsigned char* buffer,
        uint16_t* port_id,
        uint16_t* federate_id,
        size_t* length
);

/**
 * Extract the timed header information for timed messages between
 * federates. This is two bytes with the ID of the destination port,
 * two bytes with the ID of the destination
 * federate, four bytes with the length of the message,
 * eight bytes with a timestamp, and four bytes with a microstep.
 * @param buffer The buffer to read from.
 * @param port_id The place to put the port ID.
 * @param federate_id The place to put the federate ID.
 * @param length The place to put the length.
 * @param tag The place to put the tag.
 */
void extract_timed_header(
        unsigned char* buffer,
        uint16_t* port_id,
        uint16_t* federate_id,
        size_t* length,
		tag_t* tag
);

/**
 * Extract tag information from buffer.
 *
 * The tag is transmitted as a 64-bit (8 byte) signed integer for time and a
 * 32-bit (4 byte) unsigned integer for microstep.
 *
 * @param buffer The buffer to read from.
 * @return The extracted tag.
 */
tag_t extract_tag(
	unsigned char* buffer
);

/**
 * Encode tag information into buffer.
 *
 * Buffer must have been allocated externally.
 *
 * @param buffer The buffer to encode into.
 * @param tag The tag to encode into 'buffer'.
 */
void encode_tag(
    unsigned char* buffer,
	tag_t tag
);

/**
 * A helper struct for passing rti_addr information between lf_parse_rti_addr and extract_rti_addr_info
 */
typedef struct rti_addr_info_t {
    char rti_host_str[256];
    char rti_port_str[6];
    char rti_user_str[256];
    bool has_host;
    bool has_port;
    bool has_user;
} rti_addr_info_t;

/**
 * Check whether str matches regex.
 * @return true if there is a match, false otherwise.
 */
bool match_regex(const char* str, char* regex);

/**
 * Check whether port is valid.
 * @return true if valid, false otherwise.
 */
bool validate_port(char* port);

/**
 * Check whether host is valid.
 * @return true if valid, false otherwise.
 */
bool validate_host(const char* host);

/**
 * Check whether user is valid.
 * @return true if valid, false otherwise.
 */
bool validate_user(const char* user);

/**
 * Extract one match group from the rti_addr regex .
 * @return true if SUCCESS, else false.
 */
bool extract_match_group(const char* rti_addr, char* dest, regmatch_t group,
		int max_len, int min_len, const char* err_msg);

/**
 * Extract match groups from the rti_addr regex.
 * @return true if success, else false.
 */
bool extract_match_groups(const char* rti_addr, char** rti_addr_strs, bool** rti_addr_flags, regmatch_t* group_array, int* gids, int* max_lens, int* min_lens, const char** err_msgs);

/**
 * Extract the host, port and user from rti_addr.
 */
void extract_rti_addr_info(const char* rti_addr, rti_addr_info_t* rti_addr_info);

#endif // FEDERATED

#endif /* NET_UTIL_H */
