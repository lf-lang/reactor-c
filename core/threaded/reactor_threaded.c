/* Runtime infrastructure for the threaded version of the C target of Lingua Franca. */

/*************
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

/** Runtime infrastructure for the threaded version of the C target of Lingua Franca.
 *  
 *  @author{Edward A. Lee <eal@berkeley.edu>}
 *  @author{Marten Lohstroh <marten@berkeley.edu>}
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 */

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "../reactor_common.c"
#include "../platform.h"
#include "scheduler.h"
#include "worker.h"
#include "wait_until.h"
#include <signal.h>


/**
 * The maximum amount of time a worker thread should stall
 * before checking the reaction queue again.
 * This is not currently used.
 */
#define MAX_STALL_INTERVAL MSEC(1)

/*
 * A struct representing a barrier in threaded 
 * Lingua Franca programs that can prevent advancement 
 * of tag if
 * 1- Number of requestors is larger than 0
 * 2- Value of horizon is not (FOREVER, 0)
 */
typedef struct _lf_tag_advancement_barrier {
    int requestors; // Used to indicate the number of
                    // requestors that have asked
                    // for a barrier to be raised
                    // on tag.
    tag_t horizon;  // If semaphore is larger than 0
                    // then the runtime should not
                    // advance its tag beyond the
                    // horizon.
} _lf_tag_advancement_barrier;


/**
 * Create a global tag barrier and
 * initialize the barrier's semaphore to 0 and its horizon to FOREVER_TAG.
 */
_lf_tag_advancement_barrier _lf_global_tag_advancement_barrier = {0, FOREVER_TAG_INITIALIZER};

// The one and only mutex lock.
lf_mutex_t mutex;

// Condition variables used for notification between threads.
lf_cond_t event_q_changed;

// A condition variable that notifies threads whenever the number
// of requestors on the tag barrier reaches zero.
lf_cond_t global_tag_barrier_requestors_reached_zero;

/**
 * Enqueue network input control reactions that determine if the trigger for a
 * given network input port is going to be present at the current logical time
 * or absent.
 */
void enqueue_network_input_control_reactions();

/**
 * Enqueue network output control reactions that will send a PORT_ABSENT
 * message to downstream federates if a given network output port is not present.
 */
void enqueue_network_output_control_reactions();

/**
 * Raise a barrier to prevent the current tag from advancing to or
 * beyond the value of the future_tag argument, if possible.
 * If the current tag is already at or beyond future_tag, then
 * prevent any further advances. This function will increment the
 * total number of pending barrier requests. For each call to this
 * function, there should always be a subsequent call to
 * _lf_decrement_global_tag_barrier_locked()
 * to release the barrier.
 * 
 * If there is already a barrier raised at a tag later than future_tag, this
 * function will change the barrier to future_tag or the current tag, whichever
 * is larger. If the existing barrier is earlier 
 * than future_tag, this function will not change the barrier. If there are
 * no existing barriers and future_tag is in the past relative to the 
 * current tag, this function will raise a barrier to the current tag.
 * 
 * This function assumes the mutex lock is already held, thus, it will not
 * acquire it itself.
 * 
 * @note This function is only useful in threaded applications to facilitate
 *  certain non-blocking functionalities such as receiving timed messages
 *  over the network or handling stop in a federated execution.
 * 
 * @param future_tag A desired tag for the barrier. This function will guarantee
 * that current logical time will not go past future_tag if it is in the future.
 * If future_tag is in the past (or equals to current logical time), the runtime
 * will freeze advancement of logical time.
 */
