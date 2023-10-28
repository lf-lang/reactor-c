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

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_SYSTEM_VERSION 10.0)
    message("Using Windows SDK version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    target_compile_definitions(core PUBLIC PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    target_compile_definitions(core PUBLIC PLATFORM_ZEPHYR)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    target_compile_definitions(core PUBLIC PLATFORM_RP2040)
endif()

# Add sources to the list for debug info
list(APPEND INFO_SOURCES ${LF_PLATFORM_FILES})

# Prepend all sources with platform
list(TRANSFORM LF_PLATFORM_FILES PREPEND platform/)

# Add sources to core lib
target_sources(core PRIVATE ${LF_PLATFORM_FILES})
