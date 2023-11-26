#if defined STANDALONE_RTI
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rti_remote.h"
#include "rti_common.h"
#include "tag.h"

static rti_common_t test_rti;

/**
 * The structure is the same as the test AfterDelays.lf
 * 
*/
int create_scheduling_nodes() {
  for (int i = 0; i < test_rti.number_of_scheduling_nodes; i++) {
    test_rti.scheduling_nodes[i]->num_upstream = 2;
    test_rti.scheduling_nodes[i]->num_downstream = 2;
    test_rti.scheduling_nodes[i]->upstream = (int*) calloc(test_rti.scheduling_nodes[i]->num_upstream, sizeof(int));
    test_rti.scheduling_nodes[i]->upstream_delay = (interval_t*) calloc(test_rti.scheduling_nodes[i]->num_upstream, sizeof(interval_t));
    test_rti.scheduling_nodes[i]->upstream_delay[0] = MSEC(1);
    test_rti.scheduling_nodes[i]->upstream_delay[1] = MSEC(1);
    test_rti.scheduling_nodes[i]->downstream = (int*) calloc(test_rti.scheduling_nodes[i]->num_downstream, sizeof(int));
  }

  // n1
  test_rti.scheduling_nodes[0]->upstream[0] = 2;
  test_rti.scheduling_nodes[0]->upstream[1] = 3;
  test_rti.scheduling_nodes[0]->downstream[0] = 2;
  test_rti.scheduling_nodes[0]->downstream[1] = 3;
  // n2
  test_rti.scheduling_nodes[1]->upstream[0] = 2;
  test_rti.scheduling_nodes[1]->upstream[1] = 3;
  test_rti.scheduling_nodes[1]->downstream[0] = 2;
  test_rti.scheduling_nodes[1]->downstream[1] = 3;

  // sw1
  test_rti.scheduling_nodes[2]->upstream[0] = 0;
  test_rti.scheduling_nodes[2]->upstream[1] = 1;
  test_rti.scheduling_nodes[2]->downstream[0] = 2;
  test_rti.scheduling_nodes[2]->downstream[1] = 3;

  // sw2
  test_rti.scheduling_nodes[3]->upstream[0] = 0;
  test_rti.scheduling_nodes[3]->upstream[1] = 1;
  test_rti.scheduling_nodes[3]->downstream[0] = 2;
  test_rti.scheduling_nodes[3]->downstream[1] = 3;
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

  create_scheduling_nodes();

  printf("test_rti.scheduling_nodes[0]->upstream[0] = %d\n", test_rti.scheduling_nodes[0]->upstream[0]);
  printf("test_rti.scheduling_nodes[0]->upstream[1] = %d\n", test_rti.scheduling_nodes[0]->upstream[1]);
  printf("test_rti.scheduling_nodes[0]->upstream_delay[0] = %ld\n", test_rti.scheduling_nodes[0]->upstream_delay[0]);
  
}
#endif