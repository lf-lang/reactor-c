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
extern unsigned int _lf_number_of_workers;
extern bool fast;
extern instant_t duration;
extern bool keepalive_specified;
extern interval_t _lf_fed_STA_offset;

/** Flag used to disable cleanup operations on normal termination. */
extern bool _lf_normal_termination;

extern int default_argc;
extern const char** default_argv;

extern struct allocation_record_t* _lf_reactors_to_free;
void* _lf_new_reactor(size_t size);
void _lf_free(struct allocation_record_t** head);
void _lf_free_reactor(self_base_t *self);
void _lf_free_all_reactors(void);
void _lf_set_stop_tag(environment_t* env, tag_t tag);
extern interval_t lf_get_stp_offset();
void lf_set_stp_offset(interval_t offset);
void _lf_trigger_reaction(environment_t* env, reaction_t* reaction, int worker_number);
void _lf_start_time_step(environment_t *env);
bool _lf_is_tag_after_stop_tag(environment_t* env, tag_t tag);
void _lf_pop_events(environment_t *env);
void _lf_initialize_timer(environment_t* env, trigger_t* timer);
void _lf_initialize_timers(environment_t* env);
void _lf_trigger_startup_reactions(environment_t* env);
void _lf_trigger_shutdown_reactions(environment_t *env);
void _lf_recycle_event(environment_t* env, event_t* e);
event_t* _lf_create_dummy_events(
    environment_t* env,
    trigger_t* trigger,
    instant_t time,
    event_t* next,
    microstep_t offset
);
trigger_handle_t _lf_schedule_at_tag(environment_t* env, trigger_t* trigger, tag_t tag, lf_token_t* token);
trigger_handle_t _lf_schedule(environment_t* env, trigger_t* trigger, interval_t extra_delay, lf_token_t* token);
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
void termination(void);
int lf_notify_of_event(environment_t* env);
int lf_critical_section_enter(environment_t* env);
int lf_critical_section_exit(environment_t* env);

#endif
