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
 * 
 */
#include "port.h"
#include <stdio.h>

/**
 * Compare two integers pointed to. Return -1 if a < b, 0 if a == b,
 * and 1 if a > b.
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
int compare_ints(const void* a, const void* b) {
	if (*(int*)a < *(int*)b) {
		return -1;
	} else if (*(int*)a > *(int*)b) {
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
			.idx = 0,
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
						port[0]->sparse_record->present_channels,
						port[0]->sparse_record->size,
						sizeof(int),
						&compare_ints
				);
			}
			result.next = port[0]->sparse_record->present_channels[0];
		}
		return result;
	}
	// Fallback is to iterate over all port structs representing channels.
	int start = 0;
	while(start < width) {
		if (port[start]->is_present) {
			result.idx = start;
			result.next = start;
			return result;
		}
		start++;
	}
	return result;
}

/**
 * Update the specified iterator so that its 'next' field points to the
 * channel number of the next present input on the multiport or has value
 * -1 if there are no more present channels.
 * @param iterator The iterator.
 */
void lf_multiport_iterator_advance(lf_multiport_iterator_t* iterator) {
	// If the iterator is already exhausted, return.
	if (iterator->next < 0 || iterator->width <= 0) return;
	if (iterator->port[iterator->idx]->sparse_record
			&& iterator->port[iterator->idx]->sparse_record->size >= 0) {
		// Sparse record is enabled and ready to use.
		iterator->idx++;
		if (iterator->idx >= iterator->port[iterator->idx]->sparse_record->size) {
			// No more present channels.
			iterator->next = -1;
		} else {
			iterator->next = iterator->port[iterator->idx]->
					sparse_record->present_channels[iterator->idx];
		}
	} else {
		// Fallback is to iterate over all port structs representing channels.
		int start = iterator->next + 1;
		while(start < iterator->width) {
			if (iterator->port[start]->is_present) {
				iterator->next = start;
				return;
			}
			start++;
		}
		// No more present channels found.
		iterator->next = -1;
	}
}
