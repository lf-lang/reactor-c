/**
 * @file
 * @brief Mutex support in RTOS-enabled Arduino Boards (MBED)
 *  
 * @author Anirudh Rengarajan
 */

#if !defined(LF_SINGLE_THREADED)
#include "mbed.h"
#include "rtos.h"
using namespace rtos;

#ifdef __cplusplus
extern "C" {
#endif

void *mutex_new(){
    return new Mutex();
}

void mutex_delete(void* mutex){
    Mutex *m = (Mutex *)mutex;
    delete m;
}

void mutex_lock(void* mutex){
    Mutex *m = (Mutex *)mutex;
    m->lock();
}

bool mutex_trylock(void* mutex){
    Mutex *m = (Mutex *)mutex;
    return m->trylock();
}

void mutex_unlock(void* mutex){
    Mutex *m = (Mutex *)mutex;
    m->unlock();
}

void *mutex_get_owner(void* mutex){
    Mutex *m = (Mutex *)mutex;
    return m->get_owner();
}

#ifdef __cplusplus
}
#endif
#endif