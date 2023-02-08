set(
    THREADED_SOURCES
    reactor_threaded.c
    scheduler_sync_tag_advance.c
)

if (SCHEDULER MATCHES ADAPTIVE) 
SET(
    THREADED_SOURCES 
    ${THREADED_SOURCES} 
    scheduler_adaptive.c
)
elseif (SCHEDULER MATCHES GEDF_NP_CI)
SET(
    THREADED_SOURCES 
    ${THREADED_SOURCES} 
    scheduler_GEDF_NP_CI.c
)
elseif (SCHEDULER MATCHES GEDF_NP)
SET(
    THREADED_SOURCES 
    ${THREADED_SOURCES} 
    scheduler_GEDF_NP.c
)
else ()
SET( 
    THREADED_SOURCES 
    ${THREADED_SOURCES} 
    scheduler_NP.c
)
endif ()

list(APPEND INFO_SOURCES ${THREADED_SOURCES})

list(TRANSFORM THREADED_SOURCES PREPEND threaded/)
target_sources(core PRIVATE ${THREADED_SOURCES})

