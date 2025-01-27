#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rti_remote.h"
#include "rti_common.h"
#include "tag.h"

// The RTI under test.
static rti_common_t test_rti;

/******************************************Start of Utility
 * Functions******************************************************/

/**
 * Free dynamically allocated memory on the scheduling node.
 * @param node The node to be freed
 */
void delete_scheduling_node(scheduling_node_t* node) {
  if (node->upstream != NULL) {
    free(node->upstream);
  }
  if (node->upstream_delay != NULL) {
    free(node->upstream_delay);
  }
  if (node->downstream != NULL) {
    free(node->downstream);
  }
  invalidate_min_delays_upstream(node);
}

/**
 * Set the parameters of scheduling node to construct the test case.
 * Before calling this function, reset_common_RTI should be called to
 * reset every scheduling nodes.
 * @param id The ID of the scheduling node.
 * @param num_upstream The number of upstreams of the scheduling node.
 * @param num_downstream The number of downstreams of the scheduling node.
 * @param upstream The array of IDs from upstream nodes.
 * @param upstream_delay The array of delays from upstream nodes.
 * @param downstream The array of IDs from downstream nodes.
 */
void set_scheduling_node(int id, int num_upstream, int num_downstream, int* upstream, interval_t* upstream_delay,
                         int* downstream) {
  // Save the number of upstream and downstream nodes.
  test_rti.scheduling_nodes[id]->num_upstream = num_upstream;
  test_rti.scheduling_nodes[id]->num_downstream = num_downstream;

  // If there is any upstream nodes, store IDs and delays from the upstream nodes into the structure.
  if (test_rti.scheduling_nodes[id]->num_upstream > 0) {
    test_rti.scheduling_nodes[id]->upstream =
        (uint16_t*)calloc(test_rti.scheduling_nodes[id]->num_upstream, sizeof(uint16_t));
    test_rti.scheduling_nodes[id]->upstream_delay =
        (interval_t*)calloc(test_rti.scheduling_nodes[id]->num_upstream, sizeof(interval_t));
    for (int i = 0; i < test_rti.scheduling_nodes[id]->num_upstream; i++) {
      test_rti.scheduling_nodes[id]->upstream[i] = upstream[i];
      test_rti.scheduling_nodes[id]->upstream_delay[i] = upstream_delay[i];
    }
  }
  // If there is any downstream nodes, store IDs of the downstream nodes into the structure.
  if (test_rti.scheduling_nodes[id]->num_downstream > 0) {
    test_rti.scheduling_nodes[id]->downstream =
        (uint16_t*)calloc(test_rti.scheduling_nodes[id]->num_downstream, sizeof(uint16_t));
    for (int i = 0; i < test_rti.scheduling_nodes[id]->num_downstream; i++) {
      test_rti.scheduling_nodes[id]->downstream[i] = downstream[i];
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
  test_rti.scheduling_nodes =
      (scheduling_node_t**)calloc(test_rti.number_of_scheduling_nodes, sizeof(scheduling_node_t*));
  for (uint16_t i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
    scheduling_node_t* scheduling_node = (scheduling_node_t*)malloc(sizeof(scheduling_node_t));
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
/******************************************End of Utility
 * Functions******************************************************/

void valid_cache() {
  set_common_RTI(2);

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NEVER}, NULL);

  set_state_of_nodes(GRANTED);

  // If min_delays is not null (the cached data is valid), nothing should be changed.
  test_rti.scheduling_nodes[1]->num_min_delays = 1;
  test_rti.scheduling_nodes[1]->min_delays = (minimum_delay_t*)calloc(1, sizeof(minimum_delay_t));
  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 1);
}

void not_connected() {
  set_common_RTI(2);

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NEVER}, NULL);

  set_state_of_nodes(NOT_CONNECTED);

  // If the nodes are not connected, num_min_delays should not be changed.
  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 0);
}

static void two_nodes_no_delay() {
  set_common_RTI(2);

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NEVER}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 0); // node[0] has no upstream nodes.

  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 1);   // node[1] has one upstream nodes.
  assert(test_rti.scheduling_nodes[1]->min_delays[0].id == 0); // node[1]'s upstream node is node[0].
  // The min_delay between them is node[0] and node[1] which means no delay.
  assert(lf_tag_compare(test_rti.scheduling_nodes[1]->min_delays[0].min_delay, ZERO_TAG) == 0);
}

