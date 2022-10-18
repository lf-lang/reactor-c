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
 * through multiports.
 */
#include <stdio.h>

#include "port.h"
#include "vector.h"

/**
 * Compare two non-negative integers pointed to. Return -1 if a < b, 0 if a == b,
 * and 1 if a > b.
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
int compare_sizes(const void* a, const void* b) {
	if (*(size_t*)a < *(size_t*)b) {
		return -1;
	} else if (*(size_t*)a > *(size_t*)b) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * Given an array of pointers to port structs, return an iterator
 * that can be used to iterate over the present channels.
 * @param port An array of pointers to port structs.
 * @param width The width of the multiport (or a negative number if not
 *  a multiport).
 */
lf_multiport_iterator_t _lf_multiport_iterator_impl(lf_port_base_t** port, int width) {
	// NOTE: Synchronization is not required because all writers must have
	// completed by the time this is invoked.
	struct lf_multiport_iterator_t result = (lf_multiport_iterator_t) {
			.next = -1,
			.idx = -1, // Indicate that lf_multiport_next() has not been called.
			.port = port,
			.width = width
	};
	if (width <= 0) return result;
	if (port[0]->sparse_record && port[0]->sparse_record->size >= 0) {
		// Sparse record is enabled and ready to use.
		if (port[0]->sparse_record->size > 0) {
			// Need to sort it first (if the length is greater than 1).
			if (port[0]->sparse_record->size > 1) {
				qsort(
						&port[0]->sparse_record->present_channels[0],
						(size_t)port[0]->sparse_record->size,
						sizeof(size_t),
						&compare_sizes
				);
			}
			// NOTE: Following cast is unsafe if there more than 2^31 channels.
			result.next = (int)port[0]->sparse_record->present_channels[0];
		}
		return result;
	}
	// Fallback is to iterate over all port structs representing channels.
	int start = 0;
	while(start < width) {
		if (port[start]->is_present) {
			result.next = start;
			return result;
		}
		start++;
	}
	return result;
}

/**
 * Return the channel number of the next present input on the multiport
 * or -1 if there are no more present channels.
 * @param iterator The iterator.
 */
int lf_multiport_next(lf_multiport_iterator_t* iterator) {
	// If the iterator has not been used, return next.
	if (iterator->idx < 0) {
		iterator->idx = 0;
		return iterator->next;
	}
	// If the iterator is already exhausted, return.
	if (iterator->next < 0 || iterator->width <= 0) {
		return -1;
	}
	struct lf_sparse_io_record_t* sparse_record
			= iterator->port[iterator->idx]->sparse_record;
	if (sparse_record && sparse_record->size >= 0) {
		// Sparse record is enabled and ready to use.
		iterator->idx++;
		if (iterator->idx >= sparse_record->size) {
			// No more present channels.
			iterator->next = -1;
		} else {
			// NOTE: Following cast is unsafe if there more than 2^31 channels.
			iterator->next = (int)sparse_record->present_channels[iterator->idx];
		}
		return iterator->next;
	} else {
		// Fall back to iterate over all port structs representing channels.
		int start = iterator->next + 1;
		while(start < iterator->width) {
			if (iterator->port[start]->is_present) {
				iterator->next = start;
				return iterator->next;
			}
			start++;
		}
		// No more present channels found.
		iterator->next = -1;
		return iterator->next;
	}
}
