#ifndef RTI_LOCAL_H
#define RTI_LOCAL_H

#include "rti_common.h"

/**
 * This is the RTI coordination API facing the threaded reactor runtime.
 */


/**
 * @brief Structure holding information about each enclave in the program
 * The first field is the generic reactor_node_info struct
 * 
 */
typedef struct enclave_info_t {
    reactor_node_info_t reactor;
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
void initialize_local_rti();

/**
 * @brief Initialize the enclave object
 * 
 * @param enclave 
 */
void initialize_enclave_info(enclave_info_t* enclave);

tag_advance_grant_t rti_next_event_tag(reactor_node_info_t* e, tag_t next_event_tag);

void rti_logical_tag_complete(reactor_node_info_t* enclave, tag_t completed);

#endif