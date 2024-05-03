add_library(lf-platform-api INTERFACE)
add_library(lf::platform-api ALIAS lf-platform-api)
target_link_libraries(lf-platform-api INTERFACE lf-low-level-platform-api)
target_include_directories(lf-platform-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
