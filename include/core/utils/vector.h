/*
 * @file vector.h
 * @brief A minimal vector (resizing array) data type.
 * @ingroup Utilities
 *
 * This is intended to be the simplest way of storing a collection of
 * pointers that is frequently filled and then completely emptied.
 *
 * The corresponding `.c` files are in reactor-c/core/utils/vector
 *
 * @author Peter Donovan
 * @author Soroush Bateni
 */

#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdlib.h>

/**
 * @brief A vector (resizing array) data type.
 * @ingroup Utilities
 *
 * This struct implements a dynamic array that can grow as needed.
 * It is designed to be a simple way of storing a collection of pointers
 * that is frequently filled and then completely emptied.
 */
typedef struct vector_t {
  /**
   * @brief The start of the underlying array.
   *
   * Points to the beginning of the dynamically allocated array
   * that stores the vector's elements. This array contains pointers
   * to the actual elements stored in the vector.
   */
  void** start;

  /**
   * @brief The element after the last element in the underlying array.
   *
   * Points to the next available position in the array.
   * The invariant start <= next <= end is maintained.
   * The number of elements in the vector is (next - start).
   */
  void** next;

  /**
   * @brief The end of the underlying array.
   *
   * Points to one past the last allocated position in the array.
   * The total capacity of the vector is (end - start).
   * When next == end, the vector needs to be resized to add more elements.
   */
  void** end;

  /**
   * @brief The number of votes required to shrink this vector.
   *
   * This field is used in conjunction with votes to implement
   * a voting mechanism for vector shrinking. When the number of
   * votes reaches votes_required, the vector may be resized to
   * a smaller capacity.
   */
  int votes_required;

  /**
   * @brief The number of votes to shrink this vector.
   *
   * This counter is incremented when vector_shrink_vote() is called.
   * When it reaches votes_required, the vector may be resized to
   * a smaller capacity to free up memory.
   */
  int votes;
} vector_t;

/**
 * @brief Allocate and initialize a new vector.
 * @ingroup Utilities
 * @param initial_capacity The desired initial capacity to allocate.
 * Must be more than 0.
 * @return A new vector with the given initial capacity.
 */
vector_t vector_new(size_t initial_capacity);

/**
 * @brief Free the memory held by the given vector, invalidating it.
 * @ingroup Utilities
 * @param v Any vector.
 */
void vector_free(vector_t* v);

/**
 * @brief Add the given element to the vector.
 * @ingroup Utilities
 * The given element should be non-null.
 * @param v A vector that is to grow.
 * @param element An element that the vector should contain.
 */
void vector_push(vector_t* v, void* element);

/**
 * @brief Add all elements of the given array to the vector.
 * @ingroup Utilities
 * Elements should be non-null.
 * @param v A vector that is to grow.
 * @param array An array of items to be added to the vector.
 * @param size The size of the given array.
 */
void vector_pushall(vector_t* v, void** array, size_t size);

/**
 * @brief Remove and return some pointer that is contained in the given vector,
 * or return NULL if the given vector is empty.
 * @ingroup Utilities
 * @param v Any vector.
 */
void* vector_pop(vector_t* v);

/**
 * @brief Return a pointer to where the vector element at 'idx' is stored.
 * @ingroup Utilities
 * This can be used to set the value of the element or to read it.
 * If the index is past the end of the vector, then the vector
 * is automatically expanded and filled with NULL pointers as needed.
 * If no element at `idx` has been previously set, then the value
 * pointed to by the returned pointer will be NULL.
 *
 * @param v The vector.
 * @param idx The index into the vector.
 *
 * @return A pointer to the element at 'idx', which is itself a pointer.
 */
void** vector_at(vector_t* v, size_t idx);

/**
 * @brief Return the size of the vector.
 * @ingroup Utilities
 * @param v Any vector
 * @return size_t  The size of the vector.
 */
size_t vector_size(vector_t* v);

/**
 * @brief Vote on whether this vector should be given less memory.
 * @ingroup Utilities
 * If `v` contains few elements, it becomes more likely to shrink.
 * It is suggested that this function be called when the number of
 * elements in `v` reaches a local maximum.
 * @param v Any vector.
 */
void vector_vote(vector_t* v);

#endif /* VECTOR_H */
