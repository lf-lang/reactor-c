/**
 * @file
 * @brief Threading support in RTOS-enabled Arduino Boards (MBED)
 *  
 * @author Anirudh Rengarajan
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