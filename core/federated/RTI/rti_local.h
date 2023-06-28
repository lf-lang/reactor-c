#ifdef LF_ENCLAVES
#ifndef RTI_LOCAL_H
#define RTI_LOCAL_H


#include "lf_types.h"
#include "rti_common.h"

/**
 * @brief Structure holding information about each enclave in the program
 * The first field is the generic reactor_node_info struct
 * 
 */
typedef struct enclave_info_t {
    scheduling_node_t base;
    environment_t * env; // A pointer to the environment of the enclave
    lf_cond_t next_event_condition; // Condition variable used by reactor_nodes to notify an enclave
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
 * @brief Dynamically create and initialize the local RTI
 * 
 */
void initialize_local_rti(environment_t* envs, int num_envs);

/**
 * @brief Initialize the enclave object
 * 
 * @param enclave 
 */
void initialize_enclave_info(enclave_info_t* enclave, int idx, environment_t *env);

/**
 * @brief Get the tag to advance to.
 *
 * An enclave should call this function when it is ready to advance its tag,
 * passing as the second argument the tag of the earliest event on its event queue.
 * The returned tag may be less than or equal to the argument tag and is interpreted
 * by the enclave as the tag to which it can advance.
 * 
 * This will also notify downstream reactor_nodes with a TAG or PTAG if appropriate,
 * possibly unblocking their own calls to this same function.
 *
 * @param e The enclave.
 * @param next_event_tag The next event tag for e.
 * @return If granted, return the TAG and whether it is provisional or not. 
 *  Otherwise, return the NEVER_TAG.
 */
tag_t rti_next_event_tag_locked(enclave_info_t* e, tag_t next_event_tag);

/**
 * @brief This function informs the local RTI that `enclave` has completed tag
 * `completed`. This will update the data structures and can release other
 * enclaves waiting on a TAG.
 * 
 * @param enclave 
 * @param completed 
 */
void rti_logical_tag_complete_locked(enclave_info_t* enclave, tag_t completed);

/**
 * @brief This function is called to request stopping the execution at a certain tag.
 * 
 * 
 * @param stop_tag 
 */
void rti_request_stop_locked(enclave_info_t* enclave, tag_t tag);

/**
 * @brief This functions is called after scheduling an event onto the event queue
 * of another enclave. The source enclave must call this function to potentially update
 * the NET of the target enclave. 
 * 
 * FIXME: This replicates the functionality that the remote RTI has to update the NEt
 * when a timed message flows through it. Consider refactoring the rti_remote into rti_common.
 * Or have rti_remote use this rti_local API
 * 
 * This function is called while holding the environment mutex of the target enclave
 * @param target The enclave of which we want to update the NET of
 * @param net The proposed next event tag
 */
void rti_update_other_net_locked(enclave_info_t* src, enclave_info_t* target, tag_t net);

#endif
#endif
