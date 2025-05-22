/**
 * @brief Zephyr support for reactor-c
 *
 * @author Erling Jellum
 */

#ifndef LF_ZEPHYR_SUPPORT_H
#define LF_ZEPHYR_SUPPORT_H

#include "lf_tag_64_32.h"

#include <stdint.h> // For fixed-width integral types
#include <stdbool.h>
#include <stdlib.h> //malloc, calloc, free, realloc

#include <zephyr/kernel.h>

#define NO_CLI
#define MINIMAL_STDLIB
#if !defined(LF_SINGLE_THREADED)

typedef struct k_mutex lf_mutex_t;
typedef struct {
  lf_mutex_t* mutex;
  struct k_condvar condition;
} lf_cond_t;
typedef struct k_thread* lf_thread_t;

void _lf_initialize_clock_zephyr_common();

#endif // !LF_SINGLE_THREADED

#endif // LF_ZEPHYR_SUPPORT_H
