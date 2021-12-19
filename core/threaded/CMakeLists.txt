# TODO: Conditionally substitute scheduler_PEDF_NP in place of scheduler_GEDF_NP.
set(THREADED_SOURCES reactor_threaded.c scheduler_GEDF_NP.c)
add_sources_to_parent("" "${THREADED_SOURCES}" "")
