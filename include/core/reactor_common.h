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


//  ******** Global Variables :( ********  //
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
extern interval_t _lf_fed_STA_offset;
#endif

/**
 * @brief Return true if the provided tag is after stop tag.
 * @param env Environment in which we are executing.
 * @param tag The tag to check against stop tag
 */
bool lf_is_tag_after_stop_tag(environment_t* env, tag_t tag);

/**
 * Recycle the given event.
 * Zero it out and pushed it onto the recycle queue.
 * @param env Environment in which we are executing.
 * @param e The event to recycle.
 */
void lf_recycle_event(environment_t* env, event_t* e);

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
