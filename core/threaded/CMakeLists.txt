# TODO: Conditionally substitute scheduler_PEDF_NP in place of scheduler_GEDF_NP.
if(NUMBER_OF_WORKERS)
    list(APPEND THREADED_SOURCES threaded/reactor_threaded.c threaded/scheduler_GEDF_NP.c)
endif(NUMBER_OF_WORKERS)

list(APPEND SOURCES ${THREADED_SOURCES})
set(SOURCES ${SOURCES} PARENT_SCOPE)
