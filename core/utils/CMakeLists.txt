set(UTIL_SOURCES vector.c pqueue_base.c pqueue_tag.c pqueue.c util.c lf_semaphore.c circular_buffer.c)


list(TRANSFORM UTIL_SOURCES PREPEND utils/)
list(APPEND REACTORC_SOURCES ${UTIL_SOURCES})

# Include sources from subdirectories
include(utils/hashset/CMakeLists.txt)
