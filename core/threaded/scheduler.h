#ifndef LF_SCHEDULER_H
#define LF_SCHEDULER_H

#include "../reactor.h"


typedef enum {
    GEDF_NP,
    PEDF_NP
} lf_supported_schedulers;

/**
 * @brief Initialize the scheduler.
 * 
 * This has to be called before the main thread of the scheduler is created.
 * 
 * @param number_of_workers Indicate how many workers this scheduler will be managing.
 */
void lf_sched_init(size_t number_of_workers);

/**
 * @brief Free the memory used by the scheduler.
 * 
 * This must be called after the main scheduler thread exits.
 * 
 */
void lf_sched_free();

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
reaction_t* lf_sched_pop_ready_reaction(int worker_number);

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 * 
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction is that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction);


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
void lf_sched_worker_trigger_reaction(int worker_number, reaction_t* reaction);

#endif // LF_SCHEDULER_H