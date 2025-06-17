/**
 * @file reactor_threaded.h
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @author Soroush Bateni
 *
 * @brief Runtime infrastructure for the threaded version of the C target of Lingua Franca.
 * @ingroup Internal
 */
#ifndef REACTOR_THREADED_H
#define REACTOR_THREADED_H

#include "lf_types.h"

/**
 * @brief Raise a barrier to prevent the current tag for the specified environment from advancing
 * to or beyond the value of the future_tag argument, if possible.
 * @ingroup Internal
 *
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
 * @ingroup Internal
 *
 * @param env Environment within which we are executing.
 * @param future_tag A desired tag for the barrier. This function will guarantee
 * that current logical time will not go past future_tag if it is in the future.
 * If future_tag is in the past (or equals to current logical time), the runtime
 * will freeze advancement of logical time.
 */
void _lf_increment_tag_barrier_locked(environment_t* env, tag_t future_tag);

/**
 * @brief Decrement the total number of pending barrier requests for the environment tag barrier.
 * @ingroup Internal
 *
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
 * @brief Wait on the tag barrier for the environment.
 * @ingroup Internal
 *
 * If the proposed_tag is greater than or equal to a barrier tag that has been
 * set by a call to _lf_increment_tag_barrier or
 * _lf_increment_tag_barrier_locked, and if there are requestors
 * still pending on that barrier, then wait until all requestors have been
 * satisfied. This is used in federated execution when an incoming timed
 * message has been partially read so that we know its tag, but the rest of
 * message has not yet been read and hence the event has not yet appeared
 * on the event queue.  To prevent tardiness, this function blocks the
 * advancement of time until to the proposed tag until the message has
 * been put onto the event queue.
 *
 * If the proposed_tag is greater than the stop tag, then use the stop tag instead.
 *
 * This function assumes the mutex is already locked.
 * Thus, it unlocks the mutex while it's waiting to allow
 * the tag barrier to change.
 *
 * @param env Environment within which we are executing.
 * @param proposed_tag The tag that the runtime wants to advance to.
 * @return 0 if no wait was needed and 1 if a wait actually occurred.
 */
int _lf_wait_on_tag_barrier(environment_t* env, tag_t proposed_tag);

/**
 * @brief Wait until physical time matches or exceeds the time of the specified tag.
 * @ingroup Internal
 *
 * If -fast is given, there will be no wait.
 *
 * If an event is put on the event queue during the wait, then the wait is
 * interrupted and this function returns false. It also returns false if the
 * timeout time is reached before the wait has completed. Note this this could
 * return true even if the a new event was placed on the queue. This will occur
 * if that event time matches or exceeds the specified time.
 *
 * The mutex lock associated with the condition argument is assumed to be held by
 * the calling thread. This mutex is released while waiting. If the current physical
 * time has already passed the specified time, then this function
 * immediately returns true and the mutex is not released.
 *
 * @param wait_until_time The time to wait until physical time matches it.
 * @param condition A condition variable that can interrupt the wait. The mutex
 * associated with this condition variable will be released during the wait.
 *
 * @return Return false if the wait is interrupted either because of an event
 *  queue signal or if the wait time was interrupted early by reaching
 *  the stop time, if one was specified. Return true if the full wait time
 *  was reached.
 */
bool wait_until(instant_t wait_until_time, lf_cond_t* condition);

/**
 * @brief Return the tag of the next event on the event queue.
 * @ingroup Internal
 *
 * If the event queue is empty then return either FOREVER_TAG
 * or, is a stop_time (timeout time) has been set, the stop time.
 *
 * @param env Environment within which we are executing.
 */
tag_t get_next_event_tag(environment_t* env);

/**
 * @brief Send the next event tag.
 * @ingroup Internal
 *
 * In a federated execution with centralized coordination, this function returns
 * a tag that is less than or equal to the specified tag when, as far
 * as the federation is concerned, it is safe to commit to advancing
 * to the returned tag. That is, all incoming network messages with
 * tags less than the returned tag have been received.
 * In unfederated execution or in federated execution with decentralized
 * control, this function returns the specified tag immediately.
 *
 * @param env Environment within which we are executing.
 * @param tag The tag to which to advance.
 * @param wait_for_reply If true, wait for the RTI to respond.
 * @return The tag to which it is safe to advance.
 */
tag_t send_next_event_tag(environment_t* env, tag_t tag, bool wait_for_reply);

/**
 * @brief Advance the logical time.
 * @ingroup Internal
 *
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
 * If keepalive was given, then wait for either lf_request_stop()
 * to be called or an event appears in the event queue and then return.
 *
 * Every time tag is advanced, it is checked against stop tag and if they are
 * equal, shutdown reactions are triggered.
 *
 * This does not acquire the mutex lock. It assumes the lock is already held.
 * @param env Environment within which we are executing.
 */
void _lf_next_locked(environment_t* env);
#endif // REACTOR_THREADED_H
