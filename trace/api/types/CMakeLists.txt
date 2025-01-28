add_library(lf-trace-api-types INTERFACE)
add_library(lf::trace-api-types ALIAS lf-trace-api-types)
target_include_directories(lf-trace-api-types INTERFACE ${CMAKE_CURRENT_LIST_DIR})
