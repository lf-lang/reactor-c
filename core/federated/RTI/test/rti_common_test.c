#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rti_remote.h"
#include "rti_common.h"
#include "tag.h"

// The RTI under test.
static rti_common_t test_RTI;

/******************************************Start of Utility
 * Functions******************************************************/

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
  free(node);
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
void set_scheduling_node(int id, int num_immediate_upstreams, int num_immediate_downstreams, int* immediate_upstreams,
                         interval_t* immediate_upstream_delays, int* immediate_downstreams) {
  // Save the number of immediate upstream and immediate downstream nodes.
  test_RTI.scheduling_nodes[id]->num_immediate_upstreams = num_immediate_upstreams;
  test_RTI.scheduling_nodes[id]->num_immediate_downstreams = num_immediate_downstreams;

  // If there is any immediate upstream nodes, store IDs and delays from the upstream nodes into the structure.
  if (test_RTI.scheduling_nodes[id]->num_immediate_upstreams > 0) {
    test_RTI.scheduling_nodes[id]->immediate_upstreams =
        (uint16_t*)calloc(test_RTI.scheduling_nodes[id]->num_immediate_upstreams, sizeof(uint16_t));
    test_RTI.scheduling_nodes[id]->immediate_upstream_delays =
        (interval_t*)calloc(test_RTI.scheduling_nodes[id]->num_immediate_upstreams, sizeof(interval_t));
    for (int i = 0; i < test_RTI.scheduling_nodes[id]->num_immediate_upstreams; i++) {
      test_RTI.scheduling_nodes[id]->immediate_upstreams[i] = immediate_upstreams[i];
      test_RTI.scheduling_nodes[id]->immediate_upstream_delays[i] = immediate_upstream_delays[i];
    }
  }
  // If there is any immediate downstream nodes, store IDs of the downstream nodes into the structure.
  if (test_RTI.scheduling_nodes[id]->num_immediate_downstreams > 0) {
    test_RTI.scheduling_nodes[id]->immediate_downstreams =
        (uint16_t*)calloc(test_RTI.scheduling_nodes[id]->num_immediate_downstreams, sizeof(uint16_t));
    for (int i = 0; i < test_RTI.scheduling_nodes[id]->num_immediate_downstreams; i++) {
      test_RTI.scheduling_nodes[id]->immediate_downstreams[i] = immediate_downstreams[i];
    }
  }
}

/**
 * Reset the RTI to re-construct the structure of nodes.
 * This includes freeing every scheduling node and the array of nodes.
 */
void reset_common_RTI() {
  invalidate_min_delays();
  // For every scheduling nodes, delete them and free themselves, too.
  for (uint16_t i = 0; i < test_RTI.number_of_scheduling_nodes; i++) {
    delete_scheduling_node(test_RTI.scheduling_nodes[i]);
  }
  // Free the array of scheduling nodes either. This will be re-created
  // in set_common_RTI().
  if (test_RTI.scheduling_nodes != NULL) {
    free(test_RTI.scheduling_nodes);
  }
}

/**
 * Set the number of nodes and create an array for scheduling nodes.
 * This includes resetting the previous RTI.
 * @param num_nodes The number of scheduling nodes.
 */
void set_common_RTI(uint16_t num_nodes) {
  test_RTI.number_of_scheduling_nodes = num_nodes;

  // Allocate memory for the scheduling nodes
  test_RTI.scheduling_nodes =
      (scheduling_node_t**)calloc(test_RTI.number_of_scheduling_nodes, sizeof(scheduling_node_t*));

  test_RTI.min_delays = NULL;

  for (uint16_t i = 0; i < test_RTI.number_of_scheduling_nodes; i++) {
    scheduling_node_t* scheduling_node = (scheduling_node_t*)malloc(sizeof(scheduling_node_t));
    initialize_scheduling_node(scheduling_node, i);
    test_RTI.scheduling_nodes[i] = scheduling_node;
  }
}

/**
 * Set the state of every scheduling node. The state can be NOT_CONNECTED, GRANTED,
 * or PENDING.
 * @param state The state that every scheduling node will have.
 */
