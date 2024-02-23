add_library(lf-low-level-platform-api INTERFACE)
target_include_directories(lf-low-level-platform-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::low-level-platform-api ALIAS lf-low-level-platform-api)
target_link_libraries(lf-low-level-platform-api INTERFACE lf::tag-api)
