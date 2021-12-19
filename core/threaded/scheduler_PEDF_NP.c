/* Partitioned Earliest Deadline First (PEDF) non-preemptive scheduler for the
threaded runtime of the C target of Lingua Franca. */

/*************
Copyright (c) 2021, The University of Texas at Dallas.
Copyright (c) 2019, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/** 
 * Partitioned Earliest Deadline First (GEDF) non-preemptive for the threaded
 * runtime of the C target of Lingua Franca.
 *  
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "scheduler.h"
#include "../platform.h"
#include "../utils/pqueue_support.h"
#include "../utils/semaphore.c"
#include "../utils/vector.c"


/////////////////// External Variables /////////////////////////
extern pqueue_t* reaction_q;
extern lf_mutex_t mutex;
extern tag_t current_tag;
extern tag_t stop_tag;

/////////////////// External Functions /////////////////////////
/**
 * Return whether the first and second argument are given in reverse order.
 */
static int in_reverse_order(pqueue_pri_t thiz, pqueue_pri_t that);

/**
 * Report a priority equal to the index of the given reaction.
 * Used for sorting pointers to reaction_t structs in the 
 * blocked and executing queues.
 */
static pqueue_pri_t get_reaction_index(void *a);

/**
 * Return the given reaction's position in the queue.
 */
static size_t get_reaction_position(void *a);

/**
 * Return the given reaction's position in the queue.
 */
static void set_reaction_position(void *a, size_t pos);

/**
 * Print some information about the given reaction.
 * 
 * DEBUG function only.
 */
static void print_reaction(void *reaction);

/**
 * Return whether or not the given reaction_t pointers 
 * point to the same struct.
 */
static int reaction_matches(void* next, void* curr);

/**
 * If there is at least one event in the event queue, then wait until
 * physical time matches or exceeds the time of the least tag on the event
 * queue; pop the next event(s) from the event queue that all have the same tag;
 * extract from those events the reactions that are to be invoked at this
 * logical time and insert them into the reaction queue. The event queue is
 * sorted by time tag.
 *
 * If there is no event in the queue and the keepalive command-line option was
 * not given, and this is not a federated execution with centralized coordination,
 * set the stop tag to the current tag.
 * If keepalive was given, then wait for either request_stop()
 * to be called or an event appears in the event queue and then return.
 *
 * Every time tag is advanced, it is checked against stop tag and if they are
 * equal, shutdown reactions are triggered.
 *
 * This does not acquire the mutex lock. It assumes the lock is already held.
 */
void _lf_next_locked();

/** 
 * Placeholder for code-generated function that will, in a federated
 * execution, be used to coordinate the advancement of tag. It will notify
 * the runtime infrastructure (RTI) that all reactions at the specified
 * logical tag have completed. This function should be called only while
 * holding the mutex lock.
 * @param tag_to_send The tag to send.
 */
void logical_tag_complete(tag_t tag_to_send);

/////////////////// Scheduler Variables and Structs /////////////////////////
/**
 * @brief Atomically keep track of how many worker threads are idle.
 *
 * Initially assumed that there are 0 idle threads.
 */
semaphore_t* _lf_sched_semaphore; 

/**
 * @brief Vector used to keep reactions temporarily.
 * 
 */
vector_t transfer_q;

/**
 * @brief Queue of currently executing reactions.
 * 
 * Sorted by index (precedence sort)
 */
pqueue_t* executing_q;

/**
 * @brief Information about one worker thread.
 * 
 * Only reading and writing the 'is_idle' field strictly requires acquiring the
 * 'mutex' in this struct.
 */
