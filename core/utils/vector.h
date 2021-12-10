/*
 * This file defines a minimal vector (resizing array) data type.
 * It is intended to be the simplest way of storing a collection of
 * pointers that is frequently filled and then completely emptied.
 */

#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdlib.h>

typedef struct vector_t {
    int votes_to_shrink;  /* The number of votes required to shrink this vector. */
    void** start; /* The start of the underlying array. */
    void** next;  /* The element after the last element in the underlying array.
                        start <= next <= end. */
    void** end;   /* The end of the underlying array. */
} vector_t;

/**
 * Allocate and initialize a new vector.
 * @param initial_capacity The desired initial capacity to allocate.
 *  Must be more than 0
 */
vector_t vector_new(size_t initial_capacity);

/**
 * Free the memory held by the given vector, invalidating it.
 * @param v Any vector.
 */
void vector_free(vector_t* v);

/**
 * Add the given element to the vector. The given element should be
 * non-null.
 * @param v A vector that is to grow.
 * @param element An element that the vector should contain.
 */
void vector_push(vector_t* v, void* element);

/**
 * Add all elements of the given array to the vector. Elements should be
 * non-null.
 * @param v A vector that is to grow.
 * @param array An array of items to be added to the vector.
 * @param size The size of the given array.
 */
void vector_pushall(vector_t* v, void** array, size_t size);

/**
 * Remove and return some pointer that is contained in the given vector,
 * or return NULL if the given vector is empty.
 * @param v Any vector.
 */
void* vector_pop(vector_t* v);

/**
 * Vote on whether this vector ought to have a smaller memory footprint.
 */
void vector_vote(vector_t* v);

#endif /* VECTOR_H */

