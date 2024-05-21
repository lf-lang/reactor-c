/*************
Copyright (c) 2023, Norwegian University of Science and Technology.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/**
 * @brief Zephyr support for reactor-c
 *
 * @author{Erling Jellum <erling.r.jellum@ntnu.no>}
 */

#ifndef LF_ZEPHYR_SUPPORT_H
#define LF_ZEPHYR_SUPPORT_H

#include "lf_tag_64_32.h"

#include <stdint.h> // For fixed-width integral types
#include <stdbool.h>
#include <stdlib.h> //malloc, calloc, free, realloc

#include <zephyr/kernel.h>

#define NO_TTY
#if !defined(LF_SINGLE_THREADED)

typedef struct k_mutex lf_mutex_t;
typedef struct {
  lf_mutex_t* mutex;
  struct k_condvar condition;
} lf_cond_t;
typedef struct k_thread* lf_thread_t;

/* The constants used for I-controller for lag regulation under reactor_threaded.c wait_until function.
The lag gets multiplied by KI_MUL and divided by KI_DIV before incorporated into control value.
Currently no lag control support in this platform. */
#define KI_DIV 1
#define KI_MUL 0

#endif // !LF_SINGLE_THREADED

#endif // LF_ZEPHYR_SUPPORT_H
