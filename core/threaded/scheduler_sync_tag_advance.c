/**
 * @file
 * @author Soroush Bateni
 * @author Edward A. Lee
 * @author Marten Lohstroh
 * @brief API used to advance tag globally.
 * @copyright (c) 2020-2024, The University of California at Berkeley and The University of Texas at Dallas
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 */

#if !defined(LF_SINGLE_THREADED)

#include "scheduler_sync_tag_advance.h"
#include "rti_local.h"
#include "environment.h"
#include "tracepoint.h"
#include "util.h"

// Forward declaration of function defined in reactor_threaded.h
void _lf_next_locked(struct environment_t* env);

/**
 * @brief Indicator that execution of the specified tag has completed.
 */
static tag_t _latest_tag_completed = NEVER_TAG_INITIALIZER;

bool should_stop_locked(lf_scheduler_t* sched) {
  // If this is not the very first step, check against the stop tag to see whether this is the last step.
  // Also, stop only after completing the stop tag.
  if (lf_tag_compare(_latest_tag_completed, sched->env->current_tag) == 0) {
    // If we are at the stop tag, do not call _lf_next_locked()
    // to prevent advancing the logical time.
    if (lf_tag_compare(sched->env->current_tag, sched->env->stop_tag) >= 0) {
      LF_PRINT_DEBUG("****************** Stopping execution at tag " PRINTF_TAG,
                     sched->env->current_tag.time - lf_time_start(), sched->env->current_tag.microstep);
      return true;
    }
  }
  return false;
}

bool _lf_sched_advance_tag_locked(lf_scheduler_t* sched) {
  environment_t* env = sched->env;
  logical_tag_complete(env->current_tag);

// If we are using scheduling enclaves. Notify the local RTI of the time
// advancement.
#if defined LF_ENCLAVES
  rti_logical_tag_complete_locked(env->enclave_info, env->current_tag);
#endif

  if (should_stop_locked(sched)) {
    return true;
  }

  _latest_tag_completed = env->current_tag;

  // Advance time.
  // _lf_next_locked() may block waiting for real time to pass or events to appear on the event queue.
  tracepoint_scheduler_advancing_time_starts(env);
  _lf_next_locked(env);
  tracepoint_scheduler_advancing_time_ends(env);

  LF_PRINT_DEBUG("Scheduler: Done waiting for _lf_next_locked().");
  return false;
}
#endif
