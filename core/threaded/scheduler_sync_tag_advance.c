/**
 * @file scheduler_sync_tag_advance.c
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @brief API used to advance tag globally
 * @version 0.1
 * @date 2021-12-19
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/////////////////// External Variables /////////////////////////
extern tag_t current_tag;
extern tag_t stop_tag;

/////////////////// External Functions /////////////////////////
/**
 * Placeholder for function that will advance tag and initially fill the
 * reaction queue.
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

/**
 * @brief Indicator that execution of at least one tag has completed.
 */
bool _lf_logical_tag_completed = false;

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
 * the triggered reactions on the '_lf_sched_vector_of_reaction_qs'.
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