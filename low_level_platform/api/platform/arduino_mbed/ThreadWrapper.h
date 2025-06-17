/**
 * @file ThreadWrapper.h
 * @brief Threading support for RTOS-enabled Arduino boards using MBED.
 *
 * @author Anirudh Rengarajan
 *
 * This header file provides a C wrapper around the MBED RTOS threading API
 * for use with Arduino boards that support MBED. It provides thread creation,
 * management, and synchronization primitives.
 */

#ifndef THREADWRAPPER_H
#define THREADWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void* thread_new();
void thread_delete(void* thread);
int thread_start(void* thread, void* (*function)(void*), void* arguments);
int thread_join(void* thread, int* thread_return);
int thread_terminate(void* thread);

#ifdef __cplusplus
}
#endif
#endif