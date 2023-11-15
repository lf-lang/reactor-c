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
 * pqueue_tag_element_t or a derived struct, as explained below. What you put onto the
 * queue is a pointer to a tagged_element_t struct. That pointer, when cast to pqueue_pri_t,
 * an alias for long long, also serves as the "priority" for the queue.
 */

#ifndef PQUEUE_TAG_H
#define PQUEUE_TAG_H

#include "pqueue_base.h"
#include "tag.h"

/**
 * @brief The type for an element in a priority queue that is sorted by tag.
 * 
 * In this design, a pointer to this struct is also a "priority" (it can be
 * cast to pqueue_pri_t).  The actual priority is the tag field of the struct,
 * in that the queue is sorted from least tag to largest.
 * 
 * If your struct is dynamically allocated using malloc or calloc, and you
 * would like the memory freed when the queue is freed, then set the is_dynamic
 * field to a non-zero value.
 * 
 * To customize the element you put onto the queue, you can create your
 * own element struct type by simply declaring the first field to be
 * a pqueue_tag_element_t.  For example, if you want an element of the
 * queue to include a pointer to your own payload, you can declare the
 * following struct type:
 * <pre>
 *     typedef struct {
 *         pqueue_tag_element_t base;
 *         my_type* my_payload;
 *     } my_element_type_t;
 * </pre>
 * When inserting your struct into the queue, simply cast your pointer
 * to (pqueue_tag_element_t*).  When accessing your struct from the queue,
 * simply cast the result to (my_element_type_t*);
 */
typedef struct {
    tag_t tag;
    size_t pos;       // Needed by any pqueue element.
    int is_dynamic;   // Non-zero to free this struct when the queue is freed.
} pqueue_tag_element_t;

/**
 * Type of a priority queue sorted by tags.
 */
typedef pqueue_t pqueue_tag_t;

/**
 * @brief Create a priority queue sorted by tags.
 * The elements of the priority queue will be of type pqueue_tag_element_t.
 * The caller should call pqueue_tag_free() when finished with the queue.
 * @return A dynamically allocated priority queue or NULL if memory allocation fails.
 */
pqueue_tag_t* pqueue_tag_init(size_t initial_size);

/**
 * Free all memory used by the queue including any elements that are marked is_dynamic.
 * @param q The queue.
 */
void pqueue_tag_free(pqueue_tag_t *q);

/**
 * Return the size of the queue.
 * @param q The queue.
 */
size_t pqueue_tag_size(pqueue_tag_t *q);

/**
 * @brief Insert a tag into the queue.
 * This automatically allocates memory for the element in the queue
 * and ensures that if the element is still on the queue when pqueue_tag_free
 * is called, that memory will be freed.
 * @param q The queue.
 * @param t The tag to insert.
 * @return 0 on success
 */
int pqueue_tag_insert_tag(pqueue_tag_t* q, tag_t t);

/**
 * @brief Insert a tag into the queue if the tag is not in the queue.
 * This automatically allocates memory for the element in the queue
 * and ensures that if the element is still on the queue when pqueue_tag_free
 * is called, that memory will be freed.
 * @param q The queue.
 * @param t The tag to insert.
 * @return 0 on success
 */
int pqueue_tag_insert_tag_if_not_present(pqueue_tag_t* q, tag_t t);

/**
 * @brief Pop the least-tag element from the queue and return its tag.
 * If the queue is empty, return FOREVER_TAG.
 * @param q The queue.
 * @return NULL on error, otherwise the entry
 */
tag_t pqueue_tag_pop_tag(pqueue_tag_t* q);

/**
 * Insert an element into the queue.
 * @param q The queue.
 * @param e The element to insert.
 * @return 0 on success
 */
int pqueue_tag_insert(pqueue_tag_t* q, pqueue_tag_element_t* d);

/**
 * @brief Pop the least-tag element from the queue.
 * If the entry was dynamically allocated, then it is now up to the caller
 * to ensure that it is freed. It will not be freed by pqueue_tag_free.
 * @param q The queue.
 * @return NULL on error, otherwise the entry
 */
pqueue_tag_element_t* pqueue_tag_pop(pqueue_tag_t* q);

/**
 * Find the highest-ranking item with the same tag.
 * @param q the queue
 * @param t the tag to compare against
 * @return NULL if no matching tag has been found, otherwise the entry
 */
pqueue_tag_element_t* pqueue_tag_find_same_tag(pqueue_tag_t *q, tag_t t);

/**
 * Remove an item from the queue.
 * @param q The queue.
 * @param e The entry to remove.
 * @return 0 on success
 */
int pqueue_tag_remove(pqueue_tag_t* q, pqueue_tag_element_t* e);

/**
 * Access highest-ranking item without removing it.
 * @param q The queue.
 * @return NULL on error, otherwise the entry.
 */
pqueue_tag_element_t* pqueue_tag_peek(pqueue_tag_t* q);

#endif // PQUEUE_TAG_H