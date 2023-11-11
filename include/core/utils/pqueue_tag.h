/**
 * @file tag_pqueue.h
 * @author Byeonggil Jun
 * @author Edward A. Lee
 * @copyright (c) 2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Priority queue that uses tags for sorting.
 * 
 * This file extends the pqueue infrastructure with support for queues that are sorted
 * by tag instead of by a long long.  Elements in this queue are structs of type
 * tagged_element_t, which can carry a payload that is a pointer (or an int, assuming
 * that an int has no more bits than a pointer). What you put onto the
 * queue is a pointer to a tagged_element_t struct. That pointer is cast to pqueue_pri_t,
 * an alias for long long.
 */

#ifndef PQUEUE_TAG_H
#define PQUEUE_TAG_H

#include "pqueue_base.h"
#include "tag.h"

/**
 * @brief An element in the priority queue, sorted by tag.
 * The payload can be pointer to anything or an int, assuming that an int
 * can be safely cast to a pointer.  The payload may also go unused if you
 * want just a priority queue with tags only.  In this design, a pointer to
 * this struct is priority (can be cast to pqueue_pri_t) and also an element
 * in the queue (can be cast to void*).
 */
typedef struct {
    tag_t tag;
    size_t pos;   // Needed by any pqueue element.
    void* payload;
} pqueue_tag_element_t;

/**
 * Type of a priority queue using tags.
 */
typedef pqueue_t pqueue_tag_t;

/**
 * @brief Create a priority queue sorted by tags.
 * The elements of the priority queue will be of type pqueue_tag_element_t.
 * The caller should call pqueue_tag_free() when finished with the queue.
 * @return A dynamically allocated priority queue or NULL if memory allocation fails.
 */
pqueue_tag_t* pqueue_tag_init(size_t initial_size);

#endif // PQUEUE_TAG_H