/**
 * @file ConditionWrapper.h
 * @brief Condition variable support for RTOS-enabled Arduino boards using MBED.
 *
 * @author Anirudh Rengarajan
 *
 * This header file provides a C wrapper around the MBED RTOS condition variable API
 * for use with Arduino boards that support MBED. It provides condition variable
 * creation, waiting, and notification functionality.
 */
#ifndef CONDITIONWRAPPER_H
#define CONDITIONWRAPPER_H

struct condition;

#ifdef __cplusplus
extern "C" {
#endif

void* condition_new(void*);
void condition_delete(void*);
int condition_wait_for(void*, uint64_t);
int condition_wait(void*);
void condition_notify_one(void*);
void condition_notify_all(void*);

#ifdef __cplusplus
}
#endif
#endif