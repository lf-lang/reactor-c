#ifndef REACTOR_THREADED_H
#define REACTOR_THREADED_H
/**
 * Enqueue network input control reactions that determine if the trigger for a
 * given network input port is going to be present at the current logical time
 * or absent.
 */
void enqueue_network_input_control_reactions();

/**
 * Enqueue network output control reactions that will send a PORT_ABSENT
 * message to downstream federates if a given network output port is not present.
 */
void enqueue_network_output_control_reactions();
void _lf_increment_global_tag_barrier_already_locked(environment_t *env, tag_t future_tag);
void _lf_increment_global_tag_barrier(environment_t *env, tag_t future_tag);
void _lf_decrement_global_tag_barrier_locked();
int _lf_wait_on_global_tag_barrier(tag_t proposed_tag);
void synchronize_with_other_federates();
bool wait_until(instant_t logical_time_ns, lf_cond_t* condition);
tag_t get_next_event_tag();
tag_t send_next_event_tag(tag_t tag, bool wait_for_reply);
void _lf_next_locked(environment_t* env);
#endif // REACTOR_THREADED_H
