/**
 * @file
 * @brief Patmos support for the C target of Lingua Franca.
 *
 * @author Ehsan Khodadad
 * @author Luca Pezzarossa
 * @author Martin Schoeberl
 */
#if defined(PLATFORM_PATMOS)
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "platform/lf_patmos_support.h"
#include "low_level_platform.h"
#include <machine/rtc.h>
#include <machine/exceptions.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int lf_disable_interrupts_nested(void);
int lf_enable_interrupts_nested(void);

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not

/**
 * @brief Sleep until an absolute time.
 * Since there is no sleep mode in Patmos, and energy saving is not important for real-time systems,
 * we just used a busy sleep.
 *
 * @param wakeup int64_t time of wakeup
 * @return int 0 if successful sleep, -1 if awoken by async event
 */

int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup) {
  instant_t now;
  _lf_async_event = false;
  lf_enable_interrupts_nested();

  // Do busy sleep
  do {
    _lf_clock_gettime(&now);
  } while ((now < wakeup) && !_lf_async_event);

  lf_disable_interrupts_nested();

  if (_lf_async_event) {
    _lf_async_event = false;
    return -1;
  } else {
    return 0;
  }
}

int lf_sleep(interval_t sleep_duration) {
  instant_t now;
  _lf_clock_gettime(&now);
  instant_t wakeup = now + sleep_duration;

  // Do busy sleep
  do {
    _lf_clock_gettime(&now);
  } while ((now < wakeup));
  return 0;
}

/**
 * Pause execution for a number of nanoseconds.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_nanosleep`).
 */
int lf_nanosleep(interval_t requested_time) { return lf_sleep(requested_time); }

/**
 * Patmos clock does not need initialization.
 */
void _lf_initialize_clock() {}

/**
 * Write the current time in nanoseconds into the location given by the argument.
 * This returns 0 (it never fails, assuming the argument gives a valid memory location).
 */

int _lf_clock_gettime(instant_t* t) {

  assert(t != NULL);

  *t = get_cpu_usecs() * 1000;

  return 0;
}

#if defined(LF_SINGLE_THREADED)

static volatile int _lf_num_nested_critical_sections = 0;

int lf_disable_interrupts_nested() {
  // For the single-threaded path, disable interrupts first then increment
  // the nesting counter to avoid preemption windows on this core.
  intr_disable();
  if (_lf_num_nested_critical_sections++ == 0) {
    // already disabled above
  }
  return 0;
}

int lf_enable_interrupts_nested() {
  if (_lf_num_nested_critical_sections <= 0) {
    return 1;
  }

  if (--_lf_num_nested_critical_sections == 0) {
    intr_enable();
  }
  return 0;
}

int _lf_single_threaded_notify_of_event() {
  _lf_async_event = true;
  return 0;
}
#else // multi threaded Patmos implementation

/* Dynamically allocate per-core nesting counters based on get_cpucnt().
 * This avoids a hard-coded upper bound and prevents accidental aliasing
 * of core IDs to index 0 if the platform reports more cores than the
 * compile-time constant. Allocation happens once during validation. */
static volatile int* _lf_num_nested_critical_sections_by_core = NULL;
static int _lf_patmos_max_cores = 0;

static inline void _lf_validate_patmos_core_configuration(void) {
  static bool _lf_patmos_core_configuration_validated = false;
  if (!_lf_patmos_core_configuration_validated) {
    int cpucnt = (int)get_cpucnt();
    assert(cpucnt > 0);
    /* Allocate the per-core nesting counter array on first use. Use calloc
     * to initialize counters to zero. Fail-fast on allocation error. */
    _lf_num_nested_critical_sections_by_core = (volatile int*)calloc((size_t)cpucnt, sizeof(int));
    assert(_lf_num_nested_critical_sections_by_core != NULL);
    _lf_patmos_max_cores = cpucnt;
    _lf_patmos_core_configuration_validated = true;
  }
}

static inline volatile int* _lf_current_core_nested_counter() {
  int cpucnt;
  int cpuid;

  _lf_validate_patmos_core_configuration();
  cpucnt = (int)get_cpucnt();
  cpuid = (int)get_cpuid();
  assert(cpuid >= 0);
  assert(cpuid < cpucnt);
  return &_lf_num_nested_critical_sections_by_core[cpuid];
}

