add_library(lf-tag-api INTERFACE)
target_include_directories(lf-tag-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::tag-api ALIAS lf-tag-api)
