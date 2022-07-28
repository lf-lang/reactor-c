set(
    MULTITHREADED_SOURCES
    reactor_threaded.c
    scheduler_sync_tag_advance.c
    scheduler_GEDF_NP.c
    scheduler_GEDF_NP_CI.c
    scheduler_NP.c
    scheduler_adaptive.c
)
add_sources_to_parent("" MULTITHREADED_SOURCES "")
