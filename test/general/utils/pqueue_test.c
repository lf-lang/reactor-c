#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "pqueue_tag.h"
#include "tag.h"

static void trivial(void) {
    // Create an event queue.
    pqueue_t* q = pqueue_tag_init(1);
    assert(q != NULL);
}

int main(int argc, char *argv[]) {
    trivial();
}
