/**
 * @file scheduler_sync_tag_advance.h
 * @author Soroush Bateni
 * @author Edward A. Lee
 * @author Marten Lohstroh
 *
 * @brief API used to advance tag globally.
 * @ingroup Internal
 */

#ifndef SCHEDULER_SYNC_TAG_ADVANCE_H
#define SCHEDULER_SYNC_TAG_ADVANCE_H

#include <stdbool.h>

#include "tag.h"
#include "scheduler_instance.h"

/**
 * @brief Placeholder for code-generated function that will, in a federated
 * execution, be used to coordinate the advancement of tag.
 * @ingroup Internal
 *
 * It will notify the runtime infrastructure (RTI) that all reactions at the
 * specified logical tag have completed.
 *
 * This function should be called only while holding the mutex lock.
 *
 * @param tag_to_send The tag to send.
 */
void logical_tag_complete(tag_t tag_to_send);

/**
 * @brief Return true if the worker should stop now; false otherwise.
 * @ingroup Internal
 * This function assumes the caller holds the mutex lock.
 * @param sched The scheduler instance to check.
 */
bool should_stop_locked(lf_scheduler_t* sched);

/**
 * @brief Advance the tag to the next tag on the event queue.
 * @ingroup Internal
 *
 * This will also pop events for the newly acquired tag and trigger
 * the enabled reactions using the scheduler.
 *
 * This function assumes the caller holds the environment mutex lock.
 * @param sched The scheduler instance to check.
 * @return True if the worker thread should exit. False otherwise.
 */
bool _lf_sched_advance_tag_locked(lf_scheduler_t* sched);

#endif // LF_C11_THREADS_SUPPORT_H
