## This CMakeLists is used in two ways:
## 1) Included from the reactor-c build (e.g., from `core/CMakeLists.txt`)
## 2) Built standalone by running CMake in `trace/impl/`
##
## When built standalone, we need to create the dependency targets that the
## implementation links against (they are lightweight INTERFACE libraries).

set(_LF_TRACE_IMPL_STANDALONE OFF)
# Check if the trace plugin is compiled standalone.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    cmake_minimum_required(VERSION 3.13)
    project(lf_trace_impl LANGUAGES C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(_LF_TRACE_IMPL_STANDALONE ON)
endif()

get_filename_component(LF_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include(${LF_ROOT}/core/lf_utils.cmake)

# Ensure dependency targets exist when building standalone.
# When this file is included from the reactor-c build, that build is responsible
# for including these modules (and including them twice causes duplicate targets).
if(_LF_TRACE_IMPL_STANDALONE)
    if(NOT TARGET lf::trace-api)
        include(${LF_ROOT}/trace/api/CMakeLists.txt)
    endif()
    if(NOT TARGET lf::platform-api)
        include(${LF_ROOT}/platform/api/CMakeLists.txt)
    endif()
    if(NOT TARGET lf::logging-api)
        include(${LF_ROOT}/logging/api/CMakeLists.txt)
    endif()
    if(NOT TARGET lf::version-api)
        include(${LF_ROOT}/version/api/CMakeLists.txt)
    endif()
endif()

add_library(lf-trace-impl STATIC)
add_library(lf::trace-impl ALIAS lf-trace-impl)
target_link_libraries(lf-trace-impl PRIVATE lf::trace-api)
target_link_libraries(lf-trace-impl PRIVATE lf::platform-api)
target_link_libraries(lf-trace-impl PRIVATE lf::logging-api)
target_link_libraries(lf-trace-impl PRIVATE lf::version-api)
lf_enable_compiler_warnings(lf-trace-impl)

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
