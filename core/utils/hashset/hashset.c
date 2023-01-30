/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2012 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 * 
 * Modified in 2022 by Edward A. Lee (eal@berkeley.edu) so that stored items are
 * consistently of type void*. Note that the void* value 1 is used to mark a deleted
 * item and therefore cannot be stored in the hashset.
 * Also, used float rather than double for determining whether to rehash.
 */

#include "hashset/hashset.h"
#include <assert.h>

static const unsigned int prime_1 = 73;
static const unsigned int prime_2 = 5009;

hashset_t hashset_create(unsigned short nbits) {
    hashset_t set = calloc(1, sizeof(struct hashset_st));

    if (set == NULL) {
        return NULL;
    }
    set->nbits = nbits;
    set->capacity = (size_t)(1 << set->nbits);
    set->mask = set->capacity - 1;
    set->items = (void**)calloc(set->capacity, sizeof(void*));
    if (set->items == NULL) {
        hashset_destroy(set);
        return NULL;
    }
    set->nitems = 0;
    set->n_deleted_items = 0;
    return set;
}

size_t hashset_num_items(hashset_t set) {
    return set->nitems;
}

void hashset_destroy(hashset_t set) {
    if (set && set->items) {
        free(set->items);
    }
    free(set);
}

// FIXME
#include <stdio.h>

static int hashset_add_member(hashset_t set, void *item) {
    size_t ii;

    if (item == 0 || item == (void*)1) {
        return -1;
    }
    ii = set->mask & (prime_1 * (size_t)item);

    // Search chain of possible locations and stop when slot is empty.
    // Chain of possibilities always ends with an empty (0) even
    // if some items in the chain have been deleted (1). If a
    // deleted slot is found along the way, remember it to use it.
    int available = -1;
    while (set->items[ii] != 0) {
        if (set->items[ii] == item) {
            return 0;
        } else {
            if (set->items[ii] == (void*)1 && available < 0) {
                // Slot is available from deletion.
                available = (int)ii;
            }
            /* search the next slot */
            ii = set->mask & (ii + prime_2);
        }
    }
    set->nitems++;
    if (available >= 0) {
        // Use the slot available from a deletion.
        set->n_deleted_items--;
        set->items[available] = item;
    } else {
        set->items[ii] = item;
    }
    return 1;
}

static void maybe_rehash(hashset_t set) {
    void** old_items;
    size_t old_capacity, ii;

    if (set->nitems + set->n_deleted_items >= (float)set->capacity * 0.85f) {
        old_items = set->items;
        old_capacity = set->capacity;
        set->nbits++;
        set->capacity = (size_t)(1 << set->nbits);
        set->mask = set->capacity - 1;
        set->items = (void**)calloc(set->capacity, sizeof(void*));
        set->nitems = 0;
        set->n_deleted_items = 0;
        assert(set->items);
        for (ii = 0; ii < old_capacity; ii++) {
            hashset_add_member(set, old_items[ii]);
        }
        free(old_items);
    }
}

int hashset_add(hashset_t set, void *item) {
    int rv = hashset_add_member(set, item);
    maybe_rehash(set);
    return rv;
}

int hashset_remove(hashset_t set, void *item) {
    size_t ii = set->mask & (prime_1 * (size_t)item);

    while (set->items[ii] != 0) {
        if (set->items[ii] == item) {
            set->items[ii] = (void*)1;
            set->nitems--;
            set->n_deleted_items++;
            return 1;
        } else {
            ii = set->mask & (ii + prime_2);
        }
    }
    return 0;
}

int hashset_is_member(hashset_t set, void *item) {
    size_t ii = set->mask & (prime_1 * (size_t)item);

    while (set->items[ii] != 0) {
        if (set->items[ii] == item) {
            return 1;
        } else {
            ii = set->mask & (ii + prime_2);
        }
    }
    return 0;
}
