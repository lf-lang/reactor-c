#include <stddef.h>

/**
 * @brief Ensures that the expectation of each entry of `out` is equal
 * to the corresponding entry of `src`. Assumes that a random seed has
 * already been set using `srand`.
 * 
 * @param src An array of integers of size `size`.
 * @param size The size of both `src` and `out`.
 * @param out An array of integers of size `size`.
 */
void perturb(int* src, size_t size, int* out);
