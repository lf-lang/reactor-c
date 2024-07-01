/**
 * @file pqueue_tag.c
 * @author Byeonggil Jun
 * @author Edward A. Lee
 * @copyright (c) 2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 *
 * @brief Priority queue that uses tags for sorting.
 */

#include <stdlib.h>
#include <stdint.h>

#include "pqueue_tag.h"
#include "util.h"               // For lf_print
#include "low_level_platform.h" // For PRINTF_TAG

//////////////////
// Local functions, not intended for use outside this file.

/**
 * @brief Callback function to get the priority of an element.
 * Return the pointer argument cast to pqueue_pri_t because the
 * element is also the priority. This function is of type pqueue_get_pri_f.
 * @param element A pointer to a pqueue_tag_element_t, cast to void*.
 */
static pqueue_pri_t pqueue_tag_get_priority(void* element) {
  // Suppress "error: cast from pointer to integer of different size" by casting to uintptr_t first.
  return (pqueue_pri_t)(uintptr_t)element;
}

/**
 * @brief Callback function to determine whether two elements are equivalent.
 * Return 1 if the tags contained by given elements are identical, 0 otherwise.
 * This function is of type pqueue_eq_elem_f.
 * @param element1 A pointer to a pqueue_tag_element_t, cast to void*.
 * @param element2 A pointer to a pqueue_tag_element_t, cast to void*.
 */
static int pqueue_tag_matches(void* element1, void* element2) {
  return lf_tag_compare(((pqueue_tag_element_t*)element1)->tag, ((pqueue_tag_element_t*)element2)->tag) == 0;
}

/**
 * @brief Callback function to return the position of an element.
 * This function is of type pqueue_get_pos_f.
 * @param element A pointer to a pqueue_tag_element_t, cast to void*.
 */
static size_t pqueue_tag_get_position(void* element) { return ((pqueue_tag_element_t*)element)->pos; }

/**
 * @brief Callback function to set the position of an element.
 * This function is of type pqueue_set_pos_f.
 * @param element A pointer to a pqueue_tag_element_t, cast to void*.
 * @param pos The position.
 */
static void pqueue_tag_set_position(void* element, size_t pos) { ((pqueue_tag_element_t*)element)->pos = pos; }

/**
 * @brief Callback function to print information about an element.
 * This is a function of type pqueue_print_entry_f.
 * @param element A pointer to a pqueue_tag_element_t, cast to void*.
 */
static void pqueue_tag_print_element(void* element) {
  tag_t tag = ((pqueue_tag_element_t*)element)->tag;
  lf_print("Element with tag " PRINTF_TAG ".", tag.time, tag.microstep);
}

//////////////////
// Functions defined in pqueue_tag.h.

int pqueue_tag_compare(pqueue_pri_t priority1, pqueue_pri_t priority2) {
  // Suppress "error: cast from pointer to integer of different size" by casting to uintptr_t first.
  return (lf_tag_compare(((pqueue_tag_element_t*)(uintptr_t)priority1)->tag,
                         ((pqueue_tag_element_t*)(uintptr_t)priority2)->tag));
}

pqueue_tag_t* pqueue_tag_init(size_t initial_size) {
  return (pqueue_tag_t*)pqueue_init(initial_size, pqueue_tag_compare, pqueue_tag_get_priority, pqueue_tag_get_position,
                                    pqueue_tag_set_position, pqueue_tag_matches, pqueue_tag_print_element);
}

pqueue_tag_t* pqueue_tag_init_customize(size_t initial_size, pqueue_cmp_pri_f cmppri, pqueue_eq_elem_f eqelem,
                                        pqueue_print_entry_f prt) {
  return (pqueue_tag_t*)pqueue_init(initial_size, cmppri, pqueue_tag_get_priority, pqueue_tag_get_position,
                                    pqueue_tag_set_position, eqelem, prt);
}

void pqueue_tag_free(pqueue_tag_t* q) {
  for (size_t i = 1; i < q->size; i++) {
    if (q->d[i] != NULL && ((pqueue_tag_element_t*)q->d[i])->is_dynamic) {
      free(q->d[i]);
    }
  }
  pqueue_free((pqueue_t*)q);
}

size_t pqueue_tag_size(pqueue_tag_t* q) { return pqueue_size((pqueue_t*)q); }

int pqueue_tag_insert(pqueue_tag_t* q, pqueue_tag_element_t* d) { return pqueue_insert((pqueue_t*)q, (void*)d); }

int pqueue_tag_insert_tag(pqueue_tag_t* q, tag_t t) {
  pqueue_tag_element_t* d = (pqueue_tag_element_t*)malloc(sizeof(pqueue_tag_element_t));
  d->is_dynamic = 1;
  d->tag = t;
  return pqueue_tag_insert(q, d);
}

pqueue_tag_element_t* pqueue_tag_find_with_tag(pqueue_tag_t* q, tag_t t) {
  // Create an element on the stack. This element is only needed during
  // the duration of this function call, so putting it on the stack is OK.
  pqueue_tag_element_t element = {.tag = t, .pos = 0, .is_dynamic = false};
  return pqueue_find_same_priority((pqueue_t*)q, (void*)&element);
}

pqueue_tag_element_t* pqueue_tag_find_equal_same_tag(pqueue_tag_t* q, pqueue_tag_element_t* e) {
  return pqueue_find_equal_same_priority((pqueue_t*)q, (void*)e);
}

int pqueue_tag_insert_if_no_match(pqueue_tag_t* q, tag_t t) {
  if (pqueue_tag_find_with_tag(q, t) == NULL) {
    return pqueue_tag_insert_tag(q, t);
  } else {
    return 1;
  }
}

pqueue_tag_element_t* pqueue_tag_peek(pqueue_tag_t* q) { return (pqueue_tag_element_t*)pqueue_peek((pqueue_t*)q); }

tag_t pqueue_tag_peek_tag(pqueue_tag_t* q) {
  pqueue_tag_element_t* element = (pqueue_tag_element_t*)pqueue_tag_peek(q);
  if (element == NULL)
    return FOREVER_TAG;
  else
    return element->tag;
}

pqueue_tag_element_t* pqueue_tag_pop(pqueue_tag_t* q) { return (pqueue_tag_element_t*)pqueue_pop((pqueue_t*)q); }

tag_t pqueue_tag_pop_tag(pqueue_tag_t* q) {
  pqueue_tag_element_t* element = (pqueue_tag_element_t*)pqueue_tag_pop(q);
  if (element == NULL)
    return FOREVER_TAG;
  else {
    tag_t result = element->tag;
    if (element->is_dynamic)
      free(element);
    return result;
  }
}

void pqueue_tag_remove(pqueue_tag_t* q, pqueue_tag_element_t* e) { pqueue_remove((pqueue_t*)q, (void*)e); }

void pqueue_tag_remove_up_to(pqueue_tag_t* q, tag_t t) {
  tag_t head = pqueue_tag_peek_tag(q);
  while (lf_tag_compare(head, FOREVER_TAG) < 0 && lf_tag_compare(head, t) <= 0) {
    pqueue_tag_pop_tag(q);
    head = pqueue_tag_peek_tag(q);
  }
}

void pqueue_tag_dump(pqueue_tag_t* q) { pqueue_dump((pqueue_t*)q, pqueue_tag_print_element); }