static void two_nodes_zero_delay() {
  set_common_RTI(2);

  // Construct the structure illustrated below.
  // node[0] --/0/--> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){0}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 0); // node[0] has no upstream nodes.

  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 1);   // node[1] has one upstream nodes.
  assert(test_rti.scheduling_nodes[1]->min_delays[0].id == 0); // node[1]'s upstream node is node[0].
  // The min_delay between node[0] and node[1] is (0, 1) which means zero delay.
  assert(lf_tag_compare(test_rti.scheduling_nodes[1]->min_delays[0].min_delay, (tag_t){.time = 0, .microstep = 1}) ==
         0);
}

static void two_nodes_normal_delay() {
  set_common_RTI(2);

  // Construct the structure illustrated below.
  // node[0] --/1 nsec/--> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NSEC(1)}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 0); // node[0] has no upstream nodes.

  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 1);   // node[1] has one upstream nodes.
  assert(test_rti.scheduling_nodes[1]->min_delays[0].id == 0); // node[1]'s upstream node is node[0].
  // The min_delay between node[0] and node[1] is (1 nsec, 0).
  assert(lf_tag_compare(test_rti.scheduling_nodes[1]->min_delays[0].min_delay,
                        (tag_t){.time = NSEC(1), .microstep = 0}) == 0);
}

static void multiple_nodes() {
  set_common_RTI(4);

  // Construct the structure illustrated below.
  // node[0] --/1 nsec/--> node[1] --/0/--> node[2] --/2 nsec/--> node[3]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 1, (int[]){0}, (interval_t[]){NSEC(1)}, (int[]){2});
  set_scheduling_node(2, 1, 1, (int[]){1}, (interval_t[]){0}, (int[]){3});
  set_scheduling_node(3, 1, 0, (int[]){2}, (interval_t[]){NSEC(2)}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays_upstream(test_rti.scheduling_nodes[2]);
  assert(test_rti.scheduling_nodes[2]->num_min_delays == 2);   // node[2] has two upstream nodes.
  assert(test_rti.scheduling_nodes[2]->min_delays[1].id == 1); // node[2]'s first upstream node is node[1].
  // The min_delay between node[1] and node[2] is (0, 1), which denotes zero delay.
  assert(lf_tag_compare(test_rti.scheduling_nodes[2]->min_delays[1].min_delay, (tag_t){0, 1}) == 0);
  assert(test_rti.scheduling_nodes[2]->min_delays[0].id == 0); // node[2]'s second upstream node is node[0].
  // The min_delay between node[0] and node[2] is (1 nsec, 1) = 1 nsec + zero delay.
  assert(lf_tag_compare(test_rti.scheduling_nodes[2]->min_delays[0].min_delay, (tag_t){NSEC(1), 1}) == 0);

  update_min_delays_upstream(test_rti.scheduling_nodes[3]);
  assert(test_rti.scheduling_nodes[3]->num_min_delays == 3);   // node[3] has three upstream nodes.
  assert(test_rti.scheduling_nodes[3]->min_delays[2].id == 2); // node[3]'s first upstream node is node [2].
  // The min_delay between node[2] and node[3] is (2 nsec, 0).
  assert(lf_tag_compare(test_rti.scheduling_nodes[3]->min_delays[2].min_delay, (tag_t){NSEC(2), 0}) == 0);
  assert(test_rti.scheduling_nodes[3]->min_delays[1].id == 1); // node[3]'s second upstream node is node [1].
  // The min_delay between node[1] and node[3] is (2 nsec, 0) = zero_delay + 2 nsec.
  assert(lf_tag_compare(test_rti.scheduling_nodes[3]->min_delays[1].min_delay, (tag_t){NSEC(2), 0}) == 0);
  assert(test_rti.scheduling_nodes[3]->min_delays[0].id == 0); // node[3]'s third upstream node is node [0].
  // The min_delay between node[0] and node[3] is (3 nsec, 0) = 1 nsec + zero_delay + 2 nsec.
  assert(lf_tag_compare(test_rti.scheduling_nodes[3]->min_delays[0].min_delay, (tag_t){NSEC(3), 0}) == 0);
}

int main() {
  initialize_rti_common(&test_rti);

  // Tests for the function update_min_delays_upstream()
  valid_cache();
  not_connected();
  two_nodes_no_delay();
  two_nodes_zero_delay();
  two_nodes_normal_delay();
  multiple_nodes();
}
