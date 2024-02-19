/**
 * @file
 * @author Erling Jellum (erling.r.jellum@ntnu.no)
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Chadlia Jerad (chadlia.jerad@ensi-uma.tn)
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @copyright (c) 2020-2024, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * 
 * @brief This file declares functions used to implement scheduling enclaves.
 * 
 * A scheduling enclave is portion of the runtime system that maintains its own event
 * and reaction queues and has its own scheduler. It uses a local runtime infrastructure (RTI)
 * to coordinate the advancement of tags across enclaves.
 */

#ifndef RTI_LOCAL_H
#define RTI_LOCAL_H

#ifdef LF_ENCLAVES


#include "lf_types.h"
#include "rti_common.h"

/**
 * @brief Structure holding information about each enclave in the program.
 * 
 * The first field is the generic scheduling_node_info struct
 */
typedef struct enclave_info_t {
    scheduling_node_t base;
    environment_t * env; // A pointer to the environment of the enclave
    lf_cond_t next_event_condition; // Condition variable used by scheduling_nodes to notify an enclave
                                    // that it's call to next_event_tag() should unblock.
} enclave_info_t;

/**
 * @brief Structure holding information about the local RTI
 */
typedef struct {
    rti_common_t base;
} rti_local_t;

/**
 * @brief Dynamically create and initialize the local RTI.
 * @param envs Array of environments.
 * @param num_envs Number of environments.
 */
void initialize_local_rti(environment_t* envs, int num_envs);

/**
 * @brief Free memory associated with the local the RTI and the local RTI iself.
 */
void free_local_rti();

/**
 * @brief Initialize the enclave object.
 * @param enclave The enclave object to initialize.
 * @param idx The index of the enclave.
 * @param env The environment of the enclave.
 */
void initialize_enclave_info(enclave_info_t* enclave, int idx, environment_t *env);

/**
 * @brief Notify the local RTI of a next event tag (NET).
 * 
 * This function call may block. A call to this function serves two purposes. 
 * 1) It is a promise that, unless receiving events from other enclaves, this
 * enclave will not produce any event until the next_event_tag (NET) argument.
 * 2) It is a request for permission to advance the logical tag of the enclave
 * until the NET.
 * 
 * This function call will block until the enclave has been granted a TAG,
 * which might not be the tag requested.
 * 
 * This assumes the caller is holding the environment mutex of the source enclave.
 * 
 * @param enclave The enclave requesting to advance to the NET.
 * @param next_event_tag The tag of the next event in the enclave
 * @return tag_t A tag which the enclave can safely advance its time to. It 
 * might be smaller than the requested tag.
 */
tag_t rti_next_event_tag_locked(enclave_info_t* enclave, tag_t next_event_tag);

/**
 * @brief Inform the local RTI that `enclave` has completed tag `completed`.
 * 
 * This will update the data structures and can release other
 * enclaves waiting on a TAG.
 * 
 * This assumes the caller is holding the environment mutex of the source enclave.
 * 
 * @param enclave The enclave
 * @param completed The tag just completed by the enclave.
 */
void rti_logical_tag_complete_locked(enclave_info_t* enclave, tag_t completed);

/**
 * @brief Notify the local RTI to update the next event tag (NET) of a target enclave.
 * 
 * This function is called after scheduling an event onto the event queue of another enclave. 
 * The source enclave must call this function to potentially update
 * the NET of the target enclave. 
 * 
 * This assumes the caller is holding the environment mutex of the target enclave.
 * 
 * @param src The enclave that has scheduled an event.
 * @param target The enclave of which we want to update the NET of.
 * @param net The proposed next event tag.
 */
void rti_update_other_net_locked(enclave_info_t* src, enclave_info_t* target, tag_t net);

/**
 * @brief Get the array of ids of enclaves directly upstream of the specified enclave.
 * 
 * This updates the specified result pointer to point to a statically allocated array of IDs
 * and returns the length of the array. The implementation is code-generated.
 * 
 * @param enclave_id The enclave for which to report upstream IDs.
 * @param result The pointer to dereference and update to point to the resulting array.
 * @return The number of direct upstream enclaves.
 */
int lf_get_upstream_of(int enclave_id, int** result);

/**
 * @brief Get the array of ids of enclaves directly downstream of the specified enclave.
 * 
 * This updates the specified result pointer to point to a statically allocated array of IDs
 * and returns the length of the array. The implementation is code-generated.
 * 
 * @param enclave_id The enclave for which to report downstream IDs.
 * @param result The pointer to dereference and update to point to the resulting array.
 * @return The number of direct downstream enclaves.
 */
int lf_get_downstream_of(int enclave_id, int** result);

/**
 * @brief Retrieve the delays on the connections to direct upstream enclaves.
 * 
 * This updates the result pointer to point to a statically allocated array of delays.
 * The implementation is code-generated.
 * 
 * @param enclave_id The enclave for which to search for upstream delays.
 * @param result The pointer to dereference and update to point to the resulting array.
 * @return int The number of direct upstream enclaves.
 */
int lf_get_upstream_delay_of(int enclave_id, interval_t** result);

#endif // LF_ENCLAVES
#endif // RTI_LOCAL_H
