#ifndef REACTOR_COMMON_H
#define REACTOR_COMMON_H

#include "lf_types.h"
#include "tag.h"
#include "pqueue.h"
#include "vector.h"
#include "util.h"
#include "modes.h"
#include "port.h"


//  ******** Global Variables :( ********  //
extern unsigned int _lf_number_of_workers;
extern bool fast;
extern unsigned int _lf_number_of_workers;
extern instant_t duration;
extern bool _lf_execution_started;
extern tag_t stop_tag;
extern bool keepalive_specified;
extern bool** _lf_is_present_fields;
extern interval_t _lf_fed_STA_offset;
extern int _lf_is_present_fields_size;
extern bool** _lf_is_present_fields_abbreviated;
extern int _lf_is_present_fields_abbreviated_size;
extern tag_t** _lf_intended_tag_fields;
extern int _lf_intended_tag_fields_size;
extern token_present_t* _lf_tokens_with_ref_count;
extern lf_token_t* _lf_more_tokens_with_ref_count;
extern int _lf_tokens_with_ref_count_size;
extern vector_t _lf_sparse_io_record_sizes;

extern pqueue_t* event_q;

extern int default_argc;
extern const char** default_argv;

#ifdef FEDERATED
void reset_status_fields_on_input_port_triggers();
void enqueue_network_control_reactions();
port_status_t determine_port_status_if_possible(int portID);
typedef enum parse_rti_code_t {
    SUCCESS,
    INVALID_PORT,
    INVALID_HOST,
    INVALID_USER,
    FAILED_TO_PARSE
} parse_rti_code_t;
parse_rti_code_t parse_rti_addr(const char* rti_addr);
void set_federation_id(const char* fid);
#endif

void* _lf_allocate(size_t count, size_t size, struct allocation_record_t** head);
extern struct allocation_record_t* _lf_reactors_to_free;
void* _lf_new_reactor(size_t size);
void _lf_free(struct allocation_record_t** head);
void _lf_free_reactor(struct self_base_t *self);
void _lf_free_all_reactors(void);
void _lf_set_stop_tag(tag_t tag);
extern interval_t lf_get_stp_offset();
void lf_set_stp_offset(interval_t offset);

extern pqueue_t* event_q;

void _lf_trigger_reaction(reaction_t* reaction, int worker_number);
void _lf_start_time_step();
lf_token_t* _lf_create_token(size_t element_size);
lf_token_t* create_token(size_t element_size);
lf_token_t* _lf_initialize_token_with_value(lf_token_t* token, void* value, size_t length);
lf_token_t* _lf_initialize_token(lf_token_t* token, size_t length);
bool _lf_is_tag_after_stop_tag(tag_t tag);
void _lf_pop_events();
void _lf_initialize_timer(trigger_t* timer);
void _lf_recycle_event(event_t* e);
event_t* _lf_create_dummy_events(
    trigger_t* trigger,
    instant_t time,
    event_t* next,
    microstep_t offset
);
int _lf_schedule_at_tag(trigger_t* trigger, tag_t tag, lf_token_t* token);
trigger_handle_t _lf_schedule(trigger_t* trigger, interval_t extra_delay, lf_token_t* token);
trigger_handle_t _lf_insert_reactions_for_trigger(trigger_t* trigger, lf_token_t* token);
trigger_t* _lf_action_to_trigger(void* action);
void _lf_advance_logical_time(instant_t next_time);
trigger_handle_t _lf_schedule_int(void* action, interval_t extra_delay, int value);
lf_token_t* _lf_set_new_array_impl(lf_token_t* token, size_t length, int num_destinations);
bool _lf_check_deadline(self_base_t* self, bool invoke_deadline_handler);
void _lf_invoke_reaction(reaction_t* reaction, int worker);
void schedule_output_reactions(reaction_t* reaction, int worker);
lf_token_t* writable_copy(lf_token_t* token);
int process_args(int argc, const char* argv[]);
void initialize(void);
void termination(void);

#endif
