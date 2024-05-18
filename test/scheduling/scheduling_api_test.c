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

  // Set the CPU Set of the current thread.
  res = lf_thread_set_cpu(lf_thread_self(), lf_available_cores() - 1);
  if (res != 0) {
    lf_print_error_and_exit("lf_thread_set_cpu failed with %d", res);
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
      lf_print_error_and_exit("lf_thread_set_scheduling_policy RR failed with %d", res);
    }
  }

  // Try pinning to non-existant CPU core.
  res = lf_thread_set_cpu(lf_thread_self(), lf_available_cores());
  if (res == 0) {
    lf_print_error_and_exit("lf_thread_set_cpu should fail for too high CPU id");
  }
}
