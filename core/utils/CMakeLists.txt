set(UTIL_SOURCES vector.c pqueue_base.c pqueue_tag.c pqueue.c util.c)

if(NOT DEFINED LF_SINGLE_THREADED)
  list(APPEND UTIL_SOURCES lf_semaphore.c)
endif()

list(TRANSFORM UTIL_SOURCES PREPEND utils/)
list(APPEND REACTORC_SOURCES ${UTIL_SOURCES})

# Include sources from subdirectories
include(utils/hashset/CMakeLists.txt)
