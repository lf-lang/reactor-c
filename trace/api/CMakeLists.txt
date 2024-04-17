add_library(lf-trace-api INTERFACE)
add_library(lf::trace-api ALIAS lf-trace-api)
target_include_directories(lf-trace-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