typedef struct {
    lf_mutex_t mutex;           // Used by the scheduler to access is_idle
    
    lf_cond_t cond;             // Used by the scheduler to inform a
                                // worker thread that there is more work to do.
    
    pqueue_t* ready_reactions;  // Reactions that are ready to be executed by 
                                // the worker thread. The worker thread does not
                                // need to acquire any mutex lock to read this
                                // and the scheduler does not need to acquire
                                // any mutex lock to write to this as long as
                                // the worker thread is idle.
    
    vector_t output_reactions;  // Reactions produced by the worker after 
                                // executing a reaction. The worker thread does
                                // not need to acquire any mutex lock to read
                                // this and the scheduler does not need to
                                // acquire any mutex lock to write to this as
                                // long as the worker thread is idle.
    
    vector_t done_reactions;    // Reactions that are ran to completion by the 
                                // worker thread. The worker thread does not
                                // need to acquire any mutex lock to read this
                                // and the scheduler does not need to acquire
                                // any mutex lock to write to this as long as
                                // the worker thread is idle.
    
    bool should_stop;           // Indicate to the worker thread that it should exit.
    
    size_t is_idle;             // Indicate to the scheduler that the worker thread 
                                // is idle (0 = busy, > 0 idle). This is the
                                // only attribute of this struct that requires
                                // the mutex lock to be held, both while reading
                                // and writing to it to avoid race condition.
} _lf_sched_thread_info_t;

/**
 * @brief Information about worker threads. @see _lf_sched_thread_info_t.
 * 
 */
_lf_sched_thread_info_t* _lf_sched_threads_info;

/**
 * @brief Number of workers that this scheduler is managing.
 * 
 */
size_t _lf_sched_number_of_workers = 1;

/**
 * @brief Index used by the scheduler to balance the distribution of reactions
 * to worker threads.
 * 
 * The maximum of _lf_sched_balancing_index and reaction->worker_affinity is chosen by
 * the scheduler as the starting point in the quest to find the next available
 * worker to assign a reaction to. During the work distribution phase, this
 * index is updated to make sure the scheduler does not assign work to the same
 * worker thread two times in a row. After the work distribution phase, this
 * index is reset to 0.
 * 
 */
int _lf_sched_balancing_index = 0;

/**
 * @brief Indicator that execution of at least one tag has completed.
 */
bool _lf_logical_tag_completed = false;


/**
 * @brief Thread handler for the scheduler
 * 
 */
lf_thread_t _lf_sched_thread;

/**
 * @brief The main thread of the scheduler, to be created by the main program.
 * 
 * @param arg Ignored.
 * @return void* NULL on exit.
 */
void* _lf_sched_scheduling_thread(void* arg);

///////////////////// Scheduler Init and Destroy API /////////////////////////

/**
 * @brief Initialize the scheduler.
 * 
 * This has to be called before the main thread of the scheduler is created.
 * 
 * @param number_of_workers Indicate how many workers this scheduler will be managing.
 */
void lf_sched_init(size_t number_of_workers) {
    DEBUG_PRINT("Scheduler: Initializing with %d workers", number_of_workers);
    
    _lf_sched_semaphore = lf_semaphore_new(0);
    _lf_sched_number_of_workers = number_of_workers;
    transfer_q = vector_new(INITIAL_REACT_QUEUE_SIZE);
    // Create a queue on which to put reactions that are currently executing.
    executing_q = pqueue_init(_lf_number_of_threads, in_reverse_order, get_reaction_index,
        get_reaction_position, set_reaction_position, reaction_matches, print_reaction);
    
    _lf_sched_threads_info = 
        (_lf_sched_thread_info_t*)malloc(
            sizeof(_lf_sched_thread_info_t) * _lf_sched_number_of_workers);
    
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        lf_cond_init(&_lf_sched_threads_info[i].cond);
        lf_mutex_init(&_lf_sched_threads_info[i].mutex);
        _lf_sched_threads_info[i].ready_reactions = 
            pqueue_init(
                INITIAL_REACT_QUEUE_SIZE, 
                in_reverse_order, 
                get_reaction_index,
                get_reaction_position, 
                set_reaction_position, 
                reaction_matches, 
                print_reaction
            );
        _lf_sched_threads_info[i].output_reactions = 
            vector_new(INITIAL_REACT_QUEUE_SIZE);
        _lf_sched_threads_info[i].done_reactions = 
            vector_new(INITIAL_REACT_QUEUE_SIZE);
        _lf_sched_threads_info[i].should_stop = false;
        _lf_sched_threads_info[i].is_idle = 0;
    }

    lf_thread_create(&_lf_sched_thread, _lf_sched_scheduling_thread, NULL);
}

