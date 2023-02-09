set(
    THREADED_SOURCES
    reactor_threaded.c
    scheduler.c
    scheduler_sync_tag_advance.c
)
list(APPEND INFO_SOURCES ${MUTLITHREADED_SOURCES})

list(TRANSFORM THREADED_SOURCES PREPEND threaded/)
target_sources(core PRIVATE ${THREADED_SOURCES})

