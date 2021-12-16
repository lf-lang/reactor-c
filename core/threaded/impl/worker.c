/**
 * @brief Worker functionality for the threaded runtime of the C target of
 * Lingua Franca. 
 * 
 * @file worker.c
 * 
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * 
 * @copyright Copyright (c) 2021, The University of California at Berkeley.
 * 
 */

/*************
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

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "../worker.h"
#include "../../reactor.h"
#include "../../utils/util.h"
#include "../../platform.h"
#include "../scheduler.h"


/////////////////// External Variables /////////////////////////
/**
 * The global mutex lock.
 */
extern lf_mutex_t mutex;

/**
 * Global condition variable used for notification between threads.
 */
extern lf_cond_t event_q_changed;

/////////////////// External Functions /////////////////////////
/**
 * For the specified reaction, if it has produced outputs, insert the
 * resulting triggered reactions into the reaction queue.
 * This procedure assumes the mutex lock is NOT held and grabs
 * the lock only when it actually inserts something onto the reaction queue.
 * @param reaction The reaction that has just executed.
 * @param worker The thread number of the worker thread or 0 for unthreaded execution (for tracing).
 */
void schedule_output_reactions(reaction_t* reaction, int worker);

/**
 * In a federated execution with centralized coordination, this function returns
 * a tag that is less than or equal to the specified tag when, as far
 * as the federation is concerned, it is safe to commit to advancing
 * to the returned tag. That is, all incoming network messages with
 * tags less than the returned tag have been received.
 * In unfederated execution or in federated execution with decentralized
 * control, this function returns the specified tag immediately.
 *
 * @param tag The tag to which to advance.
 * @param wait_for_reply If true, wait for the RTI to respond.
 * @return The tag to which it is safe to advance.
 */
tag_t send_next_event_tag(tag_t tag, bool wait_for_reply);

///////////////////////////////////////////////////////////////

/** For logging and debugging, each worker thread is numbered. */
int worker_thread_count = 0;

// Array of thread IDs (to be dynamically allocated).
lf_thread_t* _lf_thread_ids;

/**
 * Handle deadline violation for 'reaction'. 
 * The mutex should NOT be locked when this function is called. It might acquire
 * the mutex when scheduling the reactions that are triggered as a result of
 * executing the deadline violation handler on the 'reaction', if it exists.
 *
 * @return true if a deadline violation occurred. false otherwise.
 */
bool _lf_worker_handle_deadline_violation_for_reaction(int worker_number, reaction_t* reaction) {
    bool violation_occurred = false;
    // If the reaction has a deadline, compare to current physical time
    // and invoke the deadline violation reaction instead of the reaction function
    // if a violation has occurred. Note that the violation reaction will be invoked
    // at most once per logical time value. If the violation reaction triggers the
    // same reaction at the current time value, even if at a future superdense time,
    // then the reaction will be invoked and the violation reaction will not be invoked again.
    if (reaction->deadline > 0LL) {
        // Get the current physical time.
        instant_t physical_time = get_physical_time();
        // Check for deadline violation.
        if (physical_time > get_logical_time() + reaction->deadline) {
            // Deadline violation has occurred.
            violation_occurred = true;
            // Invoke the local handler, if there is one.
            reaction_function_t handler = reaction->deadline_violation_handler;
            if (handler != NULL) {
                LOG_PRINT("Worker %d: Deadline violation. Invoking deadline handler.",
                        worker_number);
                (*handler)(reaction->self);

                // If the reaction produced outputs, put the resulting
                // triggered reactions into the queue or execute them directly if possible.
                schedule_output_reactions(reaction, worker_number);
                // Remove the reaction from the executing queue.
            }
        }
    }
    return violation_occurred;
}

/**
 * Handle STP violation for 'reaction'. 
 * The mutex should NOT be locked when this function is called. It might acquire
 * the mutex when scheduling the reactions that are triggered as a result of
 * executing the STP violation handler on the 'reaction', if it exists.
 *
 * @return true if an STP violation occurred. false otherwise.
 */
bool _lf_worker_handle_STP_violation_for_reaction(int worker_number, reaction_t* reaction) {
    bool violation_occurred = false;
    // If the reaction violates the STP offset,
    // an input trigger to this reaction has been triggered at a later
    // logical time than originally anticipated. In this case, a special
    // STP handler will be invoked.             
    // FIXME: Note that the STP handler will be invoked
    // at most once per logical time value. If the STP handler triggers the
    // same reaction at the current time value, even if at a future superdense time,
    // then the reaction will be invoked and the STP handler will not be invoked again.
    // However, inputs ports to a federate reactor are network port types so this possibly should
    // be disallowed.
    // @note The STP handler and the deadline handler are not mutually exclusive.
    //  In other words, both can be invoked for a reaction if it is triggered late
    //  in logical time (STP offset is violated) and also misses the constraint on 
    //  physical time (deadline).
    // @note In absence of an STP handler, the is_STP_violated will be passed down the reaction
    //  chain until it is dealt with in a downstream STP handler.
    if (reaction->is_STP_violated == true) {
        reaction_function_t handler = reaction->STP_handler;
        LOG_PRINT("STP violation detected.");
        // Invoke the STP handler if there is one.
        if (handler != NULL) {
            LOG_PRINT("Worker %d: Invoking tardiness handler.", worker_number);
            // There is a violation
            violation_occurred = true;
            (*handler)(reaction->self);
            
            // If the reaction produced outputs, put the resulting
            // triggered reactions into the queue or execute them directly if possible.
            schedule_output_reactions(reaction, worker_number);
            
            // Reset the is_STP_violated because it has been dealt with
            reaction->is_STP_violated = false;
        }
    }
    return violation_occurred;
}

