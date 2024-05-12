/**
 * @file pqueue_reaction.c
 * @author Edward A. Lee
 * @copyright (c) 2024, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 *
 * @brief Priority queue for reactions.
 */

#include <stdlib.h>

#include "pqueue_reaction.h"
#include "util.h"               // For lf_print
#include "low_level_platform.h" // For PRINTF_TAG

//////////////////
// Local functions, not intended for use outside this file.

/**
 * @brief Callback function to get the priority of an element.
 * 
 * This function is of type pqueue_get_pri_f.
 * @param element A pointer to a reaction_t, cast to void*.
 * @return The reaction, cast to pqueue_pri_t.
 */
static pqueue_pri_t pqueue_reaction_get_priority(void* element) { return (pqueue_pri_t)element; }

/**
 * @brief Callback function to determine whether two elements are equivalent.
 * 
 * Return 1 if the reactions are identical, 0 otherwise.
 * This function is of type pqueue_eq_elem_f.
 * @param element1 A pointer to a reaction_t, cast to void*.
 * @param element2 A pointer to a reaction_t, cast to void*.
 * @return 1 if the reactions are identical, 0 otherwise.
 */
static int pqueue_reaction_matches(void* element1, void* element2) {
  return (element1 == element2) ? 1 : 0;
}

/**
 * @brief Callback function to return the position of an element.
 * 
 * This function is of type pqueue_get_pos_f.
 * @param element A pointer to a reaction_t, cast to void*.
 * @return The position of the reaction in the queue.
 */
static size_t pqueue_reaction_get_position(void* element) { return ((reaction_t*)element)->pos; }

/**
 * @brief Callback function to set the position of an element.
 * 
 * This function is of type pqueue_set_pos_f.
 * @param element A pointer to a reaction_t, cast to void*.
 * @param pos The position.
 */
static void pqueue_reaction_set_position(void* element, size_t pos) { ((reaction_t*)element)->pos = pos; }

/**
 * @brief Callback function to print information about an element.
 * 
 * This is a function of type pqueue_print_entry_f.
 * It will print the name of the reaction if it is set and NULL otherwise.
 * @param element A pointer to a reaction_t, cast to void*.
 */
static void pqueue_reaction_print_element(void* element) {
  reaction_t* reaction = (reaction_t*)element;
  lf_print("Reaction %s", reaction->name);
}

//////////////////
// Functions defined in pqueue_reaction.h.

int pqueue_reaction_compare(pqueue_pri_t priority1, pqueue_pri_t priority2) {
  // FIXME: For now, just use the index. Replace with inferred deadline and level.
  index_t index1 = ((reaction_t*)priority1)->index;
  index_t index2 = ((reaction_t*)priority2)->index;
  if (index1 < index2) {
    return -1;
  } else if (index1 > index2) {
    return 1;
  } else {
    return 0;
  }
}

pqueue_tag_t* pqueue_reaction_init(size_t initial_size) {
  return (pqueue_reaction_t*)pqueue_init(
      initial_size,
      pqueue_reaction_compare,
      pqueue_reaction_get_priority,
      pqueue_reaction_get_position,
      pqueue_reaction_set_position,
      pqueue_reaction_matches,
      pqueue_reaction_print_element);
}

void pqueue_reaction_free(pqueue_reaction_t* q) {
  // Do not free reactions.
  pqueue_free((pqueue_t*)q);
}

size_t pqueue_reaction_size(pqueue_reaction_t* q) {
  return pqueue_size((pqueue_t*)q);
}

int pqueue_reaction_insert(pqueue_reaction_t* q, reaction_t* r) {
  return pqueue_insert((pqueue_t*)q, (void*)r);
}

reaction_t* pqueue_reaction_peek(pqueue_reaction_t* q) {
  return (reaction_t*)pqueue_peek((pqueue_t*)q);
}

reaction_t* pqueue_reaction_pop(pqueue_reaction_t* q) {
  return (reaction_t*)pqueue_pop((pqueue_t*)q);
}

void pqueue_reaction_dump(pqueue_tag_t* q) {
  pqueue_dump((pqueue_t*)q, pqueue_reaction_print_element);
}