void set_state_of_nodes(scheduling_node_state_t state) {
  for (uint16_t i = 0; i < test_RTI.number_of_scheduling_nodes; i++) {
    test_RTI.scheduling_nodes[i]->state = state;
  }
}
/******************************************End of Utility
 * Functions******************************************************/

void valid_cache() {
  set_common_RTI(2);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NEVER}, NULL);

  set_state_of_nodes(GRANTED);

  test_RTI.min_delays = (tag_t*)calloc((n * n), sizeof(tag_t));
  test_RTI.min_delays[0] = (tag_t){.time = NSEC(1), .microstep = 0};

  // If min_delays is not null (the cached data is valid), nothing should be changed.
  update_min_delays();
  assert(lf_tag_compare(test_RTI.min_delays[0], (tag_t){.time = NSEC(1), .microstep = 0}) == 0);

  reset_common_RTI();
}

void not_connected() {
  set_common_RTI(2);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NEVER}, NULL);

  set_state_of_nodes(NOT_CONNECTED);

  // If the nodes are not connected, the matrix should be filled with FOREVER_TAG.
  update_min_delays();
  for (uint16_t i = 0; i < n; i++) {
    for (uint16_t j = 0; j < n; j++) {
      assert(lf_tag_compare(test_RTI.min_delays[i * n + j], FOREVER_TAG) == 0);
    }
  }

  reset_common_RTI();
}

static void two_nodes_no_delay() {
  set_common_RTI(2);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NEVER}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays();
  // The min_delay from 0 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 0 to 1 should be ZERO_TAG which means no delay.
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 1], ZERO_TAG) == 0);
  // The min_delay from 1 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 1 to 1 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 1], FOREVER_TAG) == 0);

  reset_common_RTI();
}

static void two_nodes_zero_delay() {
  set_common_RTI(2);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --/0/--> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){0}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays();
  // The min_delay from 0 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 0 to 1 should be (0, 1).
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 1], (tag_t){.time = 0, .microstep = 1}) == 0);
  // The min_delay from 1 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 1 to 1 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 1], FOREVER_TAG) == 0);

  reset_common_RTI();
}

static void two_nodes_normal_delay() {
  set_common_RTI(2);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --/1 nsec/--> node[1]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 0, (int[]){0}, (interval_t[]){NSEC(1)}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays();
  // The min_delay from 0 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 0 to 1 should be (1, 0).
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 1], (tag_t){.time = 1, .microstep = 0}) == 0);
  // The min_delay from 1 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 1 to 1 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 1], FOREVER_TAG) == 0);

  reset_common_RTI();
}

static void two_nodes_cycle() {
  set_common_RTI(2);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --/1 nsec/--> node[1] --> node[0]
  set_scheduling_node(0, 1, 1, (int[]){1}, (interval_t[]){NEVER}, (int[]){1});
  set_scheduling_node(1, 1, 1, (int[]){0}, (interval_t[]){NSEC(1)}, (int[]){0});

  set_state_of_nodes(GRANTED);

  update_min_delays();
  // The min_delay from 0 to 0 should be (1, 0).
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 0], (tag_t){.time = 1, .microstep = 0}) == 0);
  // The min_delay from 0 to 1 should be (1, 0).
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 1], (tag_t){.time = 1, .microstep = 0}) == 0);
  // The min_delay from 1 to 0 should be ZERO_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 0], ZERO_TAG) == 0);
  // The min_delay from 1 to 1 should be (1, 0).
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 1], (tag_t){.time = 1, .microstep = 0}) == 0);

  // Both of them are in a cycle.
  assert(is_in_cycle(test_RTI.scheduling_nodes[0]) == 1);
  assert(is_in_cycle(test_RTI.scheduling_nodes[1]) == 1);
  // Both of them are in a zero delay cycle.
  assert(is_in_zero_delay_cycle(test_RTI.scheduling_nodes[0]) == 0);
  assert(is_in_zero_delay_cycle(test_RTI.scheduling_nodes[1]) == 0);

  reset_common_RTI();
}

