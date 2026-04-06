add_library(lf-network-api INTERFACE)
target_include_directories(lf-network-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::network-api ALIAS lf-network-api)

#Link necessary libraries.
target_link_libraries(lf-network-api INTERFACE lf::tag-api)
target_link_libraries(lf-network-api INTERFACE lf::low-level-platform-api)

target_include_directories(lf-network-api INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../../include/core/utils)

target_compile_definitions(lf-network-api INTERFACE COMM_TYPE_${COMM_TYPE})
