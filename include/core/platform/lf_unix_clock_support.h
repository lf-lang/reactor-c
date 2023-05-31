#include <time.h>
#include <errno.h>

#include "lf_types.h"

extern instant_t convert_timespec_to_ns(struct timespec tp);
struct timespec convert_ns_to_timespec(instant_t t);
void calculate_epoch_offset(void);
void _lf_initialize_clock();
int _lf_clock_now(instant_t* t);
