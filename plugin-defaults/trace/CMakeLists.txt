cmake_minimum_required(VERSION 3.13..3.22)

project(DefaultTracing LANGUAGES C)

add_library(DefaultTracing STATIC trace/default-trace.c)
target_include_directories(DefaultTracing PRIVATE ../../include/plugin-apis)
target_include_directories(DefaultTracing PRIVATE ../../include)
target_include_directories(DefaultTracing PRIVATE ./include)

# handle compile-time parameters
if(NOT DEFINED LOG_LEVEL)
    message(FATAL_ERROR "You must set LOG_LEVEL cmake argument")
endif()
target_compile_definitions(DefaultTracing PRIVATE LOG_LEVEL=${LOG_LEVEL})
# build type parameter (release, debug, etc) is implicitly handled by CMake

# make name platform-independent
set_target_properties(DefaultTracing PROPERTIES PREFIX "")
set_target_properties(DefaultTracing PROPERTIES OUTPUT_NAME "defaulttracing")
set_target_properties(DefaultTracing PROPERTIES SUFFIX ".a")
set_target_properties(DefaultTracing PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/lib")
set_target_properties(DefaultTracing PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/lib")
set_target_properties(DefaultTracing PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/lib")
