# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE

set(LF_PLATFORM_FILES
    ${CMAKE_CURRENT_LIST_DIR}/lf_unix_clock_support.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_unix_syscall_support.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_linux_support.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_macos_support.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_windows_support.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_nrf52_support.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_zephyr_support.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_zephyr_clock_counter.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_zephyr_clock_kernel.c
    ${CMAKE_CURRENT_LIST_DIR}/lf_rp2040_support.c
    ${CMAKE_CURRENT_LIST_DIR}/platform.c
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_SYSTEM_VERSION 10.0)
    message("Using Windows SDK version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_ZEPHYR)
    set(PLATFORM_ZEPHYR true)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_RP2040)
endif()

# # Add sources to the list for debug info
# list(APPEND INFO_SOURCES ${LF_PLATFORM_FILES})

# # Prepend all sources with platform
# list(TRANSFORM LF_PLATFORM_FILES PREPEND platform/)

add_library(Platform STATIC ${LF_PLATFORM_FILES})
target_compile_definitions(Platform PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})
message(STATUS "Applying preprocessor definitions to platform...")
macro(platform_define X)
    if(DEFINED ${X})
        message(STATUS ${X}=${${X}})
        target_compile_definitions(Platform PUBLIC ${X}=${${X}})
    endif(DEFINED ${X})
endmacro()
platform_define(LF_SINGLE_THREADED)
platform_define(LOG_LEVEL)
platform_define(MODAL_REACTORS)
platform_define(USER_THREADS)

target_include_directories(Platform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include)
target_include_directories(Platform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include/core)
target_include_directories(Platform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include/core/platform)
target_include_directories(Platform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include/core/utils)

# # Add sources to core lib
# target_sources(react PRIVATE ${LF_PLATFORM_FILES})