// Global (cross-core) lock for atomic operations.

static pthread_mutex_t _lf_patmos_global_lock = PTHREAD_MUTEX_INITIALIZER;

void _lf_patmos_global_lock_acquire(void) { pthread_mutex_lock(&_lf_patmos_global_lock); }
void _lf_patmos_global_lock_release(void) { pthread_mutex_unlock(&_lf_patmos_global_lock); }


int lf_disable_interrupts_nested() {
  // Disable interrupts first and increment the per-core nesting counter.
  intr_disable();
  volatile int* nested_counter = _lf_current_core_nested_counter();
  (*nested_counter)++;
  return 0;
}

int lf_enable_interrupts_nested() {
  volatile int* nested_counter = _lf_current_core_nested_counter();
  if (*nested_counter <= 0) {
    return 1;
  }

  if (--(*nested_counter) == 0) {
    intr_enable();
  }
  return 0;
}

int lf_available_cores() { return (int)get_cpucnt(); }

lf_thread_t lf_thread_self() {
  lf_thread_t self = {0};
  self.cpuid = (int)get_cpuid();
  return self;
}

int lf_thread_create(lf_thread_t* thread, void* (*lf_thread)(void*), void* arguments) {
  assert(thread != NULL);
  return pthread_create(&thread->handle, NULL, lf_thread, arguments);
}

int lf_thread_join(lf_thread_t thread, void** thread_return) {
  return pthread_join(thread.handle, thread_return);
}

int lf_thread_id() { return (int)get_cpuid(); }

/* Forward declaration for the runtime core-count check. */
void initialize_lf_thread_id_check(void);

void initialize_lf_thread_id() { initialize_lf_thread_id_check(); }

// Validate platform assumptions at initialization time.
void initialize_lf_thread_id_check() {
  // Ensure per-core counters are allocated and runtime core count remains stable.
  _lf_validate_patmos_core_configuration();
  assert((int)get_cpucnt() == _lf_patmos_max_cores);
}

int lf_mutex_init(lf_mutex_t* mutex) {
  int result;
  pthread_mutexattr_t attr;

  assert(mutex != NULL);

  result = pthread_mutexattr_init(&attr);
  if (result != 0) {
    return result;
  }

  result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  if (result != 0) {
    pthread_mutexattr_destroy(&attr);
    return result;
  }

  result = pthread_mutex_init((pthread_mutex_t*)mutex, &attr);
  pthread_mutexattr_destroy(&attr);
  return result;
}

int lf_mutex_lock(lf_mutex_t* mutex) {
  assert(mutex != NULL);
  return pthread_mutex_lock((pthread_mutex_t*)mutex);
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
  assert(mutex != NULL);
  return pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
  assert(cond != NULL);
  assert(mutex != NULL);
  cond->mutex = mutex;
  return pthread_cond_init((pthread_cond_t*)&cond->condition, NULL);
}

int lf_cond_signal(lf_cond_t* cond) {
  assert(cond != NULL);
  return pthread_cond_signal((pthread_cond_t*)&cond->condition);
}

int lf_cond_broadcast(lf_cond_t* cond) {
  assert(cond != NULL);
  return pthread_cond_broadcast((pthread_cond_t*)&cond->condition);
}

int lf_cond_wait(lf_cond_t* cond) {
  assert(cond != NULL);
  assert(cond->mutex != NULL);
  return pthread_cond_wait((pthread_cond_t*)&cond->condition, (pthread_mutex_t*)cond->mutex);
}

int _lf_cond_timedwait(lf_cond_t* cond, instant_t wakeup_time) {
  struct timespec ts;
  int rc;

  assert(cond != NULL);
  assert(cond->mutex != NULL);

  instant_t now;
  _lf_clock_gettime(&now);
  
  if (now >= wakeup_time) {
    return LF_TIMEOUT;
  }

  ts.tv_sec = wakeup_time / 1000000000LL;
  ts.tv_nsec = wakeup_time % 1000000000LL;

  rc = pthread_cond_timedwait((pthread_cond_t*)&cond->condition, (pthread_mutex_t*)cond->mutex, &ts);
  if (rc == ETIMEDOUT) {
    return LF_TIMEOUT;
  }
  return rc;
}

#endif

#endif // PLATFORM_PATMOS