void _lf_increment_global_tag_barrier_already_locked(tag_t future_tag) {
    // Check if future_tag is after stop tag.
    // This will only occur when a federate receives a timed message with 
    // a tag that is after the stop tag
    if (_lf_is_tag_after_stop_tag(future_tag)) {
        warning_print("Attempting to raise a barrier after the stop tag.");
        future_tag = stop_tag;
    }
    tag_t current_tag = get_current_tag();
    // Check to see if future_tag is actually in the future.
    if (compare_tags(future_tag, current_tag) > 0) {
        // Future tag is actually in the future.
        // See whether it is smaller than any pre-existing barrier.
        if (compare_tags(future_tag, _lf_global_tag_advancement_barrier.horizon) < 0) {
            // The future tag is smaller than the current horizon of the barrier.
            // Therefore, we should prevent logical time from reaching the
            // future tag.
            _lf_global_tag_advancement_barrier.horizon = future_tag;
            DEBUG_PRINT("Raised barrier at elapsed tag (%lld, %u).",
                        _lf_global_tag_advancement_barrier.horizon.time - start_time,
                        _lf_global_tag_advancement_barrier.horizon.microstep);
        } 
    } else {
            // The future_tag is not in the future.

            // One possibility is that the incoming message has violated the STP offset.
            // Another possibility is that the message is coming from a zero-delay loop,
            // and control reactions are waiting.

            // Prevent logical time from advancing further so that the measure of
            // STP violation properly reflects the amount of time (logical or physical)
            // that has elapsed after the incoming message would have violated the STP offset.
            _lf_global_tag_advancement_barrier.horizon = current_tag;
            _lf_global_tag_advancement_barrier.horizon.microstep++;
            DEBUG_PRINT("Raised barrier at elapsed tag (%lld, %u).",
                        _lf_global_tag_advancement_barrier.horizon.time - start_time,
                        _lf_global_tag_advancement_barrier.horizon.microstep);
    }
    // Increment the number of requestors
    _lf_global_tag_advancement_barrier.requestors++;
}

/**
 * Raise a barrier to prevent the current tag from advancing to or
 * beyond the value of the future_tag argument, if possible.
 * If the current tag is already at or beyond future_tag, then
 * prevent any further advances. This function will increment the
 * total number of pending barrier requests. For each call to this
 * function, there should always be a subsequent call to
 * _lf_decrement_global_tag_barrier_locked()
 * to release the barrier.
 * 
 * If there is already a barrier raised at a tag later than future_tag, this
 * function will change the barrier to future_tag or the current tag, whichever
 * is larger. If the existing barrier is earlier 
 * than future_tag, this function will not change the barrier. If there are
 * no existing barriers and future_tag is in the past relative to the 
 * current tag, this function will raise a barrier to the current tag.
 * 
 * This function acquires the mutex lock .
 * 
 * @note This function is only useful in threaded applications to facilitate
 *  certain non-blocking functionalities such as receiving timed messages
 *  over the network or handling stop in a federated execution.
 * 
 * @param future_tag A desired tag for the barrier. This function will guarantee
 * that current tag will not go past future_tag if it is in the future.
 * If future_tag is in the past (or equals to current tag), the runtime
 * will freeze advancement of tag.
 */
void _lf_increment_global_tag_barrier(tag_t future_tag) {
    lf_mutex_lock(&mutex);
    _lf_increment_global_tag_barrier_already_locked(future_tag);
    lf_mutex_unlock(&mutex);
}

/**
 * Decrement the total number of pending barrier requests for the global tag barrier.
 * If the total number of requests reaches zero, this function resets the
 * tag barrier to FOREVER_TAG and notifies all threads that are waiting
 * on the barrier that the number of requests has reached zero.
 * 
 * This function assumes that the caller already holds the mutex lock.
 * 
 * @note This function is only useful in threaded applications to facilitate
 *  certain non-blocking functionalities such as receiving timed messages
 *  over the network or handling stop in the federated execution.
 */
