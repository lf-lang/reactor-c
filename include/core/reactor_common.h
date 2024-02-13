/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @author Marten Lohstroh
 * @author Soroush Bateni
 * @author Mehrdad Niknami
 * @author Alexander Schulz-Rosengarten
 * @author Erling Rennemo Jellum
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Declarations of functions with implementations in reactor.c and reactor_threaded.c.
 * 
 * The functions declared in this file, as opposed to the onese in reactor.h, are not meant to be
 * called by application programmers. They should be viewed as private functions that make up the
 * C runtime.
 */

#ifndef REACTOR_COMMON_H
#define REACTOR_COMMON_H

#include "lf_types.h"
#include "environment.h"
#include "tag.h"
#include "pqueue.h"
#include "vector.h"
#include "util.h"
#include "modes.h"
#include "port.h"

//////////////////////  Constants & Macros  //////////////////////

/**
 * @brief Constant giving the minimum amount of time to sleep to wait
 * for physical time to reach a logical time.
 * 
 * Unless the "fast" option is given, an LF program will wait until
 * physical time matches logical time before handling an event with
 * a given logical time. The amount of time is less than this given
 * threshold, then no wait will occur. The purpose of this is
 * to prevent unnecessary delays caused by simply setting up and
 * performing the wait.
 */
#define MIN_SLEEP_DURATION USEC(10)

//////////////////////  Global Variables  //////////////////////

// The following variables are efined in reactor_common.c and used in reactor.c,
// reactor_threaded.c, and (some) by the code generator.
extern bool _lf_normal_termination;
extern unsigned int _lf_number_of_workers;
extern int default_argc;
extern const char** default_argv;
extern instant_t duration;
extern bool fast;
extern bool keepalive_specified;

#ifdef FEDERATED
extern interval_t lf_fed_STA_offset;
#endif

//////////////////////  Private Functions  //////////////////////

/**
 * @brief Recycle the given event.
 * 
 * This will zero out the event and push it onto the recycle queue.
 * @param env Environment in which we are executing.
 * @param e The event to recycle.
 */
void lf_recycle_event(environment_t* env, event_t* e);

/**
 * @brief Perform final wrap-up on exit.
 * 
 * This function will be registered to execute on exit.
 * It reports elapsed logical and physical times and reports if any
 * memory allocated for tokens has not been freed.
 */
void termination(void);

extern struct allocation_record_t* _lf_reactors_to_free;
void _lf_trigger_reaction(environment_t* env, reaction_t* reaction, int worker_number);
void _lf_initialize_timer(environment_t* env, trigger_t* timer);
void _lf_initialize_timers(environment_t* env);
void _lf_trigger_startup_reactions(environment_t* env);
void _lf_trigger_shutdown_reactions(environment_t *env);
event_t* _lf_create_dummy_events(
    environment_t* env,
    trigger_t* trigger,
    instant_t time,
    event_t* next,
    microstep_t offset
);
trigger_handle_t _lf_schedule_at_tag(environment_t* env, trigger_t* trigger, tag_t tag, lf_token_t* token);
trigger_handle_t _lf_insert_reactions_for_trigger(environment_t* env, trigger_t* trigger, lf_token_t* token);

/**
 * Advance from the current tag to the next. If the given next_time is equal to
 * the current time, then increase the microstep. Otherwise, update the current
 * time and set the microstep to zero.
 * @param env The environment in which we are executing
 * @param next_time The time step to advance to.
 */
void _lf_advance_logical_time(environment_t *env, instant_t next_time);
trigger_handle_t _lf_schedule_int(lf_action_base_t* action, interval_t extra_delay, int value);
void _lf_invoke_reaction(environment_t* env, reaction_t* reaction, int worker);
void schedule_output_reactions(environment_t *env, reaction_t* reaction, int worker);
int process_args(int argc, const char* argv[]);
void initialize_global();

#endif
