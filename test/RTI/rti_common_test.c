#if defined STANDALONE_RTI
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rti_remote.h"
#include "rti_common.h"
#include "tag.h"

static rti_common_t test_rti;

/*************************************Functions for Structure Construction*********************************************/

void reset_scheduling_nodes(scheduling_node_t* node) {
  if (node->upstream != NULL) {
    free(node->upstream);
  }
  if (node->upstream_delay != NULL) {
    free(node->upstream_delay);
  }
  if (node->downstream != NULL) {
    free(node->downstream);
  }
  if (node->min_delays != NULL) {
    free(node->min_delays);
  }
  node->num_min_delays = 0;
}

int set_scheduling_nodes(
    int id, 
    int num_upstream, 
    int num_downstream, 
    int* upstream, 
    interval_t* upstream_delay, 
    int* downstream) {
  reset_scheduling_nodes(test_rti.scheduling_nodes[id]);

  test_rti.scheduling_nodes[id]->num_upstream = num_upstream;
  test_rti.scheduling_nodes[id]->num_downstream = num_downstream;
  if (test_rti.scheduling_nodes[id]->num_upstream > 0) {
    test_rti.scheduling_nodes[id]->upstream = (int*) calloc(test_rti.scheduling_nodes[id]->num_upstream, sizeof(int));
    test_rti.scheduling_nodes[id]->upstream_delay = (interval_t*) calloc(test_rti.scheduling_nodes[id]->num_upstream, sizeof(interval_t));
    for (int i = 0; i < test_rti.scheduling_nodes[id]->num_upstream; i++) {
      test_rti.scheduling_nodes[id]->upstream[i] = upstream[i];
      test_rti.scheduling_nodes[id]->upstream_delay[i] = upstream_delay[i];
    }
  }
  if (test_rti.scheduling_nodes[id]->num_downstream > 0) {
    test_rti.scheduling_nodes[id]->downstream = (int*) calloc(test_rti.scheduling_nodes[id]->num_downstream, sizeof(int));
    for (int i = 0; i < test_rti.scheduling_nodes[id]->num_downstream; i++) {
      test_rti.scheduling_nodes[id]->downstream[i] = downstream[i];
    }
  }
}

void reset_common_RTI() {
  for (uint16_t i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
    reset_scheduling_nodes(test_rti.scheduling_nodes[i]);
  }
  if (test_rti.scheduling_nodes != NULL) {
    free(test_rti.scheduling_nodes);
  }
}

void set_common_RTI(uint16_t num_nodes) {
  reset_common_RTI();

  test_rti.number_of_scheduling_nodes = num_nodes;

  // Allocate memory for the scheduling nodes
  test_rti.scheduling_nodes = (scheduling_node_t**)calloc(test_rti.number_of_scheduling_nodes, sizeof(scheduling_node_t*));
  for (uint16_t i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
      scheduling_node_t *scheduling_node = (scheduling_node_t *) malloc(sizeof(scheduling_node_t));
      initialize_scheduling_node(scheduling_node, i);
      test_rti.scheduling_nodes[i] = scheduling_node;
  }
}
/**********************************End of Functions for Structure Construction*****************************************/

static void no_cycle_one_branch() {
  set_common_RTI(4);

  // node[0] --/1 ns/--> node[1] --/0/--> node[2] --/2 ns/--> node[3]
  set_scheduling_nodes(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_nodes(1, 1, 1, (int[]) {0}, (interval_t[]) {NSEC(1)}, (int[]) {2});
  set_scheduling_nodes(2, 1, 1, (int[]) {1}, (interval_t[]) {0}, (int[]) {3});
  set_scheduling_nodes(3, 1, 0, (int[]) {2}, (interval_t[]) {NSEC(2)}, NULL);

  test_rti.scheduling_nodes[0]->state = GRANTED;
  test_rti.scheduling_nodes[1]->state = GRANTED;
  test_rti.scheduling_nodes[2]->state = GRANTED;
  test_rti.scheduling_nodes[3]->state = GRANTED;

  // If min_delays is not null, nothing should be changed.
  test_rti.scheduling_nodes[3]->num_min_delays = 1;
  test_rti.scheduling_nodes[3]->min_delays = (minimum_delay_t*)calloc(1, sizeof(minimum_delay_t));
  update_min_delays_upstream(test_rti.scheduling_nodes[3]);
  assert(test_rti.scheduling_nodes[3]->num_min_delays == 1);
  invalidate_min_delays_upstream(test_rti.scheduling_nodes[3]);

  test_rti.scheduling_nodes[0]->state = NOT_CONNECTED;
  test_rti.scheduling_nodes[1]->state = NOT_CONNECTED;
  test_rti.scheduling_nodes[2]->state = NOT_CONNECTED;
  test_rti.scheduling_nodes[3]->state = NOT_CONNECTED;

  // Every node is not connected, the size of min_delays should be 0.
  update_min_delays_upstream(test_rti.scheduling_nodes[3]);
  assert(test_rti.scheduling_nodes[3]->num_min_delays == 0);
  invalidate_min_delays_upstream(test_rti.scheduling_nodes[3]);

  test_rti.scheduling_nodes[0]->state = GRANTED;
  test_rti.scheduling_nodes[1]->state = GRANTED;
  test_rti.scheduling_nodes[2]->state = GRANTED;
  test_rti.scheduling_nodes[3]->state = GRANTED;

  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[0]->num_min_delays == 0); // node[0] has no upstream nodes.

  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 1); // node[1] has one upstream nodes.
  assert(test_rti.scheduling_nodes[1]->min_delays[0].id == 0); // node[1]'s upstream node is node[0].
  // The min_delay between them is (1 ns, 0).
  assert(lf_tag_compare(test_rti.scheduling_nodes[1]->min_delays[0].min_delay, (tag_t) {NSEC(1), 0}) == 0);

  update_min_delays_upstream(test_rti.scheduling_nodes[2]);
  assert(test_rti.scheduling_nodes[2]->num_min_delays == 2); // node[2] has two upstream nodes.
  assert(test_rti.scheduling_nodes[2]->min_delays[1].id == 1); // node[2]'s first upstream node is node[1].
  // The min_delay between them is (0, 1), which denotes zero delay.
  assert(lf_tag_compare(test_rti.scheduling_nodes[2]->min_delays[1].min_delay, (tag_t) {0, 1}) == 0);
  assert(test_rti.scheduling_nodes[2]->min_delays[0].id == 0); // node[2]'s second upstream node is node[0].
  // The min_delay between them is (1 ns, 1) = (0, 1) + 1 ns.
  assert(lf_tag_compare(test_rti.scheduling_nodes[2]->min_delays[0].min_delay, (tag_t) {NSEC(1), 1}) == 0);

  update_min_delays_upstream(test_rti.scheduling_nodes[3]);
  assert(test_rti.scheduling_nodes[3]->num_min_delays == 3); // node[3] has three upstream nodes.
  assert(test_rti.scheduling_nodes[3]->min_delays[0].id == 0); // node[3]'s first upstream node is node [0].
  // The min_delay between them is (3 ns, 0) = 1 ns + zero_delay + 2 ns.
  assert(lf_tag_compare(test_rti.scheduling_nodes[3]->min_delays[0].min_delay, (tag_t) {NSEC(3), 0}) == 0);
  assert(test_rti.scheduling_nodes[3]->min_delays[1].id == 1); // node[3]'s second upstream node is node [1].
  // The min_delay between them is (2 ns, 0) = zero_delay + 2 ns.
  assert(lf_tag_compare(test_rti.scheduling_nodes[3]->min_delays[1].min_delay, (tag_t) {NSEC(2), 0}) == 0);
  assert(test_rti.scheduling_nodes[3]->min_delays[2].id == 2); // node[3]'s third upstream node is node [2].
  // The min_delay between them is (2 ns, 0).
  assert(lf_tag_compare(test_rti.scheduling_nodes[3]->min_delays[2].min_delay, (tag_t) {NSEC(2), 0}) == 0);
}

static void delay_cycle() {
  
}

int main(int argc, char **argv) {
  initialize_rti_common(&test_rti);

  no_cycle_one_branch();
}
#endif