/**
 * Handle violations for 'reaction'. Currently limited to deadline violations
 * and STP violations. 
 * The mutex should NOT be locked when this function is called. It might acquire
 * the mutex when scheduling the reactions that are triggered as a result of
 * executing the deadline or STP violation handler(s) on the 'reaction', if they
 * exist.
 *
 * @return true if a violation occurred. false otherwise.
 */
bool _lf_worker_handle_violations(int worker_number, reaction_t* reaction) {
    bool violation = false;
    
    violation = _lf_worker_handle_deadline_violation_for_reaction(worker_number, reaction) ||
                    _lf_worker_handle_STP_violation_for_reaction(worker_number, reaction);
    return violation;
}

/**
 * Invoke 'reaction' and schedule any resulting triggered reaction(s) on the
 * reaction queue. 
 * The mutex should NOT be locked when this function is called. It might acquire
 * the mutex when scheduling the reactions that are triggered as a result of
 * executing 'reaction'.
 */
void _lf_worker_invoke_reaction(int worker_number, reaction_t* reaction) {
    tag_t current_tag = get_current_tag();
    LOG_PRINT("Worker %d: Invoking reaction %s at elapsed tag (%lld, %d).",
            worker_number,
            reaction->name,
            current_tag.time - get_start_time(),
            current_tag.microstep);
    tracepoint_reaction_starts(reaction, worker_number);
    reaction->function(reaction->self);
    tracepoint_reaction_ends(reaction, worker_number);

    // If the reaction produced outputs, put the resulting triggered
    // reactions into the queue or execute them immediately.
    schedule_output_reactions(reaction, worker_number);
}

/**
 * The main looping logic of each LF worker thread.
 * This function assumes the caller holds the mutex lock.
 * 
 * @param worker_number The number assigned to this worker thread
 */
void _lf_worker_do_work(int worker_number) {
    // Keep track of whether we have decremented the idle thread count.
    // Obtain a reaction from the scheduler that is ready to execute
    // (i.e., it is not blocked by concurrently executing reactions
    // that it depends on).
    // print_snapshot(); // This is quite verbose (but very useful in debugging reaction deadlocks).
    reaction_t* current_reaction_to_execute = NULL;
    while ((current_reaction_to_execute = 
            lf_sched_get_ready_reaction(worker_number)) 
            != NULL) {
        // Got a reaction that is ready to run.
        DEBUG_PRINT("Worker %d: Popped from reaction_q %s: "
                "is control reaction: %d, chain ID: %llu, and deadline %lld.", worker_number,
                current_reaction_to_execute->name,
                current_reaction_to_execute->is_a_control_reaction,
                current_reaction_to_execute->chain_id,
                current_reaction_to_execute->deadline);

        bool violation = _lf_worker_handle_violations(
            worker_number, 
            current_reaction_to_execute
        );

        if (!violation) {
            // Invoke the reaction function.
            _lf_worker_invoke_reaction(worker_number, current_reaction_to_execute);
        }

        DEBUG_PRINT("Worker %d: Done with reaction %s.",
                worker_number, current_reaction_to_execute->name);

        lf_sched_done_with_reaction(worker_number, current_reaction_to_execute);
    }
}

/**
 * Worker thread for the thread pool.
 * This acquires the mutex lock and releases it to wait for time to
 * elapse or for asynchronous events and also releases it to execute reactions.
 */
void* _lf_worker(void* arg) {
    lf_mutex_lock(&mutex);
    int worker_number = worker_thread_count++;
    LOG_PRINT("Worker thread %d started.", worker_number);
    lf_mutex_unlock(&mutex);

    _lf_worker_do_work(worker_number);

    lf_mutex_lock(&mutex);

    // This thread is exiting, so don't count it anymore.
    worker_thread_count--;

    if (worker_thread_count == 0) {
        // The last worker thread to exit will inform the RTI if needed.
        // Notify the RTI that there will be no more events (if centralized coord).
        // False argument means don't wait for a reply.
        send_next_event_tag(FOREVER_TAG, false);
    }

    lf_cond_signal(&event_q_changed);

    DEBUG_PRINT("Worker %d: Stop requested. Exiting.", worker_number);
    lf_mutex_unlock(&mutex);
    // timeout has been requested.
    return NULL;
}

/**
 * @brief Start worker threads.
 * 
 * @param number_of_threads Indicate how many worker threads should be started.
 */
void lf_worker_start(size_t number_of_threads) {
    LOG_PRINT("Starting %u worker threads.", _lf_number_of_threads);
    _lf_thread_ids = (lf_thread_t*)malloc(_lf_number_of_threads * sizeof(lf_thread_t));
    for (unsigned int i = 0; i < _lf_number_of_threads; i++) {
        lf_thread_create(&_lf_thread_ids[i], _lf_worker, NULL);
    }
}

/**
 * @brief Wait for worker threads to exit.
 * 
 * @return 0 on success, 1 otherwise.
 */
int lf_worker_wait_exit() {
    // Wait for the worker threads to exit.
    void* worker_thread_exit_status = NULL;
    DEBUG_PRINT("Number of threads: %d.", _lf_number_of_threads);
    int ret = 0;
    for (int i = 0; i < _lf_number_of_threads; i++) {
        int failure = lf_thread_join(_lf_thread_ids[i], &worker_thread_exit_status);
        if (failure) {
            error_print("Failed to join thread listening for incoming messages: %s", strerror(failure));
        }
        if (worker_thread_exit_status != NULL) {
            error_print("---- Worker %d reports error code %p", worker_thread_exit_status);
            ret = 1;
        }
    }

    if (ret == 0) {
        LOG_PRINT("---- All worker threads exited successfully.");
    }
    
    lf_sched_free();
    free(_lf_thread_ids);
}

