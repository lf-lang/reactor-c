# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE

set(LF_UNIX_FILES lf_unix_clock_support.c lf_unix_syscall_support.c)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LF_PLATFORM_FILES lf_linux_support.c ${LF_UNIX_FILES})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(LF_PLATFORM_FILES lf_macos_support.c ${LF_UNIX_FILES})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(LF_PLATFORM_FILES lf_windows_support.c)
    set(CMAKE_SYSTEM_VERSION 10.0)
    message("Using Windows SDK version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    set(LF_PLATFORM_FILES lf_nrf52_support.c)
    target_compile_definitions(core PUBLIC PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    set(LF_PLATFORM_FILES lf_zephyr_support.c)
    target_compile_definitions(core PUBLIC PLATFORM_ZEPHYR)
else()
    message(FATAL_ERROR "Your platform is not supported! The C target supports Linux, MacOS, Windows, Arduino, Nrf52 and Zephyr.")
endif()

# Add sources to the list for debug info
list(APPEND INFO_SOURCES ${LF_PLATFORM_FILES})

# Prepend all sources with platform
list(TRANSFORM LF_PLATFORM_FILES PREPEND platform/)

# Add sources to core lib
target_sources(core PRIVATE ${LF_PLATFORM_FILES})