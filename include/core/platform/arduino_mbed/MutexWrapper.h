/* MutexWrapper.h - must compile in both C and C++ */
 
#ifndef MUTEXWRAPPER_H
#define MUTEXWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

    void *mutex_new();
    void mutex_delete();
    void mutex_lock();
    bool mutex_trylock();
    void mutex_unlock();
    void *mutex_get_owner();

#ifdef __cplusplus
}
#endif
#endif