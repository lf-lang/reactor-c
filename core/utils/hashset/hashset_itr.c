/*
 * hashset_itr.c
 *
 *   Copyright 2012 Couchbase, Inc.
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
 * In Nov. 2022, Edward A. Lee fixed bug where next could advance to a removed item.
 * Also, changed the logic of the API a bit as inidicated in the .h file.
 */

#include <assert.h>
#include <stdio.h>
#include "hashset/hashset_itr.h"

hashset_itr_t hashset_iterator(hashset_t set) {
  hashset_itr_t itr = calloc(1, sizeof(struct hashset_itr_st));
  if (itr == NULL) {
    return NULL;
  }
  itr->set = set;
  itr->index = -1;

  return itr;
}

int hashset_iterator_has_next(hashset_itr_t itr) {
  assert(itr != NULL);
  size_t index = itr->index + 1;

  /* empty or end of the set */
  if (itr->set->nitems == 0 || index == itr->set->capacity) {
    return 0;
  }
  /* peek to find another entry */
  while(index < itr->set->capacity) {
    void* value = itr->set->items[index++];
    if(value != 0 && value != (void*)1)
      return 1;
  }

  /* Otherwise */
  return 0;
}

int hashset_iterator_next(hashset_itr_t itr) {
  assert(itr != NULL);

  size_t index = itr->index + 1;

  /* empty or end of the set */
  if (itr->set->nitems == 0 || index == itr->set->capacity) {
    return -1;
  }

  while (index < itr->set->capacity) {
    if (itr->set->items[index] != 0 && itr->set->items[index] != (void*)1) {
      // Found one.
      itr->index = (int)index;
      return index;
    }
    index++;
  }

  return -1;
}

void* hashset_iterator_value(hashset_itr_t itr) {

  // Check that hashset_iterator_next() has been called.
  assert(itr->index >= 0 && itr->set->items[itr->index] != 0 && itr->set->items[itr->index] != (void*)1);

  return itr->set->items[itr->index];
}

