/**
 * @brief Condition variable support in RTOS-enabled Arduino Boards (MBED)
 *
 * @author Anirudh Rengarajan
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