add_library(lib schedule.c)
# target_include_directories(lib PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lib PRIVATE lf::low-level-platform-api)
target_link_libraries(lib PRIVATE lf::logging-api)
