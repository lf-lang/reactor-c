/**
 * @file
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @author Soroush Bateni
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief  Runtime infrastructure for the threaded version of the C target of Lingua Franca.
 */
#ifndef REACTOR_THREADED_H
#define REACTOR_THREADED_H

#include "lf_types.h"

/**
 * Enqueue port absent reactions that will send a PORT_ABSENT
 * message to downstream federates if a given network output port is not present.
 * @param env The environment in which we are executing
 */
void lf_enqueue_port_absent_reactions(environment_t* env);

/**
 * Raise a barrier to prevent the current tag for the specified environment from advancing
 * to or beyond the value of the future_tag argument, if possible.
 * If the current tag is already at or beyond future_tag, then
 * prevent any further advances. This function will increment the
 * total number of pending barrier requests. For each call to this
 * function, there should always be a subsequent call to
 * _lf_decrement_tag_barrier_locked()
 * to release the barrier.
 *
 * If there is already a barrier raised at a tag later than future_tag, this
 * function will change the barrier to future_tag or the current tag, whichever
 * is larger. If the existing barrier is earlier
 * than future_tag, this function will not change the barrier. If there are
 * no existing barriers and future_tag is in the past relative to the
 * current tag, this function will raise a barrier to the current tag plus one microstep.
 *
 * This function acquires the mutex on the specified environment.
 *
 * @note This function is only useful in threaded applications to facilitate
 *  certain non-blocking functionalities such as receiving timed messages
 *  over the network or handling stop in a federated execution.
 *
 * @param env Environment within which we are executing.
 * @param future_tag A desired tag for the barrier. This function will guarantee
 * that current logical time will not go past future_tag if it is in the future.
 * If future_tag is in the past (or equals to current logical time), the runtime
 * will freeze advancement of logical time.
 */
void _lf_increment_tag_barrier(environment_t* env, tag_t future_tag);

/**
 * @brief Version of _lf_increment_tag_barrier to call when the caller holds the mutex.
 * This version does not acquire the mutex belonging to env.
 *
 * @param env Environment within which we are executing.
 * @param future_tag A desired tag for the barrier. This function will guarantee
 * that current logical time will not go past future_tag if it is in the future.
 * If future_tag is in the past (or equals to current logical time), the runtime
 * will freeze advancement of logical time.
 */
void _lf_increment_tag_barrier_locked(environment_t* env, tag_t future_tag);

/**
 * Decrement the total number of pending barrier requests for the environment tag barrier.
 * If the total number of requests reaches zero, this function resets the
 * tag barrier to FOREVER_TAG and notifies all threads that are waiting
 * on the barrier that the number of requests has reached zero.
 *
 * This function assumes that the caller already holds the mutex lock on env.
 *
 * @note This function is only useful in threaded applications to facilitate
 *  certain non-blocking functionalities such as receiving timed messages
 *  over the network or handling stop in the federated execution.
 *
 * @param env The environment in which we are executing.
 */
void _lf_decrement_tag_barrier_locked(environment_t* env);

/**
 * @brief The number of cores to use.
 *
 * If the target parameter number_of_cores is set, it will override this default.
 */
#ifndef LF_NUMBER_OF_CORES
#define LF_NUMBER_OF_CORES 0
#endif

/**
 * @brief The thread scheduling policy to use.
 *
 * This should be one of   LF_SCHED_FAIR, LF_SCHED_TIMESLICE, or LF_SCHED_PRIORITY.
 * The default is LF_SCHED_FAIR, which corresponds to the Linux SCHED_OTHER.
 * LF_SCHED_TIMESLICE corresponds to Linux SCHED_RR, and LF_SCHED_PRIORITY corresponds
 * to SCHED_FIFO.
 */
#ifndef LF_THREAD_POLICY
#define LF_THREAD_POLICY LF_SCHED_FAIR
#endif

int _lf_wait_on_tag_barrier(environment_t* env, tag_t proposed_tag);
void lf_synchronize_with_other_federates(void);
bool wait_until(instant_t logical_time_ns, lf_cond_t* condition);
tag_t get_next_event_tag(environment_t* env);
tag_t send_next_event_tag(environment_t* env, tag_t tag, bool wait_for_reply);
void _lf_next_locked(environment_t* env);
#endif // REACTOR_THREADED_H
