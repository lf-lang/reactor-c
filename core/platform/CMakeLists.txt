# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE

set(LF_PLATFORM_FILES
    lf_unix_clock_support.c
    lf_unix_syscall_support.c
    lf_linux_support.c
    lf_macos_support.c
    lf_windows_support.c
    lf_nrf52_support.c
    lf_zephyr_support.c
    lf_zephyr_clock_counter.c
    lf_zephyr_clock_kernel.c
    lf_rp2040_support.c
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LF_ATOMIC_FILE lf_atomic64.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(LF_ATOMIC_FILE lf_atomic64.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_SYSTEM_VERSION 10.0)
    message("Using Windows SDK version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
    set(LF_ATOMIC_FILE lf_atomic64.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_NRF52)
    set(LF_ATOMIC_FILE lf_atomic32.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_ZEPHYR)
    set(LF_ATOMIC_FILE lf_atomic32.c)
    set(PLATFORM_ZEPHYR true)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    set(LF_ATOMIC_FILE lf_atomic32.c)
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_RP2040)
else()
    message(FATAL_ERROR "Your platform is not supported! The C target supports Linux, MacOS and Windows.")
endif()

# Add the atomics implementation
list(APPEND LF_PLATFORM_FILES ${LF_ATOMIC_FILE})

# Prepend all sources with platform
list(TRANSFORM LF_PLATFORM_FILES PREPEND platform/)

# Add sources to the list for debug info
list(APPEND REACTORC_SOURCES ${LF_PLATFORM_FILES})
