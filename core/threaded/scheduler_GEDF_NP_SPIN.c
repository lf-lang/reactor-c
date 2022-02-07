
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "scheduler.h"
#include "../utils/pqueue_support.h"
#include "scheduler_sync_tag_advance.c"
#include <assert.h>

#ifndef MAX_REACTION_LEVEL
#define MAX_REACTION_LEVEL INITIAL_REACT_QUEUE_SIZE
#endif

#define INITIALIZE_REACTION_Q pqueue_init( \
    INITIAL_REACT_QUEUE_SIZE, in_reverse_order, get_reaction_index, get_reaction_position, \
    set_reaction_position, reaction_matches, print_reaction \
)

extern lf_mutex_t mutex;

/**
 * @brief The reactions assigned to the various workers.
 */
static volatile reaction_t* worker_assignments[NUMBER_OF_WORKERS] = { NULL };
static volatile bool worker_working[NUMBER_OF_WORKERS] = { false };

/**
 * @brief The reactions triggered by each worker.
 */
static pqueue_t* triggered_reactions_by_worker;

/**
 * @brief Queue of triggered reactions.
 */
static pqueue_t reaction_q;

/**
 * @brief Indicate whether the program should stop.
 */
volatile bool should_stop = false;

/**
 * @brief The level of the reactions currently being processed.
 */
volatile size_t current_level = 0;

static bool still_initializing = true;

////////////////////// Scheduler Private Functions ///////////////////////////

static void enqueue_reactions_locked(int worker_number) {
    while (pqueue_size(triggered_reactions_by_worker + worker_number)) {
        reaction_t* next = pqueue_pop(triggered_reactions_by_worker + worker_number);
        assert(next->status == queued);
        pqueue_insert(&reaction_q, next);
    }
}

/**
 * @brief Advance tag if needed.
 */
static void advance_tag_if_needed_locked() {
    while (!pqueue_size(&reaction_q) && !pqueue_size(triggered_reactions_by_worker - 1)) {
        for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
            if (worker_working[i]) return;
        }
        if (_lf_sched_advance_tag_locked()) {
            assert(!pqueue_size(&reaction_q));
            DEBUG_PRINT("Scheduler: Reached stop tag.");
            should_stop = true;
            for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
                assert(!worker_assignments[i]);
                worker_assignments[i] = (volatile reaction_t*) 1;
            }
            return;
        }
    }
}

/**
 * @brief Distribute the currently triggered reactions to the workers.
 */
static void distribute_reactions_locked() {
    printf("%d ", pqueue_size(&reaction_q));
    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        printf("%d ", worker_working[i]);
    }
    printf("%d ", pqueue_size(&reaction_q));
    printf("\n");
    for (int i = 0; pqueue_size(&reaction_q) && i < NUMBER_OF_WORKERS; i++) {
        if (!worker_assignments[i]) {
            reaction_t* next = (reaction_t*) pqueue_pop(&reaction_q);
            if (next) {
                if (LEVEL(next->index) != current_level) {
                    for (int j = 0; j < NUMBER_OF_WORKERS; j++) {
                        if (worker_working[j]) {
                            pqueue_insert(&reaction_q, next);
                            return;
                        }
                    }
                    current_level = LEVEL(next->index);
                }
                assert(next->status != inactive);
                worker_assignments[i] = next;
                worker_working[i] = true;
            }
        }
    }
}

///////////////////// Scheduler Init and Destroy API /////////////////////////

/**
 * @brief Initialize the scheduler.
 * 
 * This has to be called before other functions of the scheduler can be used.
 * 
 * @param number_of_workers Indicate how many workers this scheduler will be managing.
 */
void lf_sched_init(size_t number_of_workers) {
    triggered_reactions_by_worker =
        (pqueue_t*) malloc(sizeof(pqueue_t) * (NUMBER_OF_WORKERS + 1)) + 1;
    for (int i = -1; i < NUMBER_OF_WORKERS; i++) {
        triggered_reactions_by_worker[i] = *INITIALIZE_REACTION_Q;
    }
    reaction_q = *INITIALIZE_REACTION_Q;
}

/**
 * @brief Free the memory used by the scheduler.
 * 
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        free(triggered_reactions_by_worker[i].d);
    }
    free(triggered_reactions_by_worker - 1);
    free(reaction_q.d);
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
reaction_t* busy_wait(int worker_number) {
    volatile reaction_t* ret;
    bool was_working = false;
    while (!(ret = worker_assignments[worker_number])) {
        assert(!was_working);
        was_working = worker_assignments[worker_number] != NULL;
    }
    // printf("%d gets.\n", worker_number);
    return (reaction_t*) ret;
}
#pragma GCC pop_options

void distribute_startup_reactions() {
    lf_mutex_lock(&mutex);
    enqueue_reactions_locked(-1);
    advance_tag_if_needed_locked();
    enqueue_reactions_locked(-1);
    distribute_reactions_locked();
    lf_mutex_unlock(&mutex);
}

///////////////////////// Scheduler Worker API ///////////////////////////////

/**
 * @brief Ask the scheduler for one more reaction.
 * 
 * This function blocks until it can return a ready reaction for worker thread
 * 'worker_number' or it is time for the worker thread to stop and exit (where a
 * NULL value would be returned).
 * 
 * @param worker_number 
 * @return reaction_t* A reaction for the worker to execute. NULL if the calling
 * worker thread should exit.
 */
reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    assert(worker_number >= 0);
    if (still_initializing) {
        distribute_startup_reactions();
        still_initializing = false;
    }
    reaction_t* ret = busy_wait(worker_number);
    if (should_stop) return NULL;
    assert(ret->status != inactive);
    return (reaction_t*) ret;
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 * 
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    assert(worker_number >= 0);
    assert(done_reaction->status != inactive);
    lf_mutex_lock(&mutex);
    done_reaction->status = inactive;
    enqueue_reactions_locked(worker_number);
    worker_working[worker_number] = false;
    worker_assignments[worker_number] = NULL;
#ifdef FEDERATED
    enqueue_reactions_locked(-1);
#endif FEDERATED
    advance_tag_if_needed_locked();
    enqueue_reactions_locked(-1);
    // printf("(%d) ", worker_number);
    distribute_reactions_locked();
    lf_mutex_unlock(&mutex);
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 * 
 * If a worker number is not available (e.g., this function is not called by a
 * worker thread), -1 should be passed as the 'worker_number'.
 * 
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 * 
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    assert(worker_number >= -1);
    if (lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        pqueue_insert(triggered_reactions_by_worker + worker_number, reaction);
    }
}
