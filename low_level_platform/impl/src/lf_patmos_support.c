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

// Keep track of physical actions being entered into the system
static volatile bool _lf_async_event = false;
// Keep track of whether we are in a critical section or not
static volatile int _lf_num_nested_critical_sections = 0;
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

int lf_disable_interrupts_nested() {
  if (_lf_num_nested_critical_sections++ == 0) {
    intr_disable();
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
#else // LF_SINGLE_THREADED 

int lf_disable_interrupts_nested() {
  if (_lf_num_nested_critical_sections++ == 0) {
    intr_disable();
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

int lf_available_cores() { return (int)get_cpucnt(); }

lf_thread_t lf_thread_self() {
  lf_thread_t self = {0};
  self.cpuid = (int)get_cpuid();
  return self;
}

int lf_thread_create(lf_thread_t* thread, void* (*lf_thread)(void*), void* arguments) {
  assert(thread != NULL);
  return pthread_create((pthread_t*)thread, NULL, lf_thread, arguments);
}

int lf_thread_join(lf_thread_t thread, void** thread_return) {
  return pthread_join((pthread_t)thread, thread_return);
}

int lf_thread_id() { return (int)get_cpuid(); }

void initialize_lf_thread_id() {}

int lf_mutex_init(lf_mutex_t* mutex) {
  assert(mutex != NULL);
  *mutex = (lf_mutex_t)malloc(sizeof(pthread_mutex_t));
  if (*mutex == NULL) return -1;
  return pthread_mutex_init((pthread_mutex_t*)*mutex, NULL);
}

int lf_mutex_lock(lf_mutex_t* mutex) {
  assert(mutex != NULL && *mutex != NULL);
  return pthread_mutex_lock((pthread_mutex_t*)*mutex);
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
  assert(mutex != NULL && *mutex != NULL);
  return pthread_mutex_unlock((pthread_mutex_t*)*mutex);
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
  assert(cond != NULL);
  *cond = (lf_cond_t)malloc(sizeof(pthread_cond_t));
  if (*cond == NULL) return -1;
  return pthread_cond_init((pthread_cond_t*)*cond, NULL);
}

int lf_cond_signal(lf_cond_t* cond) {
  assert(cond != NULL && *cond != NULL);
  return pthread_cond_signal((pthread_cond_t*)*cond);
}

int lf_cond_broadcast(lf_cond_t* cond) {
  assert(cond != NULL && *cond != NULL);
  return pthread_cond_broadcast((pthread_cond_t*)*cond);
}

int lf_cond_wait(lf_cond_t* cond) {
  assert(cond != NULL && *cond != NULL);
  // Note: This requires that the caller has locked the associated mutex.
  // For bare-metal Patmos, we assume the calling code maintains this invariant.
  // In practice, cond_wait should be used with a mutex like:
  //   pthread_mutex_lock(&mutex);
  //   while (!condition) pthread_cond_wait(&cond, &mutex);
  //   pthread_mutex_unlock(&mutex);
  // Since we don't track the associated mutex here, the caller must manage it.
  return -1;  // Not directly supported; use architecture-specific approach
}

int _lf_cond_timedwait(lf_cond_t* cond, instant_t wakeup_time) {
  assert(cond != NULL && *cond != NULL);
  instant_t now;
  _lf_clock_gettime(&now);
  
  if (now >= wakeup_time) {
    return LF_TIMEOUT;
  }

  // Patmos does not provide a native blocking condition-variable timeout here.
  return LF_TIMEOUT;  // Treat expired or unsupported waits as timeout on Patmos.
}

#endif

#endif // PLATFORM_PATMOS
