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

/* Adds condition variable support in RTOS-enabled Arduino Boards (MBED)
 *  
 *  @author{Anirudh Rengarajan <arengarajan@berkeley.edu>}
 */

#if !defined(LF_SINGLE_THREADED)
#include "mbed.h"
#include "MutexWrapper.h"

using namespace rtos;

#ifdef __cplusplus
extern "C" {
#endif

void* condition_new(void* mutex){
    return new ConditionVariable(*((Mutex*)mutex));
}
    
void condition_delete(void* condition){
    ConditionVariable* cv = (ConditionVariable*) condition;
    delete cv;
}

bool condition_wait_for(void* condition, int64_t absolute_time_ns){
    ConditionVariable* cv = (ConditionVariable*) condition;
    return cv->wait_for(absolute_time_ns / 1000000LL);
}

int condition_wait(void* condition){
    ConditionVariable* cv = (ConditionVariable*) condition;
    cv->wait();
    return 0;
}

void condition_notify_one(void* condition) {
    ConditionVariable* cv = (ConditionVariable*) condition;
    cv->notify_one();
}

void condition_notify_all(void* condition) {
    ConditionVariable* cv = (ConditionVariable*) condition;
    cv->notify_all();
}

#ifdef __cplusplus
}
#endif
#endif