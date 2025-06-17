/**
 * @file
 * @author Edward A. Lee
 *
 * @brief Functions for permuted mixed-radix numbers used in Lingua Franca programs.
 *
 * See @ref mixed_radix.h for docs.
 */

#include <assert.h>
#include <stdlib.h> // defines NULL

#include "mixed_radix.h"

/**
 * Increment the mixed radix number by one according to the permutation matrix.
 * @param mixed A pointer to the mixed-radix number.
 */
void mixed_radix_incr(mixed_radix_int_t* mixed) {
  int i = 0;
  assert(mixed != NULL);
  assert(mixed->size > 0);
  while (i < mixed->size) {
    int digit_to_increment = mixed->permutation[i];
    assert(digit_to_increment >= 0);
    mixed->digits[digit_to_increment]++;
    if (mixed->digits[digit_to_increment] >= mixed->radixes[digit_to_increment]) {
      mixed->digits[digit_to_increment] = 0;
      i++;
    } else {
      return; // All done.
    }
  }
  // If we get here, the number has overflowed. Wrap to zero.
  mixed->digits[i - 1] = 0;
}

/**
 * Return the int value of a mixed-radix number after dropping
 * the first n digits. If n is larger than or equal to the size
 * of the mixed-radix number, then return 0.
 * @param mixed A pointer to the mixed-radix number.
 * @param n The number of digits to drop, which is assumed to
 *  be greater than or equal to 0.
 */
int mixed_radix_parent(mixed_radix_int_t* mixed, int n) {
  assert(mixed != NULL);
  assert(mixed->size > 0);
  assert(n >= 0);
  int result = 0;
  int factor = 1;
  for (int i = n; i < mixed->size; i++) {
    result += factor * mixed->digits[i];
    factor *= mixed->radixes[i];
  }
  return result;
}

/**
 * Return the int value of a mixed-radix number.
 * @param mixed A pointer to the mixed-radix number.
 */
int mixed_radix_to_int(mixed_radix_int_t* mixed) { return mixed_radix_parent(mixed, 0); }
