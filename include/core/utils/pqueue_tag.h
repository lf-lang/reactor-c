/**
 * @file pqueue_tag.h
 * @author Byeonggil Jun
 * @author Edward A. Lee
 *
 * @brief Priority queue that uses tags for sorting.
 * @ingroup Internal
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
 * field of the element to a non-zero value.
 *
 * For a priority queue that contains only tags with no payload, you can
 * avoid creating the element struct by using the functions
 * pqueue_tag_insert_tag, pqueue_tag_insert_if_no_match, and pqueue_tag_pop_tag.
 *
 * To customize the element you put onto the queue, for example to carry
 * a payload, you can create your own element struct type by simply declaring
 * the first field to be a pqueue_tag_element_t.  For example, if you want an
 * element of the queue to include a pointer to your own payload, you can
 * declare the following struct type:
 *
 * ```c
 *     typedef struct {
 *         pqueue_tag_element_t base;
 *         my_type* my_payload;
 *     } my_element_type_t;
 * ```
 * When inserting your struct into the queue, simply cast your pointer
 * to (pqueue_tag_element_t*).  When accessing your struct from the queue,
 * simply cast the result to (my_element_type_t*);
 */
typedef struct {
  /**
   * @brief The tag that determines the element's priority in the queue.
   *
   * The queue is sorted from least tag to largest tag.
   * This field is used as the actual priority value when the struct
   * pointer is cast to pqueue_pri_t.
   */
  tag_t tag;

  /**
   * @brief Current position of this element in the priority queue.
   *
   * This field is required by the priority queue implementation
   * to track the element's position in the heap structure.
   * It is maintained by the queue operations and should not be
   * modified directly.
   */
  size_t pos;

  /**
   * @brief Flag indicating whether this element should be freed with the queue.
   *
   * If non-zero, the element's memory will be freed when pqueue_tag_free
   * is called on the queue. This is useful for dynamically allocated
   * elements that should be automatically cleaned up.
   * Set this to non-zero if the element was allocated with malloc or calloc
   * and should be freed when the queue is freed.
   */
  int is_dynamic;
} pqueue_tag_element_t;

/**
 * @brief Type of a priority queue sorted by tags.
 * @ingroup Internal
 */
typedef pqueue_t pqueue_tag_t;

/**
 * @brief Callback comparison function for the tag-based priority queue.
 * @ingroup Internal
 * @param priority1 A pointer to a pqueue_tag_element_t, cast to pqueue_pri_t.
 * @param priority2 A pointer to a pqueue_tag_element_t, cast to pqueue_pri_t.
 * @return -1 if the first argument is less than second, 0 if the two arguments are the same,
 * and 1 otherwise.
 */
int pqueue_tag_compare(pqueue_pri_t priority1, pqueue_pri_t priority2);

/**
 * @brief Create a priority queue sorted by tags.
 * @ingroup Internal
 * The elements of the priority queue will be of type pqueue_tag_element_t.
 * The caller should call pqueue_tag_free() when finished with the queue.
 * @param initial_size The initial size of the priority queue.
 * @return A dynamically allocated priority queue or NULL if memory allocation fails.
 */
pqueue_tag_t* pqueue_tag_init(size_t initial_size);

/**
 * @brief Create a priority queue that stores elements with a particular payload.
 * @ingroup Internal
 *
 * The elements of the priority queue will be of type pqueue_tag_element_t.
 * The caller should call @ref pqueue_tag_free() when finished with the queue.
 *
 * @param initial_size The initial size of the priority queue.
 * @param cmppri The callback function to compare priorities.
 * @param eqelem The callback function to check equivalence of payloads.
 * @param prt The callback function to print elements.
 *
 * @return A dynamically allocated priority queue or NULL if memory allocation fails.
 */
pqueue_tag_t* pqueue_tag_init_customize(size_t initial_size, pqueue_cmp_pri_f cmppri, pqueue_eq_elem_f eqelem,
                                        pqueue_print_entry_f prt);

/**
 * @brief Free all memory used by the queue including elements that are marked dynamic.
 * @ingroup Internal
 * @param q The queue.
 */
void pqueue_tag_free(pqueue_tag_t* q);