static void two_nodes_ZDC() {
  set_common_RTI(2);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --> node[1] --> node[0]
  set_scheduling_node(0, 1, 1, (int[]){1}, (interval_t[]){NEVER}, (int[]){1});
  set_scheduling_node(1, 1, 1, (int[]){0}, (interval_t[]){NEVER}, (int[]){0});

  set_state_of_nodes(GRANTED);

  update_min_delays();
  // The min_delay from 0 to 0 should be ZERO_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 0], ZERO_TAG) == 0);
  // The min_delay from 0 to 1 should be ZERO_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 1], ZERO_TAG) == 0);
  // The min_delay from 1 to 0 should be ZERO_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 0], ZERO_TAG) == 0);
  // The min_delay from 1 to 1 should be ZERO_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 1], ZERO_TAG) == 0);

  // Both of them are in a zero delay cycle.
  assert(is_in_zero_delay_cycle(test_RTI.scheduling_nodes[0]) == 1);
  assert(is_in_zero_delay_cycle(test_RTI.scheduling_nodes[1]) == 1);

  reset_common_RTI();
}

static void multiple_nodes() {
  set_common_RTI(4);
  uint16_t n = test_RTI.number_of_scheduling_nodes;

  // Construct the structure illustrated below.
  // node[0] --/1 nsec/--> node[1] --/0/--> node[2] --/2 nsec/--> node[3]
  set_scheduling_node(0, 0, 1, NULL, NULL, (int[]){1});
  set_scheduling_node(1, 1, 1, (int[]){0}, (interval_t[]){NSEC(1)}, (int[]){2});
  set_scheduling_node(2, 1, 1, (int[]){1}, (interval_t[]){0}, (int[]){3});
  set_scheduling_node(3, 1, 0, (int[]){2}, (interval_t[]){NSEC(2)}, NULL);

  set_state_of_nodes(GRANTED);

  update_min_delays();
  // The min_delay from 0 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 0 to 1 should be (1, 0).
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 1], (tag_t){.time = 1, .microstep = 0}) == 0);
  // The min_delay from 0 to 2 should be (1, 1).
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 2], (tag_t){.time = 1, .microstep = 1}) == 0);
  // The min_delay from 0 to 3 should be (3, 0).
  assert(lf_tag_compare(test_RTI.min_delays[0 * n + 3], (tag_t){.time = 3, .microstep = 0}) == 0);

  // The min_delay from 1 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 1 to 1 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 1], FOREVER_TAG) == 0);
  // The min_delay from 1 to 2 should be (0, 1).
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 2], (tag_t){.time = 0, .microstep = 1}) == 0);
  // The min_delay from 1 to 3 should be (2, 0).
  assert(lf_tag_compare(test_RTI.min_delays[1 * n + 3], (tag_t){.time = 2, .microstep = 0}) == 0);

  // The min_delay from 2 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[2 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 2 to 1 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[2 * n + 1], FOREVER_TAG) == 0);
  // The min_delay from 2 to 2 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[2 * n + 2], FOREVER_TAG) == 0);
  // The min_delay from 2 to 3 should be (2, 0).
  assert(lf_tag_compare(test_RTI.min_delays[2 * n + 3], (tag_t){.time = 2, .microstep = 0}) == 0);

  // The min_delay from 3 to 0 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[3 * n + 0], FOREVER_TAG) == 0);
  // The min_delay from 3 to 1 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[3 * n + 1], FOREVER_TAG) == 0);
  // The min_delay from 3 to 2 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[3 * n + 2], FOREVER_TAG) == 0);
  // The min_delay from 3 to 3 should be FOREVER_TAG.
  assert(lf_tag_compare(test_RTI.min_delays[3 * n + 3], FOREVER_TAG) == 0);

  reset_common_RTI();
}

int main() {
  initialize_rti_common(&test_RTI);

  // Tests for the function update_min_delays
  valid_cache();
  not_connected();
  two_nodes_no_delay();
  two_nodes_zero_delay();
  two_nodes_normal_delay();
  two_nodes_cycle();
  two_nodes_ZDC();
  multiple_nodes();
}
