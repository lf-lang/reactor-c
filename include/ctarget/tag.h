#ifndef CTARGET_TAG
#define CTARGET_TAG

#include "core/reactor.h"

/**
 * Compare two tags. Return -1 if the first is less than
 * the second, 0 if they are equal, and +1 if the first is
 * greater than the second. A tag is greater than another if
 * its time is greater or if its time is equal and its microstep
 * is greater.
 * @param tag1
 * @param tag2
 * @return -1, 0, or 1 depending on the relation.
 */
int lf_tag_compare(tag_t tag1, tag_t tag2);

/**
 * Return the current tag, a logical time, microstep pair.
 */
tag_t lf_tag();


#endif // CTARGET_TAG
