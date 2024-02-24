add_library(lib schedule.c)
target_link_libraries(lib PRIVATE lf::low-level-platform-api)
target_link_libraries(lib PRIVATE lf::logging-api)
