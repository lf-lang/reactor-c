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
 * Given an array of pointers to port structs, return the index of the
 * first channel with index greater than or equal to the start argument
 * that is present. Return -1 if either none is present or the port
 * is not a multiport.
 * @param port An array of pointers to port structs.
 * @param start The index of the channel at which to start checking for
 *  presence.
 * @param width The width of the multiport (or a negative number if not
 *  a multiport).
 */
int _lf_input_iterator_impl(lf_port_base_t** port, size_t start, int width) {
	// NOTE: Synchronization is not required because all writers must have
	// completed by the time this is invoked.
	if (width < 0 || start >= width) return -1;
	if (port[start]->sparse_record
			&& port[start]->sparse_record->size >= 0) {
		// Sparse record is enabled and ready to use.
		int next = 0;
		while (next < port[start]->sparse_record->size) {
			if (port[start]->sparse_record->present_channels[next] >= start) {
				return port[start]->sparse_record->present_channels[next];
			}
			next++;
		}
		// None present greater than or equal to start.
		return -1;
	}
	// Fallback is to iterate over all port structs representing channels.
	while(start < width) {
		if (port[start]->is_present) return start;
		start++;
	}
	return -1;
}
