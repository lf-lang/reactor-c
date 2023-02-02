#if defined(LF_THREADED)
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