/**
 * @brief Free the memory used by the scheduler.
 * 
 * This must be called after the main scheduler thread exits.
 * 
 */
void lf_sched_free() {
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        pqueue_free(_lf_sched_threads_info[i].ready_reactions);
        vector_free(&_lf_sched_threads_info[i].output_reactions);
        vector_free(&_lf_sched_threads_info[i].done_reactions);
    }
    vector_free(&transfer_q);
    pqueue_free(executing_q);
    if (lf_semaphore_destroy(_lf_sched_semaphore) != 0) {
        error_print_and_exit("Scheduler: Could not destroy my semaphore.");
    }
    free(_lf_sched_threads_info);
    void* sched_thread_result;
    lf_thread_join(_lf_sched_thread, &sched_thread_result);
}


/////////////////// Scheduler Worker API (private) /////////////////////////
/**
 * @brief Ask the scheduler if it is time to stop (and exit).
 * 
 * @param worker_number The worker number of the worker thread asking if it
 * should stop.
 * @return true If the worker thread should stop executing reactions and exit.
 * @return false 
 */
static inline bool _lf_sched_should_stop(size_t worker_number) {
    return _lf_sched_threads_info[worker_number].should_stop;
}

/**
 * @brief Ask the scheduler for more work.
 *
 * This sets the is_idle field for the thread to true and increments
 * '_lf_sched_semaphore' by 1 to inform the scheduler that the worker thread
 * 'worker_number' is idle.
 * 
 * @param worker_number The worker number of the worker thread asking for more
 * work. 
 */
void _lf_sched_ask_for_work(size_t worker_number) {
    // Set the status of this worker thread to idle so that the scheduler can
    // access the data structure of this worker thread.
    lf_bool_compare_and_swap(&_lf_sched_threads_info[worker_number].is_idle, 0, 1);
    DEBUG_PRINT("Worker %d: Asking the scheduler for more work", worker_number);
    // Increment the counting semaphore by 1 to inform the scheduler that there
    // is one more thread that is idle.
    lf_semaphore_release(_lf_sched_semaphore, 1);
}

/**
 * @brief Wait until the scheduler assigns work.
 *
 * This will inform the scheduler that this thread is idle via @see
 * '_lf_sched_ask_for_work' and waits until work is handed out by the
 * scheduler to the worker thread 'worker_number' or it's time for the
 * worker thread to stop.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
void _lf_sched_wait_for_work(size_t worker_number) {
    // Ask for more work from the scheduler.
    _lf_sched_ask_for_work(worker_number);

    lf_mutex_lock(&_lf_sched_threads_info[worker_number].mutex);

    // Check if it is time to stop. If it is, return.
    if (_lf_sched_should_stop(worker_number)) { // Time to stop
        lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);
        return;
    }
    
    // Check if the scheduler has been able to assign work while we let go of
    // the mutex.
    if (pqueue_size(
            _lf_sched_threads_info[worker_number].ready_reactions
            ) > 0 // More work to be done
        ) {
        lf_bool_compare_and_swap(&_lf_sched_threads_info[worker_number].is_idle, 1, 0);
        lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);
        lf_semaphore_acquire(_lf_sched_semaphore);
        return;
    }

    // If no work has been assigned, wait for the signal from the scheduler
    DEBUG_PRINT("Worker %d: Waiting on work to be handed out.", worker_number);
    lf_cond_wait(&_lf_sched_threads_info[worker_number].cond, &_lf_sched_threads_info[worker_number].mutex);
    lf_mutex_unlock(&_lf_sched_threads_info[worker_number].mutex);
    lf_semaphore_acquire(_lf_sched_semaphore);
}


///////////////////// Scheduler Worker API (public) /////////////////////////
/**
 * @brief Ask the scheduler for one more reaction.
 * 
 * If there is a ready reaction for worker thread 'worker_number', then a
 * reaction will be returned. If not, this function will block and ask the
 * scheduler for more work. Once work is delivered, it will return a ready
 * reaction. When it's time for the worker thread to stop and exit, it will
 * return NULL.
 * 
 * @param worker_number 
 * @return reaction_t* A reaction for the worker to execute. NULL if the calling
 * worker thread should exit.
 */
