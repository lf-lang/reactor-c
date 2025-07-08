/**
 * @file net_util.h
 * @brief Network utility functions for Lingua Franca programs.
 * @ingroup Federated
 *
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
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

#ifdef FEDERATED
#include "socket_common.h"
#endif

#define HOST_LITTLE_ENDIAN 1
#define HOST_BIG_ENDIAN 2

/**
 * @brief Return true (1) if the host is big endian. Otherwise, return false.
 * @ingroup Federated
 */
int host_is_big_endian(void);

/**
 * @brief Write the specified data as a sequence of bytes starting at the specified address.
 * @ingroup Federated
 *
 * This encodes the data in little-endian order (lowest order byte first).
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_int64(int64_t data, unsigned char* buffer);

/**
 * @brief Write the specified data as a sequence of bytes starting at the specified address.
 * @ingroup Federated
 *
 * This encodes the data in little-endian order (lowest order byte first).
 * This works for int32_t.
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_int32(int32_t data, unsigned char* buffer);

/**
 * @brief Write the specified data as a sequence of bytes starting at the specified address.
 * @ingroup Federated
 *
 * This encodes the data in little-endian order (lowest order byte first).
 * This works for uint32_t.
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_uint32(uint32_t data, unsigned char* buffer);

/**
 * @brief Write the specified data as a sequence of bytes starting at the specified address.
 * @ingroup Federated
 *
 * This encodes the data in little-endian order (lowest order byte first).
 * @param data The data to write.
 * @param buffer The location to start writing.
 */
void encode_uint16(uint16_t data, unsigned char* buffer);

/**
 * @brief If this host is little endian, then reverse the order of the bytes of the argument.
 * @ingroup Federated
 *
 * Otherwise, return the argument unchanged.
 * This can be used to convert the argument to network order (big endian) and then back again.
 * Network transmissions, by convention, are big endian,
 * meaning that the high-order byte is sent first.
 * But many platforms, including my Mac, are little endian,
 * meaning that the low-order byte is first in memory.
 * @param src The argument to convert.
 */
int32_t swap_bytes_if_big_endian_int32(int32_t src);

/**
 * @brief If this host is little endian, then reverse the order of the bytes of the argument.
 * @ingroup Federated
 *
 * Otherwise, return the argument unchanged.
 * This can be used to convert the argument to network order (big endian) and then back again.
 * Network transmissions, by convention, are big endian, meaning that the high-order byte is sent first.
 * But many platforms, including my Mac, are little endian, meaning that the low-order byte is first in memory.
 * @param src The argument to convert.
 */
int64_t swap_bytes_if_big_endian_int64(int64_t src);

/**
 * @brief If this host is little endian, then reverse the order of the bytes of the argument.
 * @ingroup Federated
 *
 * Otherwise, return the argument unchanged.
 * This can be used to convert the argument to network order (big endian) and then back again.
 * Network transmissions, by convention, are big endian,
 * meaning that the high-order byte is sent first.
 * But many platforms, including my Mac, are little endian,
 * meaning that the low-order byte is first in memory.
 * @param src The argument to convert.
 */
uint16_t swap_bytes_if_big_endian_uint16(uint16_t src);

/**
 * @brief This will swap the order of the bytes if this machine is big endian.
 * @ingroup Federated
 *
 * @param bytes The address of the start of the sequence of bytes.
 */
int32_t extract_int32(unsigned char* bytes);

/**
 * @brief This will swap the order of the bytes if this machine is big endian.
 * @ingroup Federated
 *
 * @param bytes The address of the start of the sequence of bytes.
 */
int64_t extract_int64(unsigned char* bytes);

/**
 * @brief Extract an uint16_t from the specified byte sequence.
 * @ingroup Federated
 *
 * This will swap the order of the bytes if this machine is big endian.
 * @param bytes The address of the start of the sequence of bytes.
 */
uint16_t extract_uint16(unsigned char* bytes);

#ifdef FEDERATED

/**
 * @brief Extract the core header information that all messages between federates share.
 * @ingroup Federated
 *
 * The core header information is two bytes with the ID of the destination port,
 * two bytes with the ID of the destination federate, and four bytes with the length of the message.
 * @note Only present when federated execution is enabled.
 * @param buffer The buffer to read from.
 * @param port_id The place to put the port ID.
 * @param federate_id The place to put the federate ID.
 * @param length The place to put the length.
 */
