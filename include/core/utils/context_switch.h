typedef struct context_t context_t;
context_t* context_save(long* top_of_stack);
void context_switch(context_t* c, long* top_of_stack);
void context_free(context_t* c);
