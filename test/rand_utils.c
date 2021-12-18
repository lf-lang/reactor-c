#include "rand_utils.h"
#include <stdlib.h>

/**
 * @brief Ensures that the expectation of each entry of `out` is equal
 * to the corresponding entry of `src`. Assumes that a random seed has
 * already been set using `srand`.
 * 
 * @param src An array of integers of size `size`.
 * @param size The size of both `src` and `out`.
 * @param out An array of integers of size `size`.
 */
void perturb(int* src, size_t size, int* out) {
    out[size - 1] = src[size - 1];
    for (int a = 0; a < size - 1; a += 2) {
        int min = src[a] < src[a + 1] ? src[a] : src[a + 1];
        int diff = rand() % (min * 2) - min;
        out[a] = src[a] + diff;
        out[a + 1] = src[a + 1] - diff;
    }
}