void extract_header(unsigned char* buffer, uint16_t* port_id, uint16_t* federate_id, size_t* length);

/**
 * @brief Extract the timed header information for timed messages between federates.
 * @ingroup Federated
 *
 * This is two bytes with the ID of the destination port, two bytes with the ID of the destination
 * federate, four bytes with the length of the message, eight bytes with a timestamp, and four bytes with a microstep.
 * @param buffer The buffer to read from.
 * @param port_id The place to put the port ID.
 * @param federate_id The place to put the federate ID.
 * @param length The place to put the length.
 * @param tag The place to put the tag.
 */
void extract_timed_header(unsigned char* buffer, uint16_t* port_id, uint16_t* federate_id, size_t* length, tag_t* tag);

/**
 * @brief Extract tag information from buffer.
 * @ingroup Federated
 *
 * The tag is transmitted as a 64-bit (8 byte) signed integer for time and a 32-bit (4 byte) unsigned integer for
 * microstep.
 * @param buffer The buffer to read from.
 * @return The extracted tag.
 */
tag_t extract_tag(unsigned char* buffer);

/**
 * @brief Encode tag information into buffer.
 * @ingroup Federated
 *
 * Buffer must have been allocated externally.
 * @param buffer The buffer to encode into.
 * @param tag The tag to encode into 'buffer'.
 */
void encode_tag(unsigned char* buffer, tag_t tag);

/**
 * @brief A helper struct for passing rti_addr information between lf_parse_rti_addr and extract_rti_addr_info
 * @ingroup Federated
 *
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
 * @brief Check whether str matches regex.
 * @ingroup Federated
 *
 * @param str The string to check.
 * @param regex The regex to check against.
 * @return true if there is a match, false otherwise.
 */
bool match_regex(const char* str, char* regex);

/**
 * @brief Check whether port is valid.
 * @ingroup Federated
 *
 * @param port The port to check.
 * @return true if valid, false otherwise.
 */
bool validate_port(char* port);

/**
 * @brief Check whether host is valid.
 * @ingroup Federated
 *
 * @param host The host to check.
 * @return true if valid, false otherwise.
 */
bool validate_host(const char* host);

/**
 * @brief Check whether user is valid.
 * @ingroup Federated
 *
 * @param user The user to check.
 * @return true if valid, false otherwise.
 */
bool validate_user(const char* user);

/**
 * @brief Extract one match group from the rti_addr regex .
 * @ingroup Federated
 *
 * @param rti_addr The rti_addr to extract from.
 * @param dest The destination to store the match group.
 * @param group The group to extract.
 * @param max_len The maximum length of the match group.
 * @param min_len The minimum length of the match group.
 * @param err_msg The error message to return if there is an error.
 * @return true if SUCCESS, else false.
 */
bool extract_match_group(const char* rti_addr, char* dest, regmatch_t group, size_t max_len, size_t min_len,
                         const char* err_msg);

/**
 * @brief Extract match groups from the rti_addr regex.
 * @ingroup Federated
 *
 * @param rti_addr The rti_addr to extract from.
 * @param rti_addr_strs The array of rti_addr strings to store the match groups.
 * @param rti_addr_flags The array of rti_addr flags to store the match groups.
 * @param group_array The array of regmatch_t to store the match groups.
 * @param gids The array of gids to store the match groups.
 * @param max_lens The array of max_lens to store the match groups.
 * @param min_lens The array of min_lens to store the match groups.
 * @param err_msgs The array of error messages to store the match groups.
 * @return true if success, else false.
 */
bool extract_match_groups(const char* rti_addr, char** rti_addr_strs, bool** rti_addr_flags, regmatch_t* group_array,
                          int* gids, size_t* max_lens, size_t* min_lens, const char** err_msgs);

/**
 * @brief Extract the host, port and user from rti_addr.
 * @ingroup Federated
 *
 * @param rti_addr The rti_addr to extract from.
 * @param rti_addr_info The rti_addr_info into which to store the extracted information.
 */
void extract_rti_addr_info(const char* rti_addr, rti_addr_info_t* rti_addr_info);

#endif // FEDERATED

#endif /* NET_UTIL_H */
