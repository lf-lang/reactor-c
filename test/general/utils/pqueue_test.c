#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "pqueue_tag.h"
#include "tag.h"

static void trivial(void) {
    // Create an event queue.
    pqueue_tag_t* q = pqueue_tag_init(1);
    assert(q != NULL);
    assert(pqueue_is_valid((pqueue_t*)q));
    pqueue_print((pqueue_t*)q, NULL);
    pqueue_tag_free(q);
}

static void insert_on_queue(pqueue_tag_t* q) {
    tag_t t1 = {.time = USEC(3), .microstep = 0};
    tag_t t2 = {.time = USEC(2), .microstep = 1};
    tag_t t3 = {.time = USEC(2), .microstep = 0};
    tag_t t4 = {.time = USEC(1), .microstep = 2};
    assert(!pqueue_tag_insert_tag(q, t1));
    assert(!pqueue_tag_insert_tag(q, t2));
    assert(!pqueue_tag_insert_tag(q, t3));
    assert(!pqueue_tag_insert_tag(q, t4));
    printf("======== Contents of the queue:\n");
    pqueue_print((pqueue_t*)q, NULL);
    assert(pqueue_tag_size(q) == 4);
}

static void pop_from_queue(pqueue_tag_t* q) {
    tag_t t1_back = pqueue_tag_pop_tag(q);
    assert(t1_back.time == USEC(1));
    assert(t1_back.microstep == 2);
    tag_t t2_back = pqueue_tag_pop_tag(q);
    assert(t2_back.time == USEC(2));
    assert(t2_back.microstep == 0);
    tag_t t3_back = pqueue_tag_pop_tag(q);
    assert(t3_back.time == USEC(2));
    assert(t3_back.microstep == 1);
    tag_t t4_back = pqueue_tag_pop_tag(q);
    assert(t4_back.time == USEC(3));
    assert(t4_back.microstep == 0);
}

static void pop_empty(pqueue_tag_t* q) {
    assert(pqueue_tag_size(q) == 0);
    assert(pqueue_tag_pop(q) == NULL);
}

static void remove_from_queue(pqueue_tag_t* q, pqueue_tag_element_t* e1, pqueue_tag_element_t* e2) {
    assert(pqueue_tag_insert(q, e1) == 0);
    assert(pqueue_tag_insert(q, e2) == 0);
    assert(pqueue_tag_remove(q, e1) == 0);
    assert(pqueue_tag_peek(q) == e2);
    assert(pqueue_tag_size(q) == 1);
}

int main(int argc, char *argv[]) {
    trivial();
    // Create an event queue.
    pqueue_tag_t* q = pqueue_tag_init(2);

    insert_on_queue(q);
    pop_from_queue(q);
    pop_empty(q);

    pqueue_tag_element_t e1 = {.tag = {.time = USEC(3), .microstep = 0}, .pos = 0, .is_dynamic = 0};
    pqueue_tag_element_t e2 = {.tag = {.time = USEC(2), .microstep = 0}, .pos = 0, .is_dynamic = 0};

    remove_from_queue(q, &e1, &e2);

    pqueue_tag_free(q);
}
