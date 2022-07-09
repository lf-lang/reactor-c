/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 *
 * @section LICENSE
 * Copyright (c) 2022, The University of California at Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 *
 * Header file for macros, functions, and structs for optimized sparse I/O
 * through multiports. When reading from a wide input multiport,
 * before this optimization, it was necessary for a reactor to test each
 * channel for presence each time a reaction was triggered by the multiport.
 * If few of the input channels are present, this can be very inefficient.
 * To more efficiently handle this situation, reactor authors should use
 * lf_input_iterator() to read from an input multiport.
 *
 * The way this works is that for each input multiport p1 that is wide enough to
 * benefit from this optimization a struct s of type lf_sparse_io_record will
 * be dynamically allocated.  Each input port struct within the multiport
 * will be given a pointer to s.  Each output port struct that
 * has p1 as a destination will also be given a pointer to s unless it
 * already has a pointer to an s of another input multiport.
 * In addition, the output port struct will have a record of the input
 * channel it is writing to.  Then, when lf_set() is called, the
 * lf_sparse_io_record will record the channel being written to.
 *
 * When reading from the input port using lf_input_iterator(), if the
 * lf_sparse_io_record is available and has not overflowed its capacity,
 * then lf_input_iterator() will be able to much more quickly determine
 * which is the next channel that has a present input. Otherwise, it will
 * simply iterate through the channels as the user needed to do before.
 */

#ifndef PORT_H
#define PORT_H

#include <stdlib.h>
#include <stdbool.h>

/** Threshold for width of multiport s.t. sparse reading is supported. */
#define LF_SPARSE_WIDTH_THRESHOLD 10

/** Number of inputs that can be accepted before the input is no longer sparse. */
#define LF_SPARSE_WIDTH_CAPACITY 2

typedef struct lf_sparse_io_record_t lf_sparse_io_record_t;

/**
 * A record of channels that have present inputs
 */
struct lf_sparse_io_record_t {
	int size;  			// -1 if overflowed. 0 if empty.
	size_t capacity;    // Max number of writes to be considered sparse.
	size_t present_channels[];  // Array of channel indices that are present.
};

typedef struct lf_port_base_t lf_port_base_t;

/**
 * Port structs are customized types because their payloads are type
 * specific. This struct represents their common features. Given any
 * pointer to a port struct, it can be cast to lf_port_base_t and then
 * these common fields can be accessed.
 */
struct lf_port_base_t {
	bool is_present;
	lf_sparse_io_record_t* sparse_record; // NULL if there is no sparse record.
	int destination_channel;              // -1 if there is no destination.
};

/**
 * Set the specified channel of the specified multiport
 * to the specified value.  This is just like
 *
 *    lf_set(out[channel], val);
 *
 * except that by making the channel explicit, this can be optimized
 * to work well with lf_input_iterator(). In particular, reading an
 * input multiport will no longer iterating over all the channels to
 * determine which are present if few enough of them are present.
 *
 * It is an error to use this for a port that is not a multiport.
 *
 * @param out The output port (by name) or input of a contained
 *  reactor in form input_name.port_name.
 * @param channel The channel to write to.
 * @param value The value to send.
 */
#define lf_set_channel(out, channel, value) \
	do { \
		_LF_SET(out[channel], value); \
	} while (0)

/**
 * Given an array of pointers to port structs, return the index of the
 * first channel with index greater than or equal to the start argument
 * that is present. Return -1 if either none is present or the port
 * is not a multiport.
 * @param port An array of pointers to port structs, which will be cast to
 *  pointers to bool on the assumption that the first field of the struct
 *  is the is_present bool.
 * @param start The index of the channel at which to start checking for
 *  presence.
 * @param width The width of the multiport (or a negative number if not
 *  a multiport).
 */
int _lf_input_iterator_impl(lf_port_base_t** port, size_t start, int width);

/**
 * Macro for iterating over an input multiport.
 * The first argument is the port name and the second is the index
 * of the channel at which to start checking for present inputs.
 * This returns the first index of a channel greater than or equal
 * to the start at which an input is present.
 * If there is no such channel or the input is not a multiport,
 * then this return -1.
 */
#define lf_input_iterator(in, start) (_lf_input_iterator_impl( \
               (lf_port_base_t**)self->_lf_ ## in, \
               start, \
               self->_lf_ ## in ## _width))

#endif /* PORT_H */
/** @} */
