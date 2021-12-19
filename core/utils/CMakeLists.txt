set(UTILS_SOURCES utils/pqueue.c utils/util.c utils/vector.c)

if(NUMBER_OF_WORKERS)
    list(APPEND SOURCES utils/semaphore.c)
endif(NUMBER_OF_WORKERS)

list(APPEND SOURCES ${UTILS_SOURCES})
set(SOURCES ${SOURCES} PARENT_SCOPE)
