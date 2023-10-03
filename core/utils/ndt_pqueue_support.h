/*************
Copyright (c) 2022, The University of California at Berkeley.

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
***************/

/**
 * @file pqueue_support.h
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @brief Header-only support functions for pqueue.
 */

#ifndef NDT_PQUEUE_SUPPORT_H
#define NDT_PQUEUE_SUPPORT_H


#include "../reactor.h"
#include "tag.h"

// ********** NDT Priority Queue Support Start

typedef struct {
    tag_t tag;
    size_t pos;
} ndt_node;

/**
 * Return whether the tags contained by the first and second argument 
 * are given in reverse order.
*/
static int tag_in_reverse_order(pqueue_pri_t thiz, pqueue_pri_t that);

/**
 * Return whether or not the tags contained by given pointers are identical.
 */
static int tag_matches(void* next, void* curr);

/**
 * Report a priority equal to the pointer to an ndt_node.
 * Used for sorting pointers to ndt_node in the NDT queue.
 */
static pqueue_pri_t get_ndt_priority(void *a);

/**
 * Return the given ndt_node's position in the ndt_queue.
 */
static size_t get_ndtq_position(void *a);

/**
 * Return the given ndt_node's position in the ndt_queue.
 */
static void set_ndtq_position(void *a, size_t pos);

/**
 * 
*/
static int ndt_node_matches(void* next, void* curr);

/**
 * Print some information about the given ndt_node.
 * 
 * DEBUG function only.
 */
static void print_tag(void *reaction);

// ********** NDT Priority Queue Support End
#endif
