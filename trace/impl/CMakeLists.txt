add_library(lf-trace-impl STATIC)
add_library(lf::trace-impl ALIAS lf-trace-impl)
target_link_libraries(lf-trace-impl PRIVATE lf::trace-api)
target_link_libraries(lf-trace-impl PRIVATE lf::platform-api)
target_link_libraries(lf-trace-impl PRIVATE lf::logging-api)
target_link_libraries(lf-trace-impl PRIVATE lf::version-api)

target_sources(lf-trace-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/trace_impl.c)

target_include_directories(lf-trace-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

# handle compile-time parameters
if(NOT DEFINED LOG_LEVEL)
    message(FATAL_ERROR "You must set LOG_LEVEL cmake argument")
endif()
target_compile_definitions(lf-trace-impl PRIVATE LOG_LEVEL=${LOG_LEVEL})
# build type parameter (release, debug, etc) is implicitly handled by CMake

# make name platform-independent
set_target_properties(lf-trace-impl PROPERTIES PREFIX "")
set_target_properties(lf-trace-impl PROPERTIES OUTPUT_NAME "lf-trace-impl")
set_target_properties(lf-trace-impl PROPERTIES SUFFIX ".a")
set_target_properties(lf-trace-impl PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/lib")
set_target_properties(lf-trace-impl PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/lib")
set_target_properties(lf-trace-impl PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/lib")
