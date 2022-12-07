#include <stddef.h>
#include <stdlib.h>
#include <setjmp.h>

typedef struct context_t {
    jmp_buf registers;
    int n;
    long *stack;
} context_t;

static void save_stack(context_t *c, long *top_of_stack, long *bottom_of_stack) {
    int n = top_of_stack-bottom_of_stack;
    c->stack = (long *) malloc(n*sizeof(long));
    c->n = n;
    for (int i = 0; i < n; i++) {
        c->stack[i] = top_of_stack[-i];
    }
}

context_t* context_save(long* top_of_stack) {
    context_t* ret = (context_t*) malloc(sizeof(context_t));
    long bottom_of_stack;
    if (!setjmp(ret->registers)) {
        save_stack(ret, top_of_stack, &bottom_of_stack);
        return ret;
    } else {
        return NULL; // setjmp was returned to by a longjmp call
    }
}

void context_switch(context_t* c, long* top_of_stack) {
    long mem_allocated_on_stack[12];
    long current_bottom_of_stack;
    if (top_of_stack - &current_bottom_of_stack < c->n) context_switch(c, top_of_stack);
    for (int i = 0; i < c->n; i++) {
        top_of_stack[-i] = c->stack[i];
    }
    longjmp(c->registers, 1);
}

void context_free(context_t* ctx) {
    free(ctx->stack);
    free(ctx);
}
