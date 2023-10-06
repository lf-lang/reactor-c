set(
    THREADED_SOURCES
    reactor_threaded.c
    scheduler_adaptive.c
    scheduler_GEDF_NP.c
    scheduler_NP.c
    scheduler_sync_tag_advance.c
    scheduler_instance.c
    watchdog.c
)
list(APPEND INFO_SOURCES ${THREADED_SOURCES})

list(TRANSFORM THREADED_SOURCES PREPEND threaded/)
target_sources(core PRIVATE ${THREADED_SOURCES})

