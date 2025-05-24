/**
@file
@author Arthur Deng
@author Edward A. Lee

@brief Implementation of a double-ended queue.

This is the header file for an implementation of a double-ended queue.
Each node in the queue contains a void* pointer.

To use this, include the following in your target properties:
<pre>
target C {
    cmake-include: "/lib/c/reactor-c/util/deque.cmake"
    files: ["/lib/c/reactor-c/util/deque.c", "/lib/c/reactor-c/util/deque.h"]
};
</pre>
In addition, you need this in your Lingua Franca file:
<pre>
preamble {=
    #include "deque.h"
=}
</pre>
To create a deque, use calloc to ensure that it gets initialized
with null pointers and zero size:
<pre>
    deque_t* my_deque = (deque_t*) calloc(1, sizeof(deque_t));
</pre>
Alternatively, you can call initialize:
<pre>
    deque my_deque;
    deque_initialize(&my_deque);
</pre>
*/

#ifndef DEQUE_H
#define DEQUE_H

#include <stddef.h>  // Defines size_t
#include <stdbool.h> // Defines bool
#include <stdlib.h>  // Defines malloc and free

/**
 * A double-ended queue data structure.
 */
typedef struct deque_t {
  struct deque_node_t* front;
  struct deque_node_t* back;
  size_t size;
} deque_t;

/**
 * Initialize the specified deque to an empty deque.
 * @param d The deque.
 */
void deque_initialize(deque_t* d);

/**
 * Return true if the queue is empty.
 * @param d The deque.
 */
bool deque_is_empty(deque_t* d);

/**
 * Return the size of the queue.
 * @param d The deque.
 * @return The size of the queue.
 */
size_t deque_size(deque_t* d);

/**
 * Push a value to the front of the queue.
 * @param d The queue.
 * @param value The value to push.
 */
void deque_push_front(deque_t* d, void* value);

/**
 * Push a value to the back of the queue.
 * @param d The queue.
 * @param value The value to push.
 */
void deque_push_back(deque_t* d, void* value);

/**
 * Pop a value from the front of the queue, removing it from the queue.
 * @param d The queue.
 * @return The value on the front of the queue or NULL if the queue is empty.
 */
void* deque_pop_front(deque_t* d);

/**
 * Pop a value from the back of the queue, removing it from the queue.
 * @param d The queue.
 * @return The value on the back of the queue or NULL if the queue is empty.
 */
void* deque_pop_back(deque_t* d);

/**
 * Peek at the value on the front of the queue, leaving it on the queue.
 * @param d The queue.
 * @return The value on the front of the queue or NULL if the queue is empty.
 */
void* deque_peek_back(deque_t* d);

/**
 * Peek at the value on the back of the queue, leaving it on the queue.
 * @param d The queue.
 * @return The value on the back of the queue or NULL if the queue is empty.
 */
void* deque_peek_front(deque_t* d);

#endif // DEQUE_H
