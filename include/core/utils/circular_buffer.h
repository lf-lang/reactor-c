#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdlib.h>
#include <string.h>

typedef struct circular_buffer
{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *tail;       // pointer to tail
} circular_buffer;

void cb_init(circular_buffer *cb, size_t capacity, size_t sz);
void cb_free(circular_buffer *cb);
void cb_push_back(circular_buffer *cb, const void *item);
void cb_pop_front(circular_buffer *cb, void *item);
void cb_remove_front(circular_buffer *cb);
void* cb_peek(circular_buffer *cb);
void cb_dump_events(circular_buffer *cb);

#endif