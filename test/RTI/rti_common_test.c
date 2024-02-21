#if defined STANDALONE_RTI
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rti_remote.h"
#include "rti_common.h"
#include "tag.h"

// The RTI under test.
static rti_common_t test_rti;

/******************************************Start of Utility Functions******************************************************/

/**
 * Free dynamically allocated memory on the scheduling node.
 * @param node The node to be freed
*/
void delete_scheduling_node(scheduling_node_t* node) {
  if (node->immediate_upstreams != NULL) {
    free(node->immediate_upstreams);
  }
  if (node->immediate_upstream_delays != NULL) {
    free(node->immediate_upstream_delays);
  }
  if (node->immediate_downstreams != NULL) {
    free(node->immediate_downstreams);
  }
  invalidate_min_delays_upstream(node);
}

/**
 * Set the parameters of scheduling node to construct the test case.
 * Before calling this function, reset_common_RTI should be called to
 * reset every scheduling nodes.
 * @param id The ID of the scheduling node.
 * @param num_immediate_upstreams The number of upstreams of the scheduling node.
 * @param num_immediate_downstreams The number of downstreams of the scheduling node.
 * @param immediate_upstreams The array of IDs from immediate upstream nodes.
 * @param immediate_upstream_delays The array of delays from immediate upstream nodes.
 * @param immediate_downstreams The array of IDs from immediate downstream nodes.
*/
void set_scheduling_node(
    int id, 
    int num_immediate_upstreams, 
    int num_immediate_downstreams, 
    int* immediate_upstreams, 
    interval_t* immediate_upstream_delays, 
    int* immediate_downstreams) {
  // Save the number of immediate upstream and immediate downstream nodes.
  test_rti.scheduling_nodes[id]->num_immediate_upstreams = num_immediate_upstreams;
  test_rti.scheduling_nodes[id]->num_immediate_downstreams = num_immediate_downstreams;

  // If there is any immediate upstream nodes, store IDs and delays from the upstream nodes into the structure.
  if (test_rti.scheduling_nodes[id]->num_immediate_upstreams > 0) {
    test_rti.scheduling_nodes[id]->immediate_upstreams = (uint16_t*) calloc(test_rti.scheduling_nodes[id]->num_immediate_upstreams, sizeof(uint16_t));
    test_rti.scheduling_nodes[id]->immediate_upstream_delays = (interval_t*) calloc(test_rti.scheduling_nodes[id]->num_immediate_upstreams, sizeof(interval_t));
    for (int i = 0; i < test_rti.scheduling_nodes[id]->num_immediate_upstreams; i++) {
      test_rti.scheduling_nodes[id]->immediate_upstreams[i] = immediate_upstreams[i];
      test_rti.scheduling_nodes[id]->immediate_upstream_delays[i] = immediate_upstream_delays[i];
    }
  }
  // If there is any immediate downstream nodes, store IDs of the downstream nodes into the structure.
  if (test_rti.scheduling_nodes[id]->num_immediate_downstreams > 0) {
    test_rti.scheduling_nodes[id]->immediate_downstreams = (uint16_t*) calloc(test_rti.scheduling_nodes[id]->num_immediate_downstreams, sizeof(uint16_t));
    for (int i = 0; i < test_rti.scheduling_nodes[id]->num_immediate_downstreams; i++) {
      test_rti.scheduling_nodes[id]->immediate_downstreams[i] = immediate_downstreams[i];
    }
  }
}

/**
 * Reset the RTI to re-construct the structure of nodes.
 * This includes freeing every scheduling node and the array of nodes.
*/
void reset_common_RTI() {
  // For every scheduling nodes, delete them and free themselves, too.
  for (uint16_t i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
    delete_scheduling_node(test_rti.scheduling_nodes[i]);
    free(test_rti.scheduling_nodes[i]);
  }
  // Free the array of scheduling nodes either. This will be re-created
  // in set_common_RTI().
  if (test_rti.scheduling_nodes != NULL) {
    free(test_rti.scheduling_nodes);
  }
}

/**
 * Set the number of nodes and create an array for scheduling nodes.
 * This includes resetting the previous RTI.
 * @param num_nodes The number of scheduling nodes.
*/
void set_common_RTI(uint16_t num_nodes) {
  reset_common_RTI();

  test_rti.number_of_scheduling_nodes = num_nodes;

  // Allocate memory for the scheduling nodes
  test_rti.scheduling_nodes = (scheduling_node_t**)calloc(test_rti.number_of_scheduling_nodes, sizeof(scheduling_node_t*));
  test_rti.min_delays = (tag_t*)calloc((num_nodes*num_nodes), sizeof(tag_t));
  for (uint16_t i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
      scheduling_node_t *scheduling_node = (scheduling_node_t *) malloc(sizeof(scheduling_node_t));
      initialize_scheduling_node(scheduling_node, i);
      test_rti.scheduling_nodes[i] = scheduling_node;
  }
}

/**
 * Set the state of every scheduling node. The state can be NOT_CONNECTED, GRANTED,
 * or PENDING.
 * @param state The state that every scheduling node will have.
*/
void set_state_of_nodes(scheduling_node_state_t state) {
  for (uint16_t i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
    test_rti.scheduling_nodes[i]->state = state;
  }
}
/******************************************End of Utility Functions******************************************************/

void valid_cache() {
  set_common_RTI(2);

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_node(1, 1, 0, (int[]) {0}, (interval_t[]) {NEVER}, NULL);

  set_state_of_nodes(GRANTED);

  // If min_delays is not null (the cached data is valid), nothing should be changed.
  test_rti.scheduling_nodes[1]->num_all_upstreams = 1;
  test_rti.scheduling_nodes[1]->all_upstreams = (uint16_t*)calloc(1, sizeof(uint16_t));
  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_all_upstreams == 1);
}

void not_connected() {
  set_common_RTI(2);

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_node(1, 1, 0, (int[]) {0}, (interval_t[]) {NEVER}, NULL);

  set_state_of_nodes(NOT_CONNECTED);

  // If the nodes are not connected, num_all_upstreams should not be changed.
  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_all_upstreams == 0);
}

static void two_nodes_no_delay() {
  set_common_RTI(2);
  uint16_t n = test_rti.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_node(1, 1, 0, (int[]) {0}, (interval_t[]) {NEVER}, NULL);

  set_state_of_nodes(GRANTED);

  // Test update_min_delays_upstream
  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[0]->num_all_upstreams == 0); // node[0] has no upstream nodes.

  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_all_upstreams == 1); // node[1] has one upstream nodes.
  assert(test_rti.scheduling_nodes[1]->all_upstreams[0] == 0); // node[1]'s upstream node is node[0].
  // The min_delay between them is node[0] and node[1] which means no delay.
  // assert(lf_tag_compare(test_rti.scheduling_nodes[1]->min_delays[0].min_delay, ZERO_TAG) == 0);
  assert(lf_tag_compare(test_rti.min_delays[0*n + 1], ZERO_TAG) == 0);
}

static void two_nodes_zero_delay() {
  set_common_RTI(2);
  uint16_t n = test_rti.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --/0/--> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_node(1, 1, 0, (int[]) {0}, (interval_t[]) {0}, NULL);

  set_state_of_nodes(GRANTED);

  // Test update_min_delays_upstream
  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[0]->num_all_upstreams == 0); // node[0] has no upstream nodes.

  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_all_upstreams == 1); // node[1] has one upstream nodes.
  assert(test_rti.scheduling_nodes[1]->all_upstreams[0] == 0); // node[1]'s upstream node is node[0].
  // The min_delay between node[0] and node[1] is (0, 1) which means zero delay.
  assert(lf_tag_compare(test_rti.min_delays[0*n + 1], (tag_t) {.time = 0, .microstep = 1}) == 0);

  // Test update_all_downstreams
  update_all_downstreams(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[0]->num_all_downstreams == 1); // node[0] has one downstream nodes.

  update_all_downstreams(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_all_downstreams == 0); // node[1] has no downstream nodes.
}

static void two_nodes_normal_delay() {
  set_common_RTI(2);
  uint16_t n = test_rti.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --/1 nsec/--> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_node(1, 1, 0, (int[]) {0}, (interval_t[]) {NSEC(1)}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[0]->num_all_upstreams == 0); // node[0] has no upstream nodes.

  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_all_upstreams == 1); // node[1] has one upstream nodes.
  assert(test_rti.scheduling_nodes[1]->all_upstreams[0] == 0); // node[1]'s upstream node is node[0].
  // The min_delay between node[0] and node[1] is (1 nsec, 0).
  assert(lf_tag_compare(test_rti.min_delays[0*n + 1], (tag_t) {.time = NSEC(1), .microstep = 0}) == 0);
}

