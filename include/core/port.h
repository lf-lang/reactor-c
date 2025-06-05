/**
 * @file port.h
 * @brief Support for optimizing sparse input through multiports.
 * @ingroup API
 *
 * @author Edward A. Lee
 *
 * This header file is for macros, functions, and structs for optimized sparse
 * input through multiports. When reading from a wide input multiport,
 * before this optimization, it was necessary for a reactor to test each
 * channel for presence each time a reaction was triggered by the multiport.
 * If few of the input channels are present, this can be very inefficient.
 * To more efficiently handle this situation, reactor authors should
 * annotate the input port with the "@sparse" annotation and use
 * lf_multiport_iterator() to read from an input multiport. For example:
 *
 * ```
 *     @sparse
 *     input[100] in:int;
 *     reaction(in) {=
 *         struct lf_multiport_iterator_t i = lf_multiport_iterator(in);
 *         int channel = lf_multiport_next(&i);
 *         while(channel >= 0) {
 *             printf("Received %d on channel %d.\n", in[channel]->value, channel);
 *             channel = lf_multiport_next(&i);
 *         }
 *     =}
 * ```
 *
 * The @ref lf_multiport_iterator() function constructs an iterator (which is
 * a struct) that can be passed to the @ref lf_multiport_next() function to
 * obtain the first channel number with a present input. Subsequent calls
 * to @ref lf_multiport_next() return the next channel index that is present
 * until there are no more, at which point they return -1.
 *
 * The way this works is that for each input multiport p1 that is marked
 * `@sparse`, a struct s of type @ref lf_sparse_io_record_t will
 * be dynamically allocated and a pointer to this struct will be put on the
 * self struct in a field named "portname__sparse".  Each port channel struct
 * within the multiport will be given a pointer to s.
 */

#ifndef PORT_H
#define PORT_H

#include <stdlib.h>
#include <stdbool.h>
#include "lf_token.h" // Defines token types and lf_port_base_t, lf_sparse_io_record

/**
 * @brief Threshold for width of multiport s.t. sparse reading is supported.
 * @ingroup API
 */
#define LF_SPARSE_WIDTH_THRESHOLD 10

/**
 * @brief Divide LF_SPARSE_WIDTH_THRESHOLD by this number to get the capacity of a
 * sparse input record for a multiport.
 * @ingroup API
 */
#define LF_SPARSE_CAPACITY_DIVIDER 10

/**
 * @brief An iterator over a record of the subset of channels of a multiport that
 * have present inputs.
 * @ingroup API
 *
 * To use this, create an iterator using the function
 * @ref lf_multiport_iterator(). That function returns a struct that can be
 * passed to the @ref lf_multiport_next() function to obtain the next channel
 * number of a present input (or -1 if there is no next present input).
 */
typedef struct lf_multiport_iterator_t {
  /**
   * @brief Index of the next present channel.
   *
   * Stores the channel index of the next present input to be returned
   * by lf_multiport_next(). This is updated each time lf_multiport_next()
   * is called to point to the next present channel.
   */
  int next;

  /**
   * @brief Current position in the sparse record.
   *
   * Tracks the current position in the sparse record array.
   * Set to -1 if lf_multiport_next() has not been called yet.
   * Used internally to iterate through the present channels.
   */
  int idx;

  /**
   * @brief Array of port pointers to iterate over.
   *
   * Points to the array of port structs that make up the multiport.
   * This is used to check the presence of each channel and access
   * their values.
   */
  lf_port_base_t** port;

  /**
   * @brief Total width of the multiport.
   *
   * Indicates the total number of channels in the multiport.
   * Used to determine when we've reached the end of the iteration.
   */
  int width;
} lf_multiport_iterator_t;

/**
 * @brief Given an array of pointers to port structs, return an iterator
 * that can be used to iterate over the present channels.
 * @ingroup Internal
 *
 * @param port An array of pointers to port structs.
 * @param width The width of the multiport (or a negative number if not
 *  a multiport).
 */
lf_multiport_iterator_t _lf_multiport_iterator_impl(lf_port_base_t** port, int width);

/**
 * @brief Macro for creating an iterator over an input multiport.
 * @ingroup API
 *
 * The argument is the port name. This returns an instance of
 * @ref lf_multiport_iterator_t on the stack, a pointer to which should be
 * passed to @ref lf_multiport_next() to advance.
 *
 * @param in The port name.
 */
#define lf_multiport_iterator(in)                                                                                      \
  (_lf_multiport_iterator_impl((lf_port_base_t**)self->_lf_##in, self->_lf_##in##_width))

/**
 * @brief Return the channel number of the next present input on the multiport
 * or -1 if there are no more present channels.
 * @ingroup API
 *
 * @param iterator The iterator.
 */
int lf_multiport_next(lf_multiport_iterator_t* iterator);

#endif /* PORT_H */
