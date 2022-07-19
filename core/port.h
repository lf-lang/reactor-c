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
 *         struct lf_multiport_iterator_t i = lf_multiport_iterator(in_ping);
 *         while(i.next >= 0) {
 *             // Input channel i.next is present.
 *             printf("Received %d on channel %d.\n", in[i.next]->value, i.next);
 *             lf_multiport_iterator_advance(&i);
 *         }
 *     =}
 * ```
 *
 * The `lf_multiport_iterator_t` struct has a field `next` that initially
 * indicates the first channel number with a present input. The
 * `lf_multiport_iterator_advance` increments the `next` field to the next
 * channel number with a present input, or to -1 when there are no more
 * channels with present inputs.
 *
 * The way this works is that for each input multiport p1 that is marked
 * @sparse, a struct s of type `lf_sparse_io_record_t` will
 * be dynamically allocated and a pointer to this struct will be put on the
 * self struct in a field named "portname__sparse".  Each port channel struct
 * within the multiport will be given a pointer to s.
 */

#ifndef PORT_H
#define PORT_H

#include <stdlib.h>
#include <stdbool.h>
#include "utils/vector.h"

/** Threshold for width of multiport s.t. sparse reading is supported. */
#define LF_SPARSE_WIDTH_THRESHOLD 10

/**
 * Divide LF_SPARSE_WIDTH_THRESHOLD by this number to get the capacity of a
 * sparse input record for a multiport.
 */
#define LF_SPARSE_CAPACITY_DIVIDER 10

/**
 * A record of the subset of channels of a multiport that have present inputs.
 */
typedef struct lf_sparse_io_record_t {
	int size;  			// -1 if overflowed. 0 if empty.
	size_t capacity;    // Max number of writes to be considered sparse.
	size_t present_channels[];  // Array of channel indices that are present.
} lf_sparse_io_record_t;

/**
 * Port structs are customized types because their payloads are type
 * specific. This struct represents their common features. Given any
 * pointer to a port struct, it can be cast to lf_port_base_t and then
 * these common fields can be accessed.
 */
typedef struct lf_port_base_t {
	bool is_present;
	lf_sparse_io_record_t* sparse_record; // NULL if there is no sparse record.
	int destination_channel;              // -1 if there is no destination.
} lf_port_base_t;

/**
 * An iterator over a record of the subset of channels of a multiport that
 * have present inputs.  To use this, create an iterator using the function
 * lf_multiport_iterator(). That function returns a struct with a next
 * field that, if non-negative, indicates the next channel number of a present
 * input. To advance to the next channel with a present input, call
 * lf_multiport_iterator_advance(), which will update the next field.
 * When next becomes negative, then you have iterated over all the present
 * inputs.
 */
typedef struct lf_multiport_iterator_t {
	int next;
	size_t idx; // Index in the record of next.
	lf_port_base_t** port;
	int width;
} lf_multiport_iterator_t;


/**
 * A vector of pointers to the size fields of instances of
 * lf_sparse_io_record_t so that these can be set to 0 between iterations.
 * The start field of this struct will be NULL initially, so calling
 * vector_new(_lf_sparse_io_record_sizes) will be necessary to use this.
 */
extern struct vector_t _lf_sparse_io_record_sizes;

/**
 * Given an array of pointers to port structs, return an iterator
 * that can be used to iterate over the present channels.
 * @param port An array of pointers to port structs.
 * @param width The width of the multiport (or a negative number if not
 *  a multiport).
 */
lf_multiport_iterator_t _lf_multiport_iterator_impl(lf_port_base_t** port, int width);

/**
 * Macro for creating an iterator over an input multiport.
 * The argument is the port name. This returns an instance of
 * lf_multiport_iterator_t on the stack, a pointer to which should be
 * passed to lf_multiport_iterator_next() to advance.
 */
#define lf_multiport_iterator(in) (_lf_multiport_iterator_impl( \
               (lf_port_base_t**)self->_lf_ ## in, \
               self->_lf_ ## in ## _width))

/**
 * Update the specified iterator so that its 'next' field points to the
 * channel number of the next present input on the multiport or has value
 * -1 if there are no more present channels.
 * @param iterator The iterator.
 */
void lf_multiport_iterator_advance(lf_multiport_iterator_t* iterator);

#endif /* PORT_H */
/** @} */
