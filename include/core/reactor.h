/**
 * @file reactor.h
 * @brief Definitions for the C target of Lingua Franca shared by threaded and unthreaded versions.
 * @ingroup API
 *
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @author Chris Gill
 * @author Mehrdad Niknami
 *
 * This header file defines functions that programmers use in the body of reactions for reading and
 * writing inputs and outputs and scheduling future events. Other functions that might be useful to
 * application programmers are also defined here.
 *
 * Many of these functions have macro wrappers defined in reaction_macros.h.
 */

#ifndef REACTOR_H
#define REACTOR_H

#include "lf_types.h"
#include "low_level_platform.h" // For lf_cond_t in lf_async_state_t.
#include "modes.h"              // Modal model support
#include "port.h"
#include "tag.h"   // Time-related functions.
#include "clock.h" // Time-related functions.
#include "tracepoint.h"
#include "util.h"

/**
 * @brief Macro to suppress warnings about unused variables.
 */
#define SUPPRESS_UNUSED_WARNING(x) (void)(x)

//////////////////////  Function Declarations  //////////////////////

/**
 * @brief Return true if the provided tag is after stop tag.
 * @ingroup API
 *
 * @param env Environment in which we are executing.
 * @param tag The tag to check against stop tag
 */
bool lf_is_tag_after_stop_tag(environment_t* env, tag_t tag);

/**
 * @brief Mark the given port's is_present field as true.
 * @ingroup API
 *
 * @param port A pointer to the port struct as an `lf_port_base_t*`.
 */
void lf_set_present(lf_port_base_t* port);

/**
 * @brief Set the stop tag if it is less than the stop tag of the specified environment.
 * @ingroup Internal
 *
 * @note In threaded programs, the environment's mutex must be locked before calling this function.
 */
void lf_set_stop_tag(environment_t* env, tag_t tag);

#ifdef FEDERATED

/**
 * @brief Set the federation ID of this federate.
 * @ingroup Federated
 *
 * @param fid The federation ID.
 */
void lf_set_federation_id(const char* fid);

/**
 * @brief Return the federation ID.
 * @ingroup Federated
 */
const char* lf_get_federation_id();

/**
 * @brief Return true if the RTI has reported failure.
 * @ingroup Federated
 *
 * Set by the RTI listener thread when it receives MSG_TYPE_FAILED.
 * The main thread uses this to return a nonzero status without calling
 * exit() from the listener.
 */
bool lf_rti_has_failed(void);

#endif // FEDERATED

#ifdef FEDERATED_DECENTRALIZED

/**
 * @brief Return the global STP offset on advancement of logical time for federated execution.
 * @deprecated Use lf_get_fed_maxwait() instead.
 */
interval_t lf_get_stp_offset(void);

/**
 * @brief Return the global STA (safe to advance) offset for federated execution.
 * @ingroup Federated
 * @deprecated Use lf_get_fed_maxwait() instead.
 */
interval_t lf_get_sta(void);

/**
 * @brief Return the global maxwait for the current federate.
 * @ingroup Federated
 */
interval_t lf_get_fed_maxwait(void);

/**
 * @brief Set the global STP offset on advancement of logical time for federated execution.
 * @param offset A non-negative time value to be applied as the STP offset.
 * @deprecated Use lf_set_fed_maxwait() instead.
 */
void lf_set_stp_offset(interval_t offset);

/**
 * @brief Set the global STA (safe to advance) offset for federated execution.
 * @ingroup Federated
 * @param offset A non-negative time value to be applied as the STA offset.
 * @deprecated Use lf_set_fed_maxwait() instead.
 */
void lf_set_sta(interval_t offset);

/**
 * @brief Set the global maxwait for the current federate.
 * @ingroup Federated
 * @param offset A non-negative time value to be applied as the maxwait.
 */
void lf_set_fed_maxwait(interval_t offset);

#endif // FEDERATED_DECENTRALIZED

/**
 * @brief Print a snapshot of the priority queues used during execution (for debugging).
 * @ingroup Internal
 *
 * This function implementation will be empty if the NDEBUG macro is defined; that macro
 * is normally defined for release builds.
 * @param env The environment in which we are executing, which you can access in a reaction
 *  body with `self->base.environment`.
 */
void lf_print_snapshot(environment_t* env);

