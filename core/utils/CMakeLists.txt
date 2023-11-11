set(UTIL_SOURCES vector.c pqueue_base.c pqueue_tag.c pqueue.c util.c semaphore.c)

list(APPEND INFO_SOURCES ${UTIL_SOURCES})

list(TRANSFORM UTIL_SOURCES PREPEND utils/)
target_sources(core PRIVATE ${UTIL_SOURCES})

# Include sources from subdirectories
include(utils/hashset/CMakeLists.txt)
