#ifndef RTI_LOCAL_H
#define RTI_LOCAL_H

#ifdef LF_ENCLAVES


#include "lf_types.h"
#include "rti_common.h"

/**
 * @brief Structure holding information about each enclave in the program
 * The first field is the generic scheduling_node_info struct
 * 
 */
typedef struct enclave_info_t {
    scheduling_node_t base;
    environment_t * env; // A pointer to the environment of the enclave
    lf_cond_t next_event_condition; // Condition variable used by scheduling_nodes to notify an enclave
                                    // that it's call to next_event_tag() should unblock.
} enclave_info_t;

/**
 * @brief Structure holding information about the local RTI
 * 
 */
typedef struct {
    rti_common_t base;
} rti_local_t;

/**
 * @brief Dynamically create and initialize the local RTI.
 */
void initialize_local_rti(environment_t* envs, int num_envs);

/**
 * @brief Free memory associated with the local the RTI and the local RTI iself.
 */
void free_local_rti();

/**
 * @brief Initialize the enclave object.
 * 
 * @param enclave 
 */
void initialize_enclave_info(enclave_info_t* enclave, int idx, environment_t *env);

/**
 * @brief Notify the local RTI of a next event tag (NET).
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
 * @brief This function informs the local RTI that `enclave` has completed tag
 * `completed`. This will update the data structures and can release other
 * enclaves waiting on a TAG.
 * 
 * This assumes the caller is holding the environment mutex of the source enclave.
 * 
 * @param enclave The enclave
 * @param completed The tag just completed by the enclave.
 */
void rti_logical_tag_complete_locked(enclave_info_t* enclave, tag_t completed);

/**
 * @brief This function is called after scheduling an event onto the event queue
 * of another enclave. The source enclave must call this function to potentially update
 * the NET of the target enclave. 
 * 
 * This assumes the caller is holding the environment mutex of the target enclave.
 * 
 * @param target The enclave of which we want to update the NET of
 * @param net The proposed next event tag
 */
void rti_update_other_net_locked(enclave_info_t* src, enclave_info_t* target, tag_t net);

#endif // LF_ENCLAVES
#endif // RTI_LOCAL_H