/**
 * @brief Return the size of the queue.
 * @ingroup Internal
 * @param q The queue.
 */
size_t pqueue_tag_size(pqueue_tag_t* q);

/**
 * @brief Insert an element into the queue.
 * @ingroup Internal
 * @param q The queue.
 * @param d The element to insert.
 * @return 0 on success
 */
int pqueue_tag_insert(pqueue_tag_t* q, pqueue_tag_element_t* d);

/**
 * @brief Insert a tag into the queue.
 * @ingroup Internal
 * This automatically creates a dynamically allocated element in the queue
 * and ensures that if the element is still on the queue when pqueue_tag_free
 * is called, then that memory will be freed.
 * @param q The queue.
 * @param t The tag to insert.
 * @return 0 on success
 */
int pqueue_tag_insert_tag(pqueue_tag_t* q, tag_t t);

/**
 * @brief Insert a tag into the queue if the tag is not already in the queue.
 * @ingroup Internal
 * This automatically creates a dynamically allocated element in the queue
 * and ensures that if the element is still on the queue when pqueue_tag_free
 * is called, then that memory will be freed.
 * @param q The queue.
 * @param t The tag to insert.
 * @return 0 on success, 1 otherwise.
 */
int pqueue_tag_insert_if_no_match(pqueue_tag_t* q, tag_t t);

/**
 * @brief Return the first item with the specified tag or NULL if there is none.
 * @ingroup Internal
 * @param q The queue.
 * @param t The tag.
 * @return An entry with the specified tag or NULL if there isn't one.
 */
pqueue_tag_element_t* pqueue_tag_find_with_tag(pqueue_tag_t* q, tag_t t);

/**
 * @brief Return an item with the same tag (`cmppri` returns 0) that matches the supplied element
 * (`eqelem` returns non-zero) or NULL if there is none.
 * @ingroup Internal
 * @param q The queue.
 * @param e The element.
 * @return An entry with the specified tag or NULL if there isn't one.
 */
pqueue_tag_element_t* pqueue_tag_find_equal_same_tag(pqueue_tag_t* q, pqueue_tag_element_t* e);

/**
 * @brief Return highest-ranking item (the one with the least tag) without removing it.
 * @ingroup Internal
 * @param q The queue.
 * @return NULL on if the queue is empty, otherwise the entry.
 */
pqueue_tag_element_t* pqueue_tag_peek(pqueue_tag_t* q);

/**
 * @brief Return the least tag in the queue or FOREVER if the queue is empty.
 * @ingroup Internal
 * @param q The queue.
 * @return The least tag in the queue or FOREVER if the queue is empty.
 */
tag_t pqueue_tag_peek_tag(pqueue_tag_t* q);

/**
 * @brief Pop the least-tag element from the queue.
 * @ingroup Internal
 * If the entry was dynamically allocated, then it is now up to the caller
 * to ensure that it is freed. It will not be freed by pqueue_tag_free.
 * @param q The queue.
 * @return NULL on error, otherwise the entry
 */
pqueue_tag_element_t* pqueue_tag_pop(pqueue_tag_t* q);

/**
 * @brief Pop the least-tag element from the queue and return its tag.
 * @ingroup Internal
 * If the queue is empty, return FOREVER_TAG. This function handles freeing
 * the element struct if it was dynamically allocated.
 * @param q The queue.
 * @return NULL on error, otherwise the entry
 */
tag_t pqueue_tag_pop_tag(pqueue_tag_t* q);

/**
 * @brief Remove an item from the queue.
 * @ingroup Internal
 * @param q The queue.
 * @param e The entry to remove.
 */
void pqueue_tag_remove(pqueue_tag_t* q, pqueue_tag_element_t* e);

/**
 * @brief Remove items from the queue with tags up to and including the specified tag.
 * @ingroup Internal
 * If the specified tag is FOREVER_TAG, then all items will be removed.
 * @param q The queue.
 * @param t The specified tag.
 */
void pqueue_tag_remove_up_to(pqueue_tag_t* q, tag_t t);

/**
 * Dump the queue and it's internal structure.
 * @param q the queue
 */
void pqueue_tag_dump(pqueue_tag_t* q);

#endif // PQUEUE_TAG_H
