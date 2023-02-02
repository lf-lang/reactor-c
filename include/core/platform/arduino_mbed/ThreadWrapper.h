/* ThreadWrapper.h - must compile in both C and C++ */
 
#ifndef THREADWRAPPER_H
#define THREADWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

    void *thread_new();
    void thread_delete(void* thread);
    int thread_start(void* thread, void *(*function) (void *), void* arguments);
    int thread_join(void* thread, int* thread_return);
    int thread_terminate(void* thread);

#ifdef __cplusplus
}
#endif
#endif