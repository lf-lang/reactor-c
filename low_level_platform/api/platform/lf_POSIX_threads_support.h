/**
 * @file lf_POSIX_threads_support.h
 * @brief POSIX API support for the C target of Lingua Franca.
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 *
 * All functions return 0 on success.
 */

#ifndef LF_POSIX_THREADS_SUPPORT_H
#define LF_POSIX_THREADS_SUPPORT_H

#include <pthread.h>

typedef pthread_mutex_t lf_mutex_t;
typedef struct {
  lf_mutex_t* mutex;
  pthread_cond_t condition;
} lf_cond_t;
typedef pthread_t lf_thread_t;

#endif
