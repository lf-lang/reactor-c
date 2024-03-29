add_library(lf-network-api INTERFACE)
add_library(lf::network-api ALIAS lf-network-api)
target_include_directories(lf-network-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(lf-network-api INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../../utils)

target_compile_definitions(lf-network-api INTERFACE COMM_TYPE_${COMM_TYPE})
target_link_libraries(lf-network-api INTERFACE lf::tag-api)
# target_link_libraries(lf-network-api INTERFACE lf::low-level-platform-impl)
target_link_libraries(lf-network-api INTERFACE lf::low-level-platform-api)
