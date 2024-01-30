cmake_minimum_required(VERSION 3.13..3.22)

project(DefaultPlatform LANGUAGES C)

set(LF_PLATFORM_FILES
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_unix_clock_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_unix_syscall_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_linux_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_macos_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_windows_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_nrf52_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_zephyr_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_zephyr_clock_counter.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_zephyr_clock_kernel.c
    ${CMAKE_CURRENT_LIST_DIR}/platform/lf_rp2040_support.c
)

add_library(DefaultPlatform STATIC ${LF_PLATFORM_FILES})
target_include_directories(DefaultPlatform PRIVATE ../../include/plugin-apis)
target_include_directories(DefaultPlatform PRIVATE ../../include/api)
target_include_directories(DefaultPlatform PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

# Link with thread library, unless we are on the Zephyr platform.
if(NOT DEFINED LF_SINGLE_THREADED OR DEFINED LF_TRACE)
    if (NOT PLATFORM_ZEPHYR)
        find_package(Threads REQUIRED)
        target_link_libraries(DefaultPlatform PUBLIC Threads::Threads)
    endif()
endif()

target_compile_definitions(DefaultPlatform PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})

macro(define X)
    if(DEFINED ${X})
        message(STATUS ${X}=${${X}})
        target_compile_definitions(DefaultPlatform PUBLIC ${X}=${${X}})
    endif(DEFINED ${X})
endmacro()

define(LF_SINGLE_THREADED)
define(USER_THREADS)

# FIXME: This is a hack, see the comment in Platform.cmake
MESSAGE(INFO ${CMAKE_CURRENT_LIST_DIR}/../../include/plugin-apis/Platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../include/plugin-apis/Platform.cmake)