reaction_t* lf_sched_pop_ready_reaction(int worker_number) {
    // Iterate until the stop_tag is reached or reaction queue is empty
    while (!_lf_sched_should_stop(worker_number)) {
        reaction_t* reaction_to_return = (reaction_t*)pqueue_pop(_lf_sched_threads_info[worker_number].ready_reactions);
        
        if (reaction_to_return != NULL) {
            // Got a reaction
            return reaction_to_return;
        }

        DEBUG_PRINT("Worker %d is out of ready reactions.", worker_number);

        // Ask the scheduler for more work and wait
        _lf_sched_wait_for_work(worker_number);
    }

    // It's time for the worker thread to stop and exit.
    return NULL;

    // if (reaction_to_return == NULL && _lf_sched_number_of_workers > 1) {
    //     // Try to steal
    //     int index_to_steal = (worker_number + 1) % _lf_sched_number_of_workers;
    //     lf_mutex_lock(&_lf_sched_threads_info[index_to_steal].mutex);
    //     reaction_to_return = 
    //         pqueue_pop(_lf_sched_threads_info[index_to_steal].ready_reactions);
    //     if (reaction_to_return != NULL) {
    //         DEBUG_PRINT(
    //             "Worker %d: Had nothing on my ready queue. Stole reaction %s from %d", 
    //             worker_number,
    //             reaction_to_return->name,
    //             index_to_steal);
    //     }
    //     lf_mutex_unlock(&_lf_sched_threads_info[index_to_steal].mutex);
    // }

    // return reaction_to_return;
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 * 
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction is that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction) {
    vector_push(&_lf_sched_threads_info[worker_number].done_reactions, (void*)done_reaction);
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * enqueue 'reaction'.
 * 
 * This enqueuing happens lazily (at a later point when the scheduler deems
 * appropriate), unless worker_number is set to -1. In that case, the enqueuing
 * of 'reaction' is done immediately.
 * 
 * @param reaction The reaction to enqueue.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 should be used if the scheduler should handle
 *  enqueuing the reaction immediately.
 */
void lf_sched_worker_enqueue_reaction(int worker_number, reaction_t* reaction) {
    if (worker_number == -1) {
        // The scheduler should handle this immediately
        lf_mutex_lock(&mutex);
        // Do not enqueue this reaction twice.
        if (reaction != NULL && reaction->status == inactive) {
            DEBUG_PRINT("Enqueing downstream reaction %s, which has level %lld.",
                        reaction->name, reaction->index & 0xffffLL);
            reaction->status = queued;
            // Immediately put 'reaction' on the reaction queue.
            pqueue_insert(reaction_q, reaction);
        }
        lf_mutex_unlock(&mutex);
        return;
    }
    if (reaction != NULL) {
        DEBUG_PRINT("Worker %d: Enqueuing downstream reaction %s, which has level %lld.",
        		worker_number, reaction->name, reaction->index & 0xffffLL);
        reaction->worker_affinity = worker_number;
        // Note: The scheduler will check that we don't enqueue this reaction
        // twice when it is actually pushing it to the global reaction queue.
        vector_push(&_lf_sched_threads_info[worker_number].output_reactions, (void*)reaction);
    }
}

///////////////////// Scheduler Runtime API (private) /////////////////////////

/**
 * @brief Return true if the worker thread 'worker_number' is idle. False otherwise.
 * 
 */
static inline bool _lf_sched_is_worker_idle(size_t worker_number) {
    return (_lf_sched_threads_info[worker_number].is_idle == 1);
}

/**
 * @brief Distribute 'ready_reaction' to the best idle thread.
 * 
 * This will start from 'ready_reaction->worker_affinity' and rotates through
 * workers (only once) until it finds an idle thread to distribute the work to.
 * If successful, it will return true. If it cannot find a worker thread to
 * execute this reaction, it will return false.
 *
 * @param ready_reaction A reaction that is ready to execute.
 * @return true Found a worker thread to execute 'ready_reaction'.
 * @return false Could not find a worker thread to execute 'ready_reaction'.
 */
static inline bool _lf_sched_distribute_ready_reaction(reaction_t* ready_reaction) {
    DEBUG_PRINT("Scheduler: Trying to distribute reaction %s.", ready_reaction->name);
    bool target_thread_found = false;
    // Start with the preferred worker for the ready reaction or the balancing
    // index, whichever is larger.
    size_t worker_id = MAX(ready_reaction->worker_affinity, _lf_sched_balancing_index);
    // Rotate through all the workers once.
    for(size_t i=0; i<_lf_sched_number_of_workers; i++) {
        // Go over all the workers to see if anyone is idle.
        if (_lf_sched_is_worker_idle(worker_id)) {
            // FIXME: It could be possible to cache this status for each round
            // of work distribution only once so that locking the thread mutex
            // is subsequently not necessary for workers that are busy, but this
            // caching adds overhead.

            // The worker is idle.
            DEBUG_PRINT(
                "Scheduler: Assigning reaction %s to worker %d.",
                ready_reaction->name,
                worker_id);
            // Add the ready reaction to the ready_reaction queue of the idle worker.
            if (pqueue_insert(
                _lf_sched_threads_info[worker_id].ready_reactions,
                ready_reaction
            ) != 0) {
                error_print_and_exit("Could not assign reaction to worker %d.", worker_id);
            }
            target_thread_found = true;
            ready_reaction->status = running;
            // Push the reaction on the executing queue in order to prevent any
            // reactions that may depend on it from executing before this reaction is finished.
            pqueue_insert(executing_q, ready_reaction);
        }

        worker_id++;
        
        // Rotate through workers in a circular fashion.
        if (worker_id == _lf_sched_number_of_workers) {
            worker_id = 0;
        }

        if (target_thread_found) {
            break;
        }
    }

    // Update the balancing index to be the next worker in line
    // FIXME: Ideally, it's better to set this index to the least idle worker
    // number but that is an expensive operation.
    _lf_sched_balancing_index = worker_id;

    return target_thread_found;
        
}

/**
 * Return true if the first reaction has precedence over the second, false otherwise.
 * @param r1 The first reaction.
 * @param r2 The second reaction.
 */
bool _lf_has_precedence_over(reaction_t* r1, reaction_t* r2) {
    if (LEVEL(r1->index) < LEVEL(r2->index)
            && OVERLAPPING(r1->chain_id, r2->chain_id)) {
        return true;
    }
    return false;
}

/**
 * If the reaction is blocked by a currently executing
 * reaction, return true. Otherwise, return false.
 * A reaction blocks the specified reaction if it has a
 * level less than that of the specified reaction and it also has
 * an overlapping chain ID, meaning that it is (possibly) upstream
 * of the specified reaction.
 * This function assumes the mutex is held because it accesses
 * the executing_q.
 * @param reaction The reaction.
 * @return true if this reaction is blocked, false otherwise.
 */
bool _lf_sched_is_blocked_by_executing_reaction(reaction_t* reaction) {
    if (reaction == NULL) {
        return false;
    }
    for (size_t i = 1; i < executing_q->size; i++) {
        reaction_t* running = (reaction_t*) executing_q->d[i];
        if (_lf_has_precedence_over(running, reaction)) {
            DEBUG_PRINT("Reaction %s is blocked by reaction %s.", reaction->name, running->name);
            return true;
        }
    }
    // NOTE: checks against the transfer_q are not performed in 
    // this function but at its call site (where appropriate).

    // printf("Not blocking for reaction with chainID %llu and level %llu\n", reaction->chain_id, reaction->index);
    // pqueue_dump(executing_q, stdout, executing_q->prt);
    return false;
}

/**
 * @brief Distribute any reaction that is ready to execute to idle worker thread(s).
 * 
 * This assumes that the caller is not holding any thread mutexes.
 * 
 * @return Number of reactions that were successfully distributed to worker threads.
 */ 
int distribute_ready_reactions() {    
    reaction_t* r;
    // Keep track of the chain IDs of blocked reactions.
    unsigned long long mask = 0LL;

    int reactions_distributed = 0;

    // Find a reaction that is ready to execute.
    while ((r = (reaction_t*)pqueue_pop(reaction_q)) != NULL) {
        // Set the reaction aside if it is blocked, either by another
        // blocked reaction or by a reaction that is currently executing.
        if (OVERLAPPING(mask, r->chain_id)) {
            DEBUG_PRINT("Reaction %s is blocked by a reaction that is also blocked.", r->name);
        } else {
            if (!_lf_sched_is_blocked_by_executing_reaction(r)) {
                if (_lf_sched_distribute_ready_reaction(r)){
                    // Found a thread to execute r
                    reactions_distributed++;
                    continue;
                }
                // Couldn't find a thread to execute r.
                DEBUG_PRINT("Scheduler: Could not find an idle thread to execute reaction %s.", r->name);
            }
        }
        // Couldn't execute the reaction. Will have to put it back in the
        // reaction queue.
        vector_push(&transfer_q, (void*)r);
        mask = mask | r->chain_id;
    }

    // Reset the balancing index since this work distribution round is over.
    _lf_sched_balancing_index = 0;

    // Put back the set-aside reactions into the reaction queue.
    reaction_t* reaction_to_transfer = NULL;
    while ((reaction_to_transfer = (reaction_t*)vector_pop(&transfer_q)) != NULL) {
        pqueue_insert(reaction_q, reaction_to_transfer);
    }

    return reactions_distributed;
}


/**
 * Return true if the worker should stop now; false otherwise.
 * This function assumes the caller holds the mutex lock.
 */
bool _lf_sched_should_stop_locked() {
    // If this is not the very first step, notify that the previous step is complete
    // and check against the stop tag to see whether this is the last step.
    if (_lf_logical_tag_completed) {
        logical_tag_complete(current_tag);
        // If we are at the stop tag, do not call _lf_next_locked()
        // to prevent advancing the logical time.
        if (compare_tags(current_tag, stop_tag) >= 0) {
            return true;
        }
    }
    return false;
}

/**
 * Advance tag. This will also pop events for the newly acquired tag and put
 * the triggered reactions on the reaction queue.
 * 
 * This function assumes the caller holds the 'mutex' lock.
 * 
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_advance_tag_locked() {

    if (_lf_sched_should_stop_locked()) {
        return true;
    }

    _lf_logical_tag_completed = true;

    // Advance time.
    // _lf_next_locked() may block waiting for real time to pass or events to appear.
    // to appear on the event queue. Note that we already
    // hold the mutex lock.
    // tracepoint_worker_advancing_time_starts(worker_number); 
    // FIXME: Tracing should be updated to support scheduler events
    _lf_next_locked();

    DEBUG_PRINT("Scheduler: Done waiting for _lf_next_locked().");
    return false;
}

/**
 * @brief Transfer the contents of worker thread queues to the actual global queues.
 * 
 * This will transfer worker threads' output reactions to the reaction queue and
 * removes worker threads' done reactions from the executing queue.
 * 
 * This assumes that the caller is not holding any thread mutexes.
 * 
 * @return true If any of the workers were busy.
 * @return false All the workers were idle.
 */
bool _lf_sched_update_queues() {
    bool is_any_worker_busy = false;
    for (int i = 0; i < _lf_sched_number_of_workers; i++) {
        // Check if we have actually assigned work to this worker thread previously.
        reaction_t* reaction_to_add = NULL;
        reaction_t* reaction_to_remove = NULL;
        if (!_lf_sched_is_worker_idle(i)) {
            // Don't touch the queues since the thread is still busy
            DEBUG_PRINT("Scheduler: Worker %d is busy. Won't empty the queues for it.", i);
            is_any_worker_busy = true;
            continue;
        }
        DEBUG_PRINT("Scheduler: Emptying queues of Worker %d.", i);
        // Add output reactions to the reaction queue
        while(
            (reaction_to_add = 
            (reaction_t*)vector_pop(&_lf_sched_threads_info[i].output_reactions))
            != NULL) {
            DEBUG_PRINT(
                "Scheduler: Inserting reaction %s into the reaction queue.",
                reaction_to_add->name
            );
            // Avoid inserting duplicate reactions.
            if (reaction_to_add->status == inactive) {
                reaction_to_add->status = queued;
                if (pqueue_insert(reaction_q, reaction_to_add) != 0) {
                    error_print_and_exit("Scheduler: Could not properly fill the reaction queue.");
                }
            }
        }

        // Remove done reactions from the executing queue
        while(
            (reaction_to_remove = 
            (reaction_t*)vector_pop(&_lf_sched_threads_info[i].done_reactions))
            != NULL) {
            DEBUG_PRINT(
                "Scheduler: Removing reaction %s from executing queue.",
                reaction_to_remove->name
            );
            if (pqueue_remove(executing_q, reaction_to_remove) != 0) {
                error_print_and_exit("Scheduler: Could not properly clear the executing queue.");
            }
            reaction_to_remove->status = inactive;
        }
    }
    return is_any_worker_busy;
}

/**
 * @brief If there is work to be done, notify workers individually.
 * 
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers() {
    for (int i=0; i< _lf_sched_number_of_workers; i++) {
        if (pqueue_size(_lf_sched_threads_info[i].ready_reactions) > 0 &&
            lf_bool_compare_and_swap(&_lf_sched_threads_info[i].is_idle, 1, 0)) {
            DEBUG_PRINT("Notifying worker %d that there is work to do.", i);
            lf_mutex_lock(&_lf_sched_threads_info[i].mutex);
            lf_cond_signal(&_lf_sched_threads_info[i].cond);
            lf_mutex_unlock(&_lf_sched_threads_info[i].mutex);
        }
    }
}

/**
 * @brief Advance tag or distribute reactions to worker threads.
 *
 * Advance tag if there are no reactions in the reaction queue or in progress. If
 * there are such reactions, distribute them to worker threads. As part of its
 * book-keeping, this function will clear the output_reactions and
 * done_reactions queues of all idle worker threads if appropriate.
 * 
 * This function assumes the caller does not hold the 'mutex' lock.
 * 
 * @return should_exit True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_try_advance_tag_and_distribute() {
    bool return_value = false;

    if (!_lf_sched_update_queues()) {
        lf_mutex_lock(&mutex);
        if (pqueue_size(reaction_q) == 0
                && pqueue_size(executing_q) == 0) {
            // Nothing more happening at this logical time.

            // Protect against asynchronous events while advancing time by locking
            // the mutex.
            DEBUG_PRINT("Scheduler: Advancing time.");
            // This thread will take charge of advancing time.
            if (_lf_sched_advance_tag_locked()) {
                DEBUG_PRINT("Scheduler: Reached stop tag.");
                return_value = true;
            }
        }
        lf_mutex_unlock(&mutex);
    }
    
    if (distribute_ready_reactions() > 0) {
        _lf_sched_notify_workers();
    }
    
    DEBUG_PRINT("Scheduler: Executing queue size is %zu.", pqueue_size(executing_q));
    // pqueue_dump(executing_q, print_reaction);
    return return_value;
}

/**
 * @brief Block the scheduler until at least one worker thread is asking for
 * more work.
 * 
 */
void _lf_sched_wait_for_threads_asking_for_more_work() {
    if (pqueue_size(reaction_q) == 0 && pqueue_size(executing_q) == 0) {
        // No work to be handed out
        return;
    }
    DEBUG_PRINT("Scheduler: Waiting for threads to ask for more work");

    // If the semaphore is 0, all threads are busy.
    lf_semaphore_wait(_lf_sched_semaphore);
    
}

/**
 * @brief Signal all worker threads that it is time to stop.
 * 
 */
void _lf_sched_signal_stop() {
    for (int i=0; i < _lf_sched_number_of_workers; i++) {
        lf_mutex_lock(&_lf_sched_threads_info[i].mutex);
        _lf_sched_threads_info[i].should_stop = true;
        lf_cond_signal(&_lf_sched_threads_info[i].cond);
        lf_mutex_unlock(&_lf_sched_threads_info[i].mutex);
    }
}

/**
 * @brief The main thread of the scheduler, to be created by the main program.
 * 
 * @param arg Ignored.
 * @return void* NULL on exit.
 */
void* _lf_sched_scheduling_thread(void* arg) {
    while(!_lf_sched_try_advance_tag_and_distribute()) {
        _lf_sched_wait_for_threads_asking_for_more_work();
    }
    _lf_sched_signal_stop();
    return NULL;
}