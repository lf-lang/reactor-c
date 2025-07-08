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
#include "modes.h" // Modal model support
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

#ifdef FEDERATED_DECENTRALIZED

/**
 * @brief Return the global STP offset on advancement of logical time for federated execution.
 * @deprecated Use lf_get_sta() instead.
 */
interval_t lf_get_stp_offset(void);

/**
 * @brief Return the global STA (safe to advance) offset for federated execution.
 * @ingroup Federated
 */
interval_t lf_get_sta(void);

/**
 * @brief Set the global STP offset on advancement of logical time for federated execution.
 * @param offset A non-negative time value to be applied as the STP offset.
 * @deprecated Use lf_set_sta() instead.
 */
void lf_set_stp_offset(interval_t offset);

/**
 * @brief Set the global STA (safe to advance) offset for federated execution.
 * @ingroup Federated
 *
 * @param offset A non-negative time value to be applied as the STA offset.
 */
void lf_set_sta(interval_t offset);

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
