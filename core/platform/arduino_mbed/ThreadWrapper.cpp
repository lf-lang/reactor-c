/*************
Copyright (c) 2023, The University of California at Berkeley.

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

/* Adds threading support in RTOS-enabled Arduino Boards (MBED)
 *  
 *  @author{Anirudh Rengarajan <arengarajan@berkeley.edu>}
 */

#if !defined(LF_SINGLE_THREADED)
#include "mbed.h"
#include "rtos.h"

using namespace mbed;
using namespace rtos;

#ifdef __cplusplus
extern "C" {
#endif

void *thread_new(){
    return new Thread();
}

void thread_delete(void* thread){
    Thread *t = (Thread*)thread;
    delete t;
}

long int thread_start(void* thread, void (*function) (void *), void* arguments){
    Thread *t = (Thread*)thread;
    osStatus s = t->start(callback(function, arguments));
    return s;
}

long int thread_join(void* thread, void** thread_return){
    Thread *t = (Thread*)thread;
    osStatus s = t->join();
    return s;
}

int thread_terminate(void* thread){
    Thread *t = (Thread*)thread;
    return t->terminate();
}

#ifdef __cplusplus
}
#endif
#endif