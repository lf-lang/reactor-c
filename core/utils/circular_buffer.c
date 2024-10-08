/**
 * @file circular_buffer.c
 * @brief A circular buffer implementation from stack overflow
 * (https://stackoverflow.com/questions/827691/how-do-you-implement-a-circular-buffer-in-c) 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "util.h"
#include "circular_buffer.h"
#include "lf_types.h"

void cb_init(circular_buffer *cb, size_t capacity, size_t sz)
{
    cb->buffer = malloc(capacity * sz);
    if(cb->buffer == NULL) {
        // handle error
        lf_print("ERROR: Fail to allocate memory to circular buffer.");
        return;
    }
    cb->buffer_end = (char *)cb->buffer + capacity * sz;
    cb->capacity = capacity;
    cb->count = 0;
    cb->sz = sz;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
}

void cb_free(circular_buffer *cb)
{
    free(cb->buffer);
    // clear out other fields too, just to be safe
}

void cb_push_back(circular_buffer *cb, const void *item)
{
    if(cb->count == cb->capacity){
        lf_print("ERROR: Buffer is full. Some in-flight events will be overwritten!");
    }
    memcpy(cb->head, item, cb->sz);
    cb->head = (char*)cb->head + cb->sz;
    if(cb->head == cb->buffer_end)
        cb->head = cb->buffer;
    cb->count++;
}

void cb_pop_front(circular_buffer *cb, void *item)
{
    if(cb->count == 0){
        // handle error
        lf_print("ERROR: Popping from an empty buffer!");
        return;
    }
    memcpy(item, cb->tail, cb->sz);
    cb->tail = (char*)cb->tail + cb->sz;
    if(cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
}

void cb_remove_front(circular_buffer *cb)
{
    if(cb->count == 0){
        // handle error
        lf_print("ERROR: Removing from an empty buffer!");
        return;
    }
    cb->tail = (char*)cb->tail + cb->sz;
    if(cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
}

void* cb_peek(circular_buffer *cb)
{
    if(cb->count == 0)
        return NULL;
    return cb->tail;
}

void cb_dump_events(circular_buffer *cb)
{
    lf_print("*** Dumping Events ***");
    void *p = cb->tail;
    while (p != cb->head) {
        event_t* e = (event_t*)p;
        lf_print("Event @ %lld w/ token %p", e->base.tag.time, e->token);
        p += cb->sz;
        if (p == cb->buffer_end) p = cb->buffer;
    }
    lf_print("**********************");
}