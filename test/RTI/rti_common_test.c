#if defined STANDALONE_RTI
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rti_remote.h"
#include "rti_common.h"
#include "tag.h"

static rti_common_t test_rti;

static void eimt() {

}

int set_scheduling_nodes (
    int id, 
    int num_upstream, 
    int num_downstream, 
    int* upstream, 
    interval_t* upstream_delay, 
    int* downstream) {
  if (test_rti.scheduling_nodes[id]->upstream != NULL) {
    free(test_rti.scheduling_nodes[id]->upstream);
  }
  if (test_rti.scheduling_nodes[id]->upstream_delay != NULL) {
    free(test_rti.scheduling_nodes[id]->upstream_delay);
  }
  if (test_rti.scheduling_nodes[id]->downstream != NULL) {
    free(test_rti.scheduling_nodes[id]->downstream);
  }
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

static void min_delays_upstream() {
  /** a -> b -> c 
  */
  // node[0] -> node[1]
  set_scheduling_nodes(0, 0, 1, NULL, NULL, (int[]) {1});
  set_scheduling_nodes(1, 1, 0, (int[]) {0}, (interval_t[]) {NSEC(1)}, NULL);

  // node[0] has no upstreams and nothing is calculated or changed
  update_min_delays_upstream(test_rti.scheduling_nodes[0]);
  assert(test_rti.scheduling_nodes[0]->num_min_delays == 0);
  // 
  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[0]->num_min_delays == 0);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 0);

  // 
  set_scheduling_nodes(1, 1, 0, (int[]) {0}, (interval_t[]) {MSEC(1)}, (int*) calloc(0, sizeof(int)));
  test_rti.scheduling_nodes[0]->state = GRANTED;
  test_rti.scheduling_nodes[1]->state = GRANTED;
  free(test_rti.scheduling_nodes[0]->min_delays);
  free(test_rti.scheduling_nodes[1]->min_delays);
  test_rti.scheduling_nodes[0]->min_delays = NULL;
  test_rti.scheduling_nodes[1]->min_delays = NULL;
  update_min_delays_upstream(test_rti.scheduling_nodes[1]);
  assert(test_rti.scheduling_nodes[1]->num_min_delays == 1);
  assert(lf_tag_compare(test_rti.scheduling_nodes[1]->min_delays[0].min_delay, (tag_t) {MSEC(1), 0}) == 0);
  assert(test_rti.scheduling_nodes[1]->min_delays[0].id == 0);
}

int main(int argc, char **argv) {
  initialize_rti_common(&test_rti);
  test_rti.number_of_scheduling_nodes = 4;

  // Allocate memory for the scheduling nodes
  test_rti.scheduling_nodes = (scheduling_node_t**)calloc(test_rti.number_of_scheduling_nodes, sizeof(scheduling_node_t*));
  for (uint16_t i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
      scheduling_node_t *scheduling_node = (scheduling_node_t *) malloc(sizeof(scheduling_node_t));
      initialize_scheduling_node(scheduling_node, i);
      test_rti.scheduling_nodes[i] = scheduling_node;
  }

  // create_scheduling_nodes(0, 2, 2, (int[]) {2, 3}, (interval_t[]) {MSEC(1), MSEC(1)}, (int[]) {2, 3});

  min_delays_upstream();

  printf("test_rti.scheduling_nodes[0]->downstream[0] = %d\n", test_rti.scheduling_nodes[0]->downstream[0]);
  printf("test_rti.scheduling_nodes[1]->num_upstream = %d\n", test_rti.scheduling_nodes[1]->num_upstream);
  printf("test_rti.scheduling_nodes[1]->upstream[0] = %d\n", test_rti.scheduling_nodes[1]->upstream[0]);
  printf("test_rti.scheduling_nodes[1]->upstream_delay[0] = %ld\n", test_rti.scheduling_nodes[1]->upstream_delay[0]);
}
#endif