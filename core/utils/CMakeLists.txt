set(GENERAL_SOURCES pqueue.c util.c vector.c)
set(THREADED_SOURCES semaphore.c)
add_sources_to_parent("${GENERAL_SOURCES}" "${THREADED_SOURCES}" "")