/**
 * @brief Request a stop to execution as soon as possible.
 * @ingroup API
 *
 * In a non-federated execution with only a single enclave, this will occur
 * one microstep later than the current tag. In a federated execution or when
 * there is more than one enclave, it will likely occur at a later tag determined
 * by the RTI so that all federates and enclaves stop at the same tag.
 */
void lf_request_stop(void);

/**
 * @brief Add a worker thread to the pool of worker threads of the specified environment.
 * @ingroup API
 *
 * The new thread joins the pool immediately and starts executing reactions alongside the
 * existing workers. This can be used, for example, to compensate for a worker thread that
 * is about to block for an extended time inside a reaction. The thread remains in the pool
 * until the program terminates, when it is joined like any other worker thread.
 *
 * This function must be called from within a reaction body, i.e., from one of the worker
 * threads of the environment. In a reaction body, the environment is `self->base.environment`,
 * and the convenience macro `lf_add_worker_thread()` (with no arguments) supplies it.
 *
 * Adding worker threads is supported by the default (NP) and GEDF_NP schedulers in the
 * threaded runtime. The adaptive scheduler and the single-threaded runtime do not support
 * it, in which case this function returns -1 and no thread is created.
 *
 * @param env The environment to which to add a worker thread.
 * @return 0 on success, or -1 if the thread could not be created or the runtime does not
 *  support adding worker threads.
 */
int lf_add_worker_thread(environment_t* env);

/**
 * @brief Return the number of worker threads added to the specified environment.
 * @ingroup API
 *
 * This counts the worker threads that have been successfully added by calls to
 * @ref lf_add_worker_thread(). It does not include the worker threads created at startup.
 * In a reaction body, the environment is `self->base.environment`, and the convenience
 * macro `lf_added_worker_thread_count()` (with no arguments) supplies it.
 *
 * @param env The environment.
 * @return The number of worker threads added at runtime (0 in the single-threaded runtime).
 */
int lf_added_worker_thread_count(environment_t* env);

/**
 * @brief State of a pending `lf_async()` call.
 * @ingroup Internal
 *
 * An instance of this struct lives on the stack of the reaction that invoked `lf_async()`.
 * While the call is waiting to be resumed, the instance is linked into the list of pending
 * calls of the environment (see `environment_t::async_waiters`). All fields other than
 * `resumed_cond` are protected by the environment mutex once the instance is on that list.
 */
typedef struct lf_async_state_t {
  /** The reactor whose reaction invoked `lf_async()`. */
  self_base_t* self;
  /** The reaction that invoked `lf_async()`. Restored as the executing reaction upon resumption. */
  reaction_t* reaction;
  /**
   * The action that resumes this call, or NULL for `lf_await()`, which is resumed by
   * `lf_await_resume()`. A physical action is scheduled in `_lf_async_end()`.
   */
  lf_action_base_t* action;
  /** The input port passed to `lf_await()`, or NULL for `lf_async()`. */
  lf_port_base_t* port;
  /** Set to true when the call has been resumed. */
  bool resumed;
  /** Set to true in `_lf_async_end()`, once the invoked function has returned. */
  bool function_done;
  /** Set to true by `lf_async_resume()` when it is waiting for `function_done`. */
  bool resume_waiting;
  /**
   * True if the calling thread left the pool of worker threads. This is false if the program
   * was already stopping when the call began, in which case the call does not suspend at all.
   */
  bool left_pool;
  /** Next pending call in the environment, or NULL. */
  struct lf_async_state_t* next;
#if !defined(LF_SINGLE_THREADED)
  /** Signaled when `resumed` becomes true. Associated with the environment mutex. */
  lf_cond_t resumed_cond;
#endif
} lf_async_state_t;

/**
 * @brief Prepare to invoke a possibly blocking function from a reaction without blocking the environment.
 * @ingroup Internal
 *
 * This is the first half of the `lf_async()` macro and is not meant to be called directly.
 * It is called from a reaction body, by the worker thread executing the reaction, immediately
 * before the function is invoked. Unless the program is already stopping, it first propagates any
 * outputs that the reaction has set, so reactions triggered by those outputs run at the current tag
 * before logical time can advance. Those reactions are queued rather than executed in this thread
 * (see `schedule_output_reactions()`). If `action` is a logical action, it is scheduled at the
 * current tag plus its minimum delay, or one microstep later than the current tag if the minimum
 * delay is zero, before the function runs. Unless every thread previously
 * added by `lf_async()` is already compensating for another suspended call, it then adds a worker
 * thread to the pool of the environment (see @ref lf_add_worker_thread()). It then removes the calling
 * worker thread from the pool so that, while the function executes, the remaining workers can advance
 * logical time and execute other reactions as if the calling reaction had completed. The thread is
 * added before the caller leaves so that the pool is never empty; if all remaining workers are idle
 * when the caller leaves, the scheduler wakes one of them to advance the tag (see
 * @ref lf_sched_remove_worker()). The reactor mutex, which is created here if the reactor does not
 * already have one, is released. If the program is already stopping, none of this happens and
 * the function simply executes inline.
 *
 * @param state Storage for the state of the call, which must remain valid until `_lf_async_end()` returns.
 * @param self The self struct of the reactor whose reaction is executing.
 * @param action The logical or physical action (an `lf_action_base_t*`) on which @ref lf_async_resume()
 * will be called. A physical action is scheduled later, by `_lf_async_end()`. NULL for `lf_await()`.
 * @param port The input port (an `lf_port_base_t*`) passed to `lf_await()`. NULL for `lf_async()`.
 */
void _lf_async_begin(lf_async_state_t* state, void* self, void* action, void* port);

/**
 * @brief Complete a call prepared by `_lf_async_begin()`.
 * @ingroup Internal
 *
 * This is the second half of the `lf_async()` macro and is not meant to be called directly.
 * It is called immediately after the function has returned. If the action is physical, it is
 * scheduled now, even if this call has already been resumed. A logical action was already
 * scheduled by `_lf_async_begin()`. This function then
 * blocks until a reaction triggered by the action calls @ref lf_async_resume() on it (or until the
 * worker threads of the environment exit because the program is stopping). If that call happens
 * before the function returns, `lf_async_resume()` blocks until this function is called, so the
 * tag does not advance past it. The calling thread, which by then has rejoined the pool of worker
 * threads, reacquires the reactor mutex and this function returns, so that the remainder of the
 * calling reaction executes at the tag at which `lf_async_resume()` was called, after the resuming
 * reaction has completed.
 *
 * @param state The state passed to `_lf_async_begin()`.
 */
void _lf_async_end(lf_async_state_t* state);

/**
 * @brief Resume a reaction suspended in `lf_async()` that is waiting on the specified action.
 * @ingroup API
 *
 * This must be called from the body of a reaction triggered by the action passed to `lf_async()`,
 * and this reaction must belong to the same reactor as the action. For example:
 * ```
 * reaction(awake) {= lf_async_resume(awake); =}
 * ```
 * If several calls from the same reaction share this action, this resumes the oldest one. A
 * physical action cannot be shared with another reaction. If that call's function has not
 * yet returned, this function blocks until `_lf_async_end()` is called for it. The calling worker
 * is busy while it blocks, so logical time cannot advance and the suspended reaction resumes at
 * this tag. The suspended thread rejoins the pool of worker threads and, once this reaction has
 * completed, continues at the current tag. If no call is waiting on this action, this function
 * does nothing. Only this reaction incurs any cost; the runtime does not check for pending
 * `lf_async()` calls anywhere else.
 *
 * @param action The action, as it is in scope in the reaction body.
 */
void lf_async_resume(void* action);

/**
 * @brief Suspend the calling reaction until another reaction of the same reactor calls
 * `lf_await_resume()` on the specified input port.
 * @ingroup Internal
 *
 * This is the implementation of the `lf_await()` macro, which supplies `self`, and is not meant
 * to be called directly. It behaves like `lf_async()` with no function to invoke and no action. Outputs set before the
 * call are propagated at the current tag, the calling worker thread leaves the pool (adding a
 * replacement if needed), and logical time advances and other reactions execute as if the
 * calling reaction had completed. When a reaction of the same reactor calls `lf_await_resume()`
 * on the same port, this function returns, and the remainder of the calling reaction executes at
 * the tag of that call, after the resuming reaction has completed. Outputs set after the call are
 * produced at that tag.
 *
 * If the program stops before `lf_await_resume()` is called, this function returns at the stop
 * tag, after the shutdown reactions have executed, so that the calling reaction can complete.
 * If the program is already stopping when this is called, it returns immediately.
 *
 * The caveats of `lf_async()` apply: the calling reaction counts as executing while suspended,
 * state variables may be modified by other reactions in the meantime, the reactions of the
 * reactor become mutually exclusive, and this requires the threaded runtime with the NP or
 * GEDF_NP scheduler.
 *
 * @param self The self struct of the reactor whose reaction is executing.
 * @param port An input port of this reactor (an `lf_port_base_t*`), as it is in scope in the
 * reaction body. The calling reaction must list it as a trigger or source.
 */
void _lf_await(void* self, void* port);

/**
 * @brief Resume every reaction of this reactor that is suspended in `lf_await()` on the specified port.
 * @ingroup Internal
 *
 * This is the implementation of the `lf_await_resume()` macro, which supplies `self`, and is not
 * meant to be called directly. It must be called from the body of a reaction of the same reactor
 * as the suspended reactions, typically a reaction triggered by the port. All pending `lf_await()`
 * calls of this reactor on this port are resumed, oldest first. Each suspended thread rejoins the pool of worker
 * threads and, once this reaction has completed, continues at the current tag; the reactor mutex serializes them. If no
 * reaction of this reactor is suspended on this port, this function does nothing. Calls suspended in `lf_async()` or on
 * other ports are not affected.
 *
 * The port is identified by its address together with the reactor, because an input port that
 * receives from a fanned-out output shares its address with the other receivers.
 *
 * @param self The self struct of the reactor whose reaction is executing.
 * @param port The input port (an `lf_port_base_t*`), as it is in scope in the reaction body.
 */
void _lf_await_resume(void* self, void* port);

/**
 * @brief Allocate memory and record on the specified allocation record (a self struct).
 * @ingroup Internal
 *
 * This will allocate memory using calloc (so the allocated memory is zeroed out)
 * and record the allocated memory on the specified self struct so that
 * it will be freed when calling @ref lf_free_reactor().
 *
 * In a reaction body, you can access the head of the allocation records for the
 * current reactor with `&self->base.allocations`.
 *
 * @param count The number of items of size 'size' to accomodate.
 * @param size The size of each item.
 * @param head Pointer to the head of a list on which to record
 *  the allocation, or NULL to not record it (an `allocation_record_t**`).
 * @return A pointer to the allocated memory.
 */
void* lf_allocate(size_t count, size_t size, struct allocation_record_t** head);

/**
 * @brief Allocate memory for a new runtime instance of a reactor.
 * @ingroup Internal
 *
 * This records the reactor on the list of reactors to be freed at
 * termination of the program. If you plan to free the reactor before
 * termination of the program, use
 * {@link lf_allocate(size_t, size_t, allocation_record_t**)}
 * with a null last argument instead.
 *
 * @param size The size of the self struct, obtained with sizeof().
 */
self_base_t* lf_new_reactor(size_t size);

/**
 * @brief Free all the reactors that are allocated with {@link #lf_new_reactor(size_t)}.
 * @ingroup Internal
 */
void lf_free_all_reactors(void);

/**
 * @brief Free the specified reactor.
 * @ingroup Internal
 *
 * This will free the memory recorded on the allocations list of the specified reactor
 * and then free the specified self struct.
 * @param self The self struct of the reactor.
 */
void lf_free_reactor(self_base_t* self);

/**
 * @brief Return the instance name of the reactor.
 * @ingroup API
 *
 * The instance name is the name of given to the instance created by the `new` operator in LF.
 * If the instance is in a bank, then the name will have a suffix of the form `[bank_index]`.
 *
 * @param self The self struct of the reactor.
 */
const char* lf_reactor_name(self_base_t* self);

/**
 * @brief Return the full name of the reactor.
 * @ingroup API
 *
 * The fully qualified name of a reactor is the instance name of the reactor concatenated with the names of all
 * of its parents, separated by dots. If the reactor or any of its parents is a bank, then the name
 * will have a suffix of the form `[bank_index]`.
 *
 * @param self The self struct of the reactor.
 */
const char* lf_reactor_full_name(self_base_t* self);

#endif /* REACTOR_H */
