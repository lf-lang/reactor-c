/**
 * This tests the real-time scheduling API implementation in Linux.
 */
#include <stdio.h>
#include <stdlib.h>
#include "core/utils/util.h"
#include "low_level_platform.h"

#if !defined PLATFORM_Linux
#error scheduling_api_test.c should only be compiled on Linux
#endif

int main() {
  int res;

  // Set the CPU affinity using specific core IDs
  {
    int core_ids[] = {0, 1};
    res = lf_thread_set_cpu(core_ids, 2);
    if (res != 0 && res != -1) {
      lf_print_error_and_exit("lf_thread_set_cpu failed with %d", res);
    }
  }

  // Configure SCHED_FIFO
  {
    lf_scheduling_policy_t cfg;
    cfg.policy = LF_SCHED_PRIORITY;
    cfg.priority = 99;
    cfg.time_slice = 0;
    res = lf_thread_set_scheduling_policy(lf_thread_self(), &cfg);
    if (res != 0) {
      lf_print_error_and_exit("lf_thread_set_scheduling_policy FIFO failed with %d", res);
    }
  }

  // Configure SCHED_RR
  {
    lf_scheduling_policy_t cfg;
    cfg.policy = LF_SCHED_TIMESLICE;
    cfg.priority = 99;
    cfg.time_slice = 0;
    res = lf_thread_set_scheduling_policy(lf_thread_self(), &cfg);
    if (res != 0) {
      lf_print_error_and_exit("lf_thread_set_scheduling_policy RR failed with %d", res);
    }
  }

  // Try illegal priority
  {
    lf_scheduling_policy_t cfg;
    cfg.policy = LF_SCHED_TIMESLICE;
    cfg.time_slice = 0;
    cfg.priority = 10000;
    res = lf_thread_set_scheduling_policy(lf_thread_self(), &cfg);
    if (res == 0) {
      lf_print_error_and_exit("lf_thread_set_scheduling_policy should have failed with illegal priority");
    }
  }

  // Set the priority
  res = lf_thread_set_priority(lf_thread_self(), 50);
  if (res != 0) {
    lf_print_error_and_exit("lf_thread_set_priority failed with %d", res);
  }

  // Try negative priority
  res = lf_thread_set_priority(lf_thread_self(), -50);
  if (res == 0) {
    lf_print_error_and_exit("lf_thread_set_priority should have failed for -50");
  }

  // Configure back to SCHED_OTHER
  {
    lf_scheduling_policy_t cfg;
    cfg.policy = LF_SCHED_FAIR;
    res = lf_thread_set_scheduling_policy(lf_thread_self(), &cfg);
    if (res != 0) {
      lf_print_error_and_exit("lf_thread_set_scheduling_policy FAIR failed with %d", res);
    }
  }

  // Try with an invalid core ID - should return -1
  {
    int bad_ids[] = {9999};
    res = lf_thread_set_cpu(bad_ids, 1);
    if (res != -1) {
      lf_print_error_and_exit("lf_thread_set_cpu should return -1 for invalid core ID");
    }
  }

  // Try with NULL core_ids - should return -1 (no pinning)
  res = lf_thread_set_cpu(NULL, 0);
  if (res != -1) {
    lf_print_error_and_exit("lf_thread_set_cpu should return -1 for NULL/0");
  }

  printf("All scheduling API tests passed!\n");
  return 0;
}
