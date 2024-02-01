/**
 * @file pqueue.c
 * @author Marten Lohstroh
 * @author Edward A. Lee
 * @copyright (c) 2020-2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * 
 * @brief Priority queue definitions for the event queue and reaction queue.
 */

#include "platform.h"
#include "pqueue.h"
#include "util.h"
#include "lf_types.h"


int in_reverse_order(pqueue_pri_t thiz, pqueue_pri_t that) {
    return (thiz > that);
}

int in_no_particular_order(pqueue_pri_t thiz, pqueue_pri_t that) {
    return 0;
}

int event_matches(void* event1, void* event2) {
    return (((event_t*)event1)->trigger == ((event_t*)event2)->trigger);
}

int reaction_matches(void* a, void* b) {
    return (a == b);
}

pqueue_pri_t get_event_time(void *event) {
    return (pqueue_pri_t)(((event_t*) event)->time);
}

pqueue_pri_t get_reaction_index(void *reaction) {
    return ((reaction_t*) reaction)->index;
}

size_t get_event_position(void *event) {
    return ((event_t*) event)->pos;
}

size_t get_reaction_position(void *reaction) {
    return ((reaction_t*) reaction)->pos;
}

void set_event_position(void *event, size_t pos) {
    ((event_t*) event)->pos = pos;
}

void set_reaction_position(void *reaction, size_t pos) {
    ((reaction_t*) reaction)->pos = pos;
}

void print_reaction(void *reaction) {
    reaction_t *r = (reaction_t*)reaction;
    LF_PRINT_DEBUG("%s: chain_id: %llu, index: %llx, reaction: %p",
            r->name, r->chain_id, r->index, r);
}

void print_event(void *event) {
    event_t *e = (event_t*)event;
    LF_PRINT_DEBUG("time: " PRINTF_TIME ", trigger: %p, token: %p",
            e->time, e->trigger, e->token);
}
