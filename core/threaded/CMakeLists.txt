set(
    MULTITHREADED_SOURCES
    reactor_threaded.c
    scheduler.c
    scheduler_sync_tag_advance.c
)
add_sources_to_parent("" MULTITHREADED_SOURCES "")
