add_library(lf-platform-api INTERFACE)
add_library(lf::platform-api ALIAS lf-platform-api)
target_include_directories(lf-platform-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
