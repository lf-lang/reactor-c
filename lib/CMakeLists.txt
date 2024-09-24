set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
include(${LF_ROOT}/core/lf_utils.cmake)

add_library(lib schedule.c)
target_link_libraries(lib PRIVATE lf::low-level-platform-api)
target_link_libraries(lib PRIVATE lf::logging-api)
target_link_libraries(lib PUBLIC lf::trace-api-types)

lf_enable_compiler_warnings(lib)