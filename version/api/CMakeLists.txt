add_library(lf-version-api INTERFACE)
target_include_directories(lf-version-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::version-api ALIAS lf-version-api)
