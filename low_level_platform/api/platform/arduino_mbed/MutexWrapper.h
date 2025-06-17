/**
 * @file MutexWrapper.h
 * @brief Mutex support for RTOS-enabled Arduino boards using MBED.
 *
 * @author Anirudh Rengarajan
 *
 * This header file provides a C wrapper around the MBED RTOS mutex API
 * for use with Arduino boards that support MBED. It provides mutex creation,
 * locking, and unlocking functionality.
 */

#ifndef MUTEXWRAPPER_H
#define MUTEXWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void* mutex_new();
void mutex_delete();
void mutex_lock();
bool mutex_trylock();
void mutex_unlock();
void* mutex_get_owner();

#ifdef __cplusplus
}
#endif
#endif