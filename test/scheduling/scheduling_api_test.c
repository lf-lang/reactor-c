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
    if (lf_thread_get_priority(lf_thread_self()) != 99) {
      lf_print_error_and_exit("lf_thread_get_priority failed got %d expected 99", res);
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
    if (lf_thread_get_priority(lf_thread_self()) != 99) {
      lf_print_error_and_exit("lf_thread_get_priority failed got %d expected 99", res);
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
  res = lf_thread_get_priority(lf_thread_self());
  if (res != 50) {
    lf_print_error_and_exit("Line %d lf_thread_get_priority failed got %d expected 50", __LINE__, res);
  }

  // Try negative priority
  res = lf_thread_set_priority(lf_thread_self(), -50);
  if (res == 0) {
    lf_print_error_and_exit("lf_thread_set_priority should have failed for -50");
  }
  res = lf_thread_get_priority(lf_thread_self());
  if (res != 50) {
    lf_print_error_and_exit("Line %d lf_thread_get_priority failed got %d expected 50", __LINE__, res);
  }

  // Configure back to SCHED_OTHER
  {
    lf_scheduling_policy_t cfg;
    cfg.policy = LF_SCHED_FAIR;
    cfg.priority = 0;
    res = lf_thread_set_scheduling_policy(lf_thread_self(), &cfg);
    if (res != 0) {
      lf_print_error_and_exit("lf_thread_set_scheduling_policy FAIR failed with %d", res);
    }
    res = lf_thread_get_priority(lf_thread_self());
    if (res == 0) {
      lf_print_error_and_exit("Line %d lf_thread_get_priority should fail with SCHED_FAIR %d", __LINE__, res);
    }
  }

  // Try pinning to non-existant CPU core.
  res = lf_thread_set_cpu(lf_thread_self(), lf_available_cores());
  if (res == 0) {
    lf_print_error_and_exit("Line %d lf_thread_set_cpu should fail for too high CPU id", __LINE__);
  }

  // Try setting nice-ness for CFS
  {
    lf_scheduling_policy_t cfg;
    cfg.policy = LF_SCHED_FAIR;
    cfg.time_slice = 0;
    cfg.priority = 5;
    res = lf_thread_set_scheduling_policy(lf_thread_self(), &cfg);
    if (res != 0) {
      lf_print_error_and_exit("Line %d lf_thread_set_scheduling_policy failed with %d", __LINE__, res);
    }
  }
}
