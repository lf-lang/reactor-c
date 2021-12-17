# TODO: Conditionally substitute scheduler_PEDF_NP in place of scheduler_GEDF_NP.

add_library(
    threaded
    reactor_threaded.c
    scheduler_GEDF_NP.c
    trace.c
)
