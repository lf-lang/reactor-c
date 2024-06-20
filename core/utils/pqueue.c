/**
 * @file pqueue.c
 * @author Marten Lohstroh
 * @author Edward A. Lee
 * @author Byeonggil Jun
 * @copyright (c) 2020-2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 *
 * @brief Priority queue definitions for queues where the priority is a number that can be compared with ordinary
 * numerical comparisons.
 *
 * This is used for the reaction queue. The event queue uses a `tag_t` struct for its priority, so it cannot use this.
 */

#include "low_level_platform.h"
#include "pqueue.h"
#include "util.h"
#include "lf_types.h"

int in_reverse_order(pqueue_pri_t thiz, pqueue_pri_t that) { return (thiz > that) ? 1 : (thiz < that) ? -1 : 0; }

int in_no_particular_order(pqueue_pri_t thiz, pqueue_pri_t that) {
  (void)thiz;
  (void)that;
  return 0;
}

int reaction_matches(void* a, void* b) { return (a == b); }

pqueue_pri_t get_reaction_index(void* reaction) { return ((reaction_t*)reaction)->index; }

size_t get_reaction_position(void* reaction) { return ((reaction_t*)reaction)->pos; }

void set_reaction_position(void* reaction, size_t pos) { ((reaction_t*)reaction)->pos = pos; }

void print_reaction(void* reaction) {
  reaction_t* r = (reaction_t*)reaction;
  LF_PRINT_DEBUG("%s: index: %llx, reaction: %p", r->name, r->index, reaction);
}
