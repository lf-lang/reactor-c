#ifndef RTI_LOCAL_H
#define RTI_LOCAL_H

#include "rti_common.h"

/**
 * @brief Structure holding information about each enclave in the program
 * The first field is the generic reactor_node_info struct
 * 
 */
typedef struct enclave_info_t {
    reactor_node_info_t reactor;
    environment_t * env; // A pointer to the environment of the enclave
    lf_cond_t next_event_condition; // Condition variable used by reactor_nodes to notify an enclave
                                    // that it's call to next_event_tag() should unblock.
};

/**
 * @brief Structure holding information about the local RTI
 * 
 */
typedef struct rti_local_t {
    rti_common_t base;
};

/**
 * @brief Dynamically create and initialize the local RTI
 * 
 */
void initialize_local_rti(environment_t ** envs, int num_envs);

/**
 * @brief Initialize the enclave object
 * 
 * @param enclave 
 */
void initialize_enclave_info(enclave_info_t* enclave, environment_t *env);

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
tag_advance_grant_t rti_next_event_tag(reactor_node_info_t* e, tag_t next_event_tag);

/**
 * @brief This function informs the local RTI that `enclave` has completed tag
 * `completed`. This will update the data structures and can release other
 * enclaves waiting on a TAG.
 * 
 * @param enclave 
 * @param completed 
 */
void rti_logical_tag_complete(enclave_info_t* enclave, tag_t completed);

/**
 * @brief This function is called to request stopping the execution at a certain tag.
 * 
 * 
 * @param stop_tag 
 */
void rti_request_stop(tag_t stop_tag);


#endif
