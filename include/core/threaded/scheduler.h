/**
 * @file scheduler.h
 * @author Soroush Bateni
 * @author Edward A. Lee
 *
 * @brief Scheduler API for the threaded C runtime.
 * @ingroup Internal
 *
 * A scheduler for the threaded runtime of reactor-c should provide an
 * implementation for functions that are defined in this header file.
 */

#ifndef LF_SCHEDULER_H
#define LF_SCHEDULER_H

#include "lf_types.h"
#include "scheduler_instance.h"

/**
 * @brief Initialize the scheduler.
 * @ingroup Internal
 *
 * This has to be called before other functions of the scheduler can be used.
 * If the scheduler is already initialized, this will be a no-op.
 *
 * @param env The environment in which we should initialize the scheduler
 * @param number_of_workers Indicate how many workers this scheduler will be
 *  managing.
 * @param parameters Pointer to a `sched_params_t` struct containing additional
 *  scheduler parameters. Can be NULL.
 */
void lf_sched_init(environment_t* env, size_t number_of_workers, sched_params_t* parameters);

/**
 * @brief Free the memory used by the scheduler.
 * @ingroup Internal
 *
 * @param scheduler The scheduler
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free(lf_scheduler_t* scheduler);

/**
 * @brief Ask the scheduler for one more reaction.
 * @ingroup Internal
 *
 * This function blocks until it can return a ready reaction for worker thread
 * 'worker_number' or it is time for the worker thread to stop and exit (where a
 * NULL value would be returned).
 * This function assumes that the environment mutex is not locked.
 *
 * @param scheduler The scheduler
 * @param worker_number For the calling worker thread.
 * @return reaction_t* A reaction for the worker to execute. NULL if the calling
 * worker thread should exit.
 */
reaction_t* lf_sched_get_ready_reaction(lf_scheduler_t* scheduler, int worker_number);

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 * @ingroup Internal
 *
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number, reaction_t* done_reaction);

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 * @ingroup Internal
 *
 * If a worker number is not available (e.g., this function is not called by a
 * worker thread), -1 should be passed as the 'worker_number'.
 *
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 *
 * @param scheduler The scheduler.
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  single-threaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 *
 */
void lf_scheduler_trigger_reaction(lf_scheduler_t* scheduler, reaction_t* reaction, int worker_number);

/**
 * @brief Inform the scheduler that one more worker thread is about to join the pool.
 * @ingroup Internal
 *
 * This is called by @ref lf_add_worker_thread() while holding the environment mutex and
 * while the calling worker is busy executing a reaction (and hence is not idle), before
 * the new thread is created. The scheduler should increase its count of managed workers
 * so that its determination of when all workers are idle accounts for the new thread.
 * A scheduler that cannot accommodate a worker added at runtime should return a non-zero
 * value without changing its state, in which case no thread will be created.
 *
 * If this returns 0 but the thread subsequently cannot be created, then
 * @ref lf_sched_cancel_add_worker() will be called to undo the effect of this call.
 *
 * @param scheduler The scheduler.
 * @return 0 on success, or -1 if this scheduler does not support adding workers at runtime.
 */
int lf_sched_add_worker(lf_scheduler_t* scheduler);

/**
 * @brief Undo a previous successful call to @ref lf_sched_add_worker().
 * @ingroup Internal
 *
 * This is called by @ref lf_add_worker_thread(), while holding the environment mutex,
 * if the worker thread could not be created after the scheduler agreed to accommodate it.
 * The scheduler should decrease its count of managed workers accordingly.
 *
 * @param scheduler The scheduler.
 */
void lf_sched_cancel_add_worker(lf_scheduler_t* scheduler);

/**
 * @brief Inform the scheduler that the calling worker thread is temporarily leaving the pool.
 * @ingroup Internal
 *
 * This is called by `lf_async()` while holding the environment mutex and while the calling
 * worker is executing a reaction (and hence is not idle). The scheduler should decrease its
 * count of managed workers so that the remaining workers can conclude that all workers are
 * idle, and hence advance logical time, without waiting for the caller.
 *
 * If, as a result, all remaining workers are idle, the scheduler must ensure that one of them
 * notices and advances the tag, since otherwise the environment would stall. At least one
 * worker must remain in the pool; the caller is responsible for adding a worker thread first
 * if necessary.
 *
 * The caller (or another thread acting on its behalf) later rejoins the pool with
 * @ref lf_sched_add_worker().
 *
 * @param scheduler The scheduler.
 * @return 0 on success, or -1 if this scheduler does not support workers leaving at runtime.
 */
int lf_sched_remove_worker(lf_scheduler_t* scheduler);

#endif // LF_SCHEDULER_H