void _lf_decrement_global_tag_barrier_locked() {
    // Decrement the number of requestors for the tag barrier.
    _lf_global_tag_advancement_barrier.requestors--;
    // Check to see if the semaphore is negative, which indicates that
    // a mismatched call was placed for this function.
    if (_lf_global_tag_advancement_barrier.requestors < 0) {
        error_print_and_exit("Mismatched use of _lf_increment_global_tag_barrier()"
                " and  _lf_decrement_global_tag_barrier_locked().");
    } else if (_lf_global_tag_advancement_barrier.requestors == 0) {
        // When the semaphore reaches zero, reset the horizon to forever.
        _lf_global_tag_advancement_barrier.horizon = FOREVER_TAG;
        // Notify waiting threads that the semaphore has reached zero.
        lf_cond_broadcast(&global_tag_barrier_requestors_reached_zero);
    }
    DEBUG_PRINT("Barrier is at tag (%lld, %u).",
                 _lf_global_tag_advancement_barrier.horizon.time,
                 _lf_global_tag_advancement_barrier.horizon.microstep);
}

/**
 * If the proposed_tag is greater than or equal to a barrier tag that has been
 * set by a call to _lf_increment_global_tag_barrier or
 * _lf_increment_global_tag_barrier_already_locked, and if there are requestors
 * still pending on that barrier, then wait until all requestors have been
 * satisfied. This is used in federated execution when an incoming timed
 * message has been partially read so that we know its tag, but the rest of
 * message has not yet been read and hence the event has not yet appeared
 * on the event queue.  To prevent tardiness, this function blocks the
 * advancement of time until to the proposed tag until the message has
 * been put onto the event queue.
 *
 * If the prposed_tag is greater than the stop tag, then use the stop tag instead.
 * 
 * This function assumes the mutex is already locked.
 * Thus, it unlocks the mutex while it's waiting to allow
 * the tag barrier to change.
 * 
 * @param proposed_tag The tag that the runtime wants to advance to.
 * @return 0 if no wait was needed and 1 if a wait actually occurred.
 */
int _lf_wait_on_global_tag_barrier(tag_t proposed_tag) {
    // Check the most common case first.
    if (_lf_global_tag_advancement_barrier.requestors == 0) return 0;
    
    // Do not wait for tags after the stop tag
    if (_lf_is_tag_after_stop_tag(proposed_tag)) {
        proposed_tag = stop_tag;
    }
    // Do not wait forever
    if (proposed_tag.time == FOREVER) {
        warning_print("Global tag barrier should not handle FOREVER proposed tags.");
        return 0;
    }
    int result = 0;
    // Wait until the global barrier semaphore on logical time is zero
    // and the proposed_time is larger than or equal to the horizon.
    while (_lf_global_tag_advancement_barrier.requestors > 0
            && compare_tags(proposed_tag, _lf_global_tag_advancement_barrier.horizon) >= 0
    ) {
        result = 1;
        LOG_PRINT("Waiting on barrier for tag (%lld, %u).", proposed_tag.time - start_time, proposed_tag.microstep);
        // Wait until no requestor remains for the barrier on logical time
        lf_cond_wait(&global_tag_barrier_requestors_reached_zero, &mutex);
        
        // The stop tag may have changed during the wait.
        if (_lf_is_tag_after_stop_tag(proposed_tag)) {
            proposed_tag = stop_tag;
        }
    }
    return result;
}

/**
 * Schedule the specified trigger at current_tag.time plus the offset of the
 * specified trigger plus the delay.
 * See reactor.h for documentation.
 */
trigger_handle_t _lf_schedule_token(void* action, interval_t extra_delay, lf_token_t* token) {
    trigger_t* trigger = _lf_action_to_trigger(action);
    lf_mutex_lock(&mutex);
    int return_value = _lf_schedule(trigger, extra_delay, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_cond_broadcast(&event_q_changed);
    lf_mutex_unlock(&mutex);
    return return_value;
}

/**
 * Schedule an action to occur with the specified value and time offset
 * with a copy of the specified value.
 * See reactor.h for documentation.
 */
trigger_handle_t _lf_schedule_copy(void* action, interval_t offset, void* value, size_t length) {
    if (value == NULL) {
        return _lf_schedule_token(action, offset, NULL);
    }
    trigger_t* trigger = _lf_action_to_trigger(action);

    if (trigger == NULL || trigger->token == NULL || trigger->token->element_size <= 0) {
        error_print("schedule: Invalid trigger or element size.");
        return -1;
    }
    lf_mutex_lock(&mutex);
    // Initialize token with an array size of length and a reference count of 0.
    lf_token_t* token = _lf_initialize_token(trigger->token, length);
    // Copy the value into the newly allocated memory.
    memcpy(token->value, value, token->element_size * length);
    // The schedule function will increment the reference count.
    trigger_handle_t result = _lf_schedule(trigger, offset, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_cond_signal(&event_q_changed);
    lf_mutex_unlock(&mutex);
    return result;
}

/**
 * Variant of schedule_token that creates a token to carry the specified value.
 * See reactor.h for documentation.
 */
trigger_handle_t _lf_schedule_value(void* action, interval_t extra_delay, void* value, size_t length) {
    trigger_t* trigger = _lf_action_to_trigger(action);

    lf_mutex_lock(&mutex);
    lf_token_t* token = create_token(trigger->element_size);
    token->value = value;
    token->length = length;
    int return_value = _lf_schedule(trigger, extra_delay, token);
    // Notify the main thread in case it is waiting for physical time to elapse.
    lf_cond_signal(&event_q_changed);
    lf_mutex_unlock(&mutex);
    return return_value;
}

/*
 * Mark the given is_present field as true. This is_present field
 * will later be cleaned up by _lf_start_time_step.
 * This assumes that the mutex is not held.
 * @param is_present_field A pointer to the is_present field that
 * must be set.
 */
void _lf_set_present(bool* is_present_field) {
    int ipfas = lf_atomic_fetch_add(&_lf_is_present_fields_abbreviated_size, 1);
    if (ipfas < _lf_is_present_fields_size) {
        _lf_is_present_fields_abbreviated[ipfas] = is_present_field;
    }
    *is_present_field = true;
}

/** 
 * Synchronize the start with other federates via the RTI.
 * This assumes that a connection to the RTI is already made 
 * and _fed.socket_TCP_RTI is valid. It then sends the current logical
 * time to the RTI and waits for the RTI to respond with a specified
 * time. It starts a thread to listen for messages from the RTI.
 * It then waits for physical time to match the specified time,
 * sets current logical time to the time returned by the RTI,
 * and then returns. If --fast was specified, then this does
 * not wait for physical time to match the logical start time
 * returned by the RTI.
 */
void synchronize_with_other_federates();

/**
 * Request a stop to execution as soon as possible.
 * In a non-federated execution, this will occur
 * at the conclusion of the current logical time.
 * In a federated execution, it will likely occur at
 * a later logical time determined by the RTI so that
 * all federates stop at the same logical time.
 */
void request_stop() {
    lf_mutex_lock(&mutex);
    // Check if already at the previous stop tag.
    if (compare_tags(current_tag, stop_tag) >= 0) {
        // If so, ignore the stop request since the program
        // is already stopping at the current tag.
        lf_mutex_unlock(&mutex);
        return;
    }
#ifdef FEDERATED
    _lf_fd_send_stop_request_to_rti();
    // Do not set stop_requested
    // since the RTI might grant a
    // later stop tag than the current
    // tag. The _lf_fd_send_request_stop_to_rti() 
    // will raise a barrier at the current
    // logical time.
#else
    // In a non-federated program, the stop_tag will be the next microstep
    _lf_set_stop_tag((tag_t) {.time = current_tag.time, .microstep = current_tag.microstep+1});
    // We signal instead of broadcast under the assumption that only
    // one worker thread can call wait_until at a given time because
    // the call to wait_until is protected by a mutex lock
    lf_cond_signal(&event_q_changed);
#endif
    lf_mutex_unlock(&mutex);
}

/**
 * Put the specified reaction eventually on the reaction queue. There is no
 * guarantee that this happens immediately, unless 'worker_number' is -1.
 * This version acquires a mutex lock.
 * @param reaction The reaction.
 * @param worker_number The ID of the worker that is making a call. 0 should be
 *  used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 should be used if putting the reaction on the
 *  reaction queue should be done immediately.
 */
void _lf_enqueue_reaction(reaction_t* reaction, int worker_number) {
    lf_sched_trigger_reaction(reaction, worker_number);
}

/**
 * Perform the necessary operations before tag (0,0) can be processed.
 * 
 * This includes injecting any reactions triggered at (0,0), initializing timers,
 * and for the federated execution, waiting for a proper coordinated start.
 *
 * This assumes the mutex lock is held by the caller.
 */
void _lf_initialize_start_tag() {
    // Initialize the scheduler
    lf_sched_init((size_t)_lf_number_of_threads);

    // Add reactions invoked at tag (0,0) (including startup reactions) to the reaction queue
    _lf_trigger_startup_reactions(); 

#ifdef FEDERATED
    // Reset status fields before talking to the RTI to set network port
    // statuses to unknown
    reset_status_fields_on_input_port_triggers();

    // Get a start_time from the RTI
    synchronize_with_other_federates(); // Resets start_time in federated execution according to the RTI.
    current_tag = (tag_t){.time = start_time, .microstep = 0u};
#endif

    _lf_initialize_timers();

    // If the stop_tag is (0,0), also insert the shutdown
    // reactions. This can only happen if the timeout time
    // was set to 0.
    if (compare_tags(current_tag, stop_tag) >= 0) {
        _lf_trigger_shutdown_reactions();
    }

#ifdef FEDERATED
    // Insert network dependent reactions for network input ports into
    // the reaction queue to prevent reactions from executing at (0,0)
    // incorrectly.
    // At (0,0), events are not currently handled through the event_q.
    // Instead, any reaction triggered by an event at (0,0) (e.g., by
    // a startup event) is directly inserted into the reaction_q. 
    // On the other hand, the typical NET/TAG procedure of the centralized
    // coordination uses events coming off of the event queue. With the
    // current implementation, therefore, federates will send a NET(0,0)
    // only if they have timers and startup events. If a federate has no
    // initial event at (0,0), and later receives a message with intended
    // tag of (0,0), it will not send a NET. For now, the best remedy for
    // this seems to be to insert control reactions for all federates at
    // (0,0), which causes all federates, even those without any startup
    // or timer events at (0,0) to wait on all of their input ports and send
    // an absent message on all of their output ports. This inadvertantly causes
    // extra messages going back and forth for (0,0).
    enqueue_network_input_control_reactions();
    enqueue_network_output_control_reactions();

    // Call wait_until if federated. This is required because the startup procedure
    // in synchronize_with_other_federates() can decide on a new start_time that is 
    // larger than the current physical time.
    // Therefore, if --fast was not specified, wait until physical time matches
    // or exceeds the start time. Microstep is ignored.  
    // This wait_until() is deliberately called after most precursor operations
    // for tag (0,0) are performed (e.g., injecting startup reactions, etc.).
    // This has two benefits: First, the startup overheads will reduce 
    // the required waiting time. Second, this call releases the mutex lock and allows
    // other threads (specifically, federate threads that handle incoming p2p messages 
    // from other federates) to hold the lock and possibly raise a tag barrier. This is 
    // especially useful if an STP offset is set properly because the federate will get
    // a chance to process incoming messages while utilizing the STP offset.
    LOG_PRINT("Waiting for start time %lld plus STP offset %lld.",
            start_time, _lf_global_time_STP_offset);
    // Ignore interrupts to this wait. We don't want to start executing until
    // physical time matches or exceeds the logical start time.
    while (!wait_until(start_time, &event_q_changed, fast)) {}
    DEBUG_PRINT("Done waiting for start time %lld.", start_time);
    DEBUG_PRINT("Physical time is ahead of current time by %lld. This should be small.",
            get_physical_time() - start_time);

    // Reinitialize the physical start time to match the start_time.
    // Otherwise, reports of get_elapsed_physical_time are not very meaningful
    // w.r.t. logical time.
    physical_start_time = start_time;
#endif

#ifdef FEDERATED_DECENTRALIZED
    // In federated execution (at least under decentralized coordination),
    // it is possible that an incoming message has been partially read at (0,0),
    // enough to see its tag. To prevent it from becoming tardy, the thread
    // that is reading the message has set a barrier to prevent logical time
    // from exceeding the timestamp of the message. It will remove that barrier
    // once the complete message has been read. Here, we wait for that barrier
    // to be removed, if appropriate before proceeding to executing tag (0,0).
    _lf_wait_on_global_tag_barrier((tag_t){.time=start_time,.microstep=0});
#endif // FEDERATED_DECENTRALIZED
    
    // Set the following boolean so that other thread(s), including federated threads,
    // know that the execution has started
    _lf_execution_started = true;
}

/**
 * If DEBUG logging is enabled, prints the status of the event queue,
 * the reaction queue, and the executing queue.
 */
void print_snapshot() {
    if(LOG_LEVEL > LOG_LEVEL_LOG) {
        DEBUG_PRINT(">>> START Snapshot");
        DEBUG_PRINT("Pending:");
        // pqueue_dump(reaction_q, print_reaction); FIXME: reaction_q is not
        // accessible here
        DEBUG_PRINT("Event queue size: %d. Contents:",
                        pqueue_size(event_q));
        pqueue_dump(event_q, print_reaction); 
        DEBUG_PRINT(">>> END Snapshot");
    }
}

/**
 * The main loop of the LF program.
 * 
 * An unambiguous function name that can be called
 * by external libraries.
 * 
 * Note: In target languages that use the C core library,
 * there should be an unambiguous way to execute the LF
 * program's main function that will not conflict with
 * other main functions that might get resolved and linked
 * at compile time.
 */
int lf_reactor_c_main(int argc, char* argv[]) {
    // Invoke the function that optionally provides default command-line options.
    _lf_set_default_command_line_options();
    
    // Initialize the one and only mutex to be recursive, meaning that it is OK
    // for the same thread to lock and unlock the mutex even if it already holds
    // the lock.
    // FIXME: This is dangerous. The docs say this: "It is advised that an
    // application should not use a PTHREAD_MUTEX_RECURSIVE mutex with
    // condition variables because the implicit unlock performed for a
    // pthread_cond_wait() or pthread_cond_timedwait() may not actually
    // release the mutex (if it had been locked multiple times).
    // If this happens, no other thread can satisfy the condition
    // of the predicate.”  This seems like a bug in the implementation of
    // pthreads. Maybe it has been fixed?
    // The one and only mutex lock.
    lf_mutex_init(&mutex);

    // Initialize condition variables used for notification between threads.
    lf_cond_init(&event_q_changed);
    lf_cond_init(&global_tag_barrier_requestors_reached_zero);

    if (atexit(termination) != 0) {
        warning_print("Failed to register termination function!");
    }
    // The above handles only "normal" termination (via a call to exit).
    // As a consequence, we need to also trap ctrl-C, which issues a SIGINT,
    // and cause it to call exit.
    signal(SIGINT, exit);
#ifdef SIGPIPE
    // Ignore SIGPIPE errors, which terminate the entire application if
    // socket write() fails because the reader has closed the socket.
    // Instead, cause an EPIPE error to be set when write() fails.
    signal(SIGPIPE, SIG_IGN);
#endif // SIGPIPE

    if (process_args(default_argc, default_argv)
            && process_args(argc, argv)) {
        lf_mutex_lock(&mutex); // Sets start_time
        initialize();

        // Call the following function only once, rather than per worker thread (although 
        // it can be probably called in that manner as well).
        _lf_initialize_start_tag();

        lf_worker_start(_lf_number_of_threads);

        lf_mutex_unlock(&mutex);
        DEBUG_PRINT("Waiting for worker threads to exit.");

        int ret = lf_worker_wait_exit();

        return ret;
    } else {
        return -1;
    }
}


int main(int argc, char* argv[]) {
    return lf_reactor_c_main(argc, argv);
}
