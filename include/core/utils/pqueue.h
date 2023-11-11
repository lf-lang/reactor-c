/**
 * @file pqueue.h
 * @author Marten Lohstroh
 * @author Edward A. Lee
 * @copyright (c) 2020-2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * 
 * @brief Priority queue declarations for the event queue and reaction queue.
 */

#ifndef PQUEUE_H
#define PQUEUE_H

#include "pqueue_base.h"

/**
 * Return 1 if the first argument is greater than the second and zero otherwise.
 * @param thiz First argument.
 * @param that Second argument.
 */
int in_reverse_order(pqueue_pri_t thiz, pqueue_pri_t that);

/**
 * Return 0 regardless of argument order.
 * @param thiz First argument.
 * @param that Second argument.
 */
int in_no_particular_order(pqueue_pri_t thiz, pqueue_pri_t that);

/**
 * Return 1 if the two events have the same trigger.
 * @param event1 A pointer to an event_t.
 * @param event2 A pointer to an event_t.
 */
int event_matches(void* event1, void* event2);

/**
 * Return 1 if the two arguments are identical pointers.
 * @param a First argument.
 * @param b Second argument.
 */
int reaction_matches(void* a, void* b);

/**
 * Report a priority equal to the time of the given event.
 * This is used for sorting pointers to event_t structs in the event queue.
 * @param a A pointer to an event_t.
 */
pqueue_pri_t get_event_time(void *event);

/**
 * Report a priority equal to the index of the given reaction.
 * Used for sorting pointers to reaction_t structs in the
 * blocked and executing queues.
 * @param reaction A pointer to a reaction_t.
 */
pqueue_pri_t get_reaction_index(void *reaction_t);

/**
 * Return the given event's position in the queue.
 * @param event A pointer to an event_t.
 */
size_t get_event_position(void *event);

/**
 * Return the given reaction's position in the queue.
 * @param reaction A pointer to a reaction_t.
 */
size_t get_reaction_position(void *reaction);

/**
 * Set the given event's position in the queue.
 * @param event A pointer to an event_t
 * @param pos The position.
 */
void set_event_position(void *event, size_t pos);

/**
 * Set the given reaction's position in the queue.
 * @param event A pointer to a reaction_t.
 * @param pos The position.
 */
void set_reaction_position(void *reaction, size_t pos);

/**
 * Print some information about the given reaction.
 * This only prints something if logging is set to DEBUG.
 * @param reaction A pointer to a reaction_t.
 */
void print_reaction(void *reaction);

/**
 * Print some information about the given event.
 * This only prints something if logging is set to DEBUG.
 * @param event A pointer to an event_t.
 */
void print_event(void *event);

#endif /* PQUEUE_H */
