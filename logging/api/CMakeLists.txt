add_library(lf-logging-api INTERFACE)
target_include_directories(lf-logging-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::logging-api ALIAS lf-logging-api)
