/**
 * @file pqueue_reaction.h
 * @author Edward A. Lee
 * @copyright (c) 2024, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Priority queue that sorts reactions first by (inferred) deadline and then by level.
 *
 * This file extends the pqueue infrastructure with support for queues that are sorted
 * by reaction.  Elements in this queue are structs of type reaction_t.
 */

#ifndef PQUEUE_REACTION_H
#define PQUEUE_REACTION_H

#include "pqueue_base.h"
#include "lf_types.h"  // Defines reaction_t.
#include "tag.h"

/**
 * @brief Type of a priority queue sorted by reactions.
 */
typedef pqueue_t pqueue_reaction_t;

/**
 * @brief Callback comparison function for the reaction-based priority queue.
 * 
 * Return -1 if the first argument is less than second, 0 if the two arguments are the same,
 * and 1 otherwise.
 * This function is of type pqueue_cmp_pri_f.
 * @param priority1 A pointer to a reaction_t, cast to pqueue_pri_t.
 * @param priority2 A pointer to a reaction_t, cast to pqueue_pri_t.
 */
int pqueue_reaction_compare(pqueue_pri_t priority1, pqueue_pri_t priority2);

/**
 * @brief Create a priority queue sorted by reactions.
 *
 * The elements of the priority queue will be of type reaction_t.
 * The caller should call pqueue_reaction_free() when finished with the queue.
 * @return A dynamically allocated priority queue or NULL if memory allocation fails.
 */
pqueue_reaction_t* pqueue_reaction_init(size_t initial_size);

/**
 * @brief Free all memory used by the queue.
 * @param q The queue.
 */
void pqueue_reaction_free(pqueue_reaction_t* q);

/**
 * @brief Return the size of the queue.
 * @param q The queue.
 */
size_t pqueue_reaction_size(pqueue_reaction_t* q);

/**
 * @brief Insert a reaction into the queue.
 * 
 * This will not insert the reaction if it is already in the queue.
 * @param q The queue.
 * @param r The reaction to insert.
 * @return 0 on success
 */
int pqueue_reaction_insert(pqueue_reaction_t* q, reaction_t* r);

/**
 * @brief Return the head of the queue without removing it.
 * @param q The queue.
 * @return NULL on if the queue is empty, otherwise the first reaction.
 */
reaction_t* pqueue_reaction_peek(pqueue_reaction_t* q);

/**
 * @brief Pop the first reaction from the queue.
 * @param q The queue.
 * @return NULL on error, otherwise the reaction
 */
reaction_t* pqueue_reaction_pop(pqueue_reaction_t* q);

/**
 * Dump the queue and it's internal structure.
 * @internal
 * Debug function only.
 * @param q the queue
 */
void pqueue_reaction_dump(pqueue_reaction_t* q);

#endif // PQUEUE_REACTION_H
