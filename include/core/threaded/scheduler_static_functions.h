/**
 * @brief Function type with a void* argument. To make this type represent a
 * generic function, one can write a wrapper function around the target function
 * and use the first argument as a pointer to a struct of input arguments
 * and return values.
 */
typedef void(*function_generic_t)(void*);

/**
 * @brief Wrapper function for peeking a priority queue.
 */
void push_pop_peek_pqueue(void* self);