static void multiple_nodes() {
  set_common_RTI(4);
  uint16_t n = test_rti.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --/1 nsec/--> node[1] --/0/--> node[2] --/2 nsec/--> node[3]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_node(1, 1, 1, (int[]) {0}, (interval_t[]) {NSEC(1)}, (int[]) {2});
  set_scheduling_node(2, 1, 1, (int[]) {1}, (interval_t[]) {0}, (int[]) {3});
  set_scheduling_node(3, 1, 0, (int[]) {2}, (interval_t[]) {NSEC(2)}, NULL);

  set_state_of_nodes(GRANTED);

  // Test update_min_delays_upstream
  update_min_delays_upstream(test_rti.scheduling_nodes[2]);
  assert(test_rti.scheduling_nodes[2]->num_all_upstreams == 2); // node[2] has two upstream nodes.
  assert(test_rti.scheduling_nodes[2]->all_upstreams[0] == 0); // node[0] is an upstream node of node[2].
  // The min_delay between node[0] and node[2] is (1 nsec, 1) = 1 nsec + zero delay.
  assert(lf_tag_compare(test_rti.min_delays[0*n + 2], (tag_t) {NSEC(1), 1}) == 0);
  assert(test_rti.scheduling_nodes[2]->all_upstreams[1] == 1); // node[1] is an upstream node of node[2].
  // The min_delay between node[1] and node[2] is (0, 1), which denotes zero delay.
  assert(lf_tag_compare(test_rti.min_delays[1*n + 2], (tag_t) {0, 1}) == 0);

  update_min_delays_upstream(test_rti.scheduling_nodes[3]);
  assert(test_rti.scheduling_nodes[3]->num_all_upstreams == 3); // node[3] has three upstream nodes.
  assert(test_rti.scheduling_nodes[3]->all_upstreams[0] == 0); // node[0] is an upstream node of node[3].
  // The min_delay between node[0] and node[3] is (3 nsec, 0) = 1 nsec + zero_delay + 2 nsec.
  assert(lf_tag_compare(test_rti.min_delays[0*n + 3], (tag_t) {NSEC(3), 0}) == 0);
  assert(test_rti.scheduling_nodes[3]->all_upstreams[1] == 1); // node[1] is an upstream node of node[3].
  // The min_delay between node[1] and node[3] is (2 nsec, 0) = zero_delay + 2 nsec.
  assert(lf_tag_compare(test_rti.min_delays[1*n + 3], (tag_t) {NSEC(2), 0}) == 0);
  assert(test_rti.scheduling_nodes[3]->all_upstreams[2] == 2); // node[2] is an upstream node of node[3].
  // The min_delay between node[2] and node[3] is (2 nsec, 0).
  assert(lf_tag_compare(test_rti.min_delays[2*n + 3], (tag_t) {NSEC(2), 0}) == 0);

  // Test update_all_downstreams
  update_all_downstreams(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[0]->num_all_downstreams == 3); // node[0] has three downstream nodes.
  assert(test_rti.scheduling_nodes[0]->all_downstreams[0] == 1); // node[1] is a downstream node of node[3].
  assert(test_rti.scheduling_nodes[0]->all_downstreams[1] == 2); // node[2] is a downstream node of node[3].
  assert(test_rti.scheduling_nodes[0]->all_downstreams[2] == 3); // node[3] is a downstream node of node[3].

  update_all_downstreams(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_all_downstreams == 2); // node[1] has two downstream nodes.
  assert(test_rti.scheduling_nodes[1]->all_downstreams[0] == 2); // node[2] is a downstream node of node[3].
  assert(test_rti.scheduling_nodes[1]->all_downstreams[1] == 3); // node[3] is a downstream node of node[3].
}

int main(int argc, char **argv) {
  initialize_rti_common(&test_rti);

  // Tests for the function update_min_delays_upstream() and update_all_downstreams()
  valid_cache();
  not_connected();
  two_nodes_no_delay();
  two_nodes_zero_delay();
  two_nodes_normal_delay();
  multiple_nodes();
}
#endif