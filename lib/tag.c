#include "ctarget/tag.h"

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
int lf_tag_compare(tag_t tag1, tag_t tag2) {
    return _lf_tag_compare(tag1, tag2);
}

/**
 * @deprecated version of `lf_tag_compare`
 */
int compare_tags(tag_t tag1, tag_t tag2) {
    return lf_tag_compare(tag1, tag2);
}

/**
 * Return the current tag, a logical time, microstep pair.
 */
tag_t lf_tag() {
    return _lf_tag();
}

/**
 * @deprecated version of `lf_tag`
 */
tag_t get_current_tag(void) {
    return lf_tag();
}

/**
 * Return the current microstep.
 * @deprecated
 */
microstep_t get_microstep() {
    return _lf_tag().microstep;
}