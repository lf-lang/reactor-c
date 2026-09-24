add_library(lf-logging-api INTERFACE)
target_include_directories(lf-logging-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::logging-api ALIAS lf-logging-api)
target_link_libraries(lf-logging-api INTERFACE lf::low-level-platform-api)
