# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE

set(LF_PLATFORM_FILES
    lf_unix_clock_support.c
    lf_linux_support.c
    lf_macos_support.c
    lf_windows_support.c
    lf_nrf52_support.c
    lf_zephyr_support.c
    lf_zephyr_clock_counter.c
    lf_zephyr_clock_kernel.c
    lf_rp2040_support.c
    lf_atomic_windows.c
    lf_atomic_gcc_clang.c
    lf_atomic_irq.c
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_ZEPHYR)
    set(PLATFORM_ZEPHYR true)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_RP2040)
endif()

# Prepend all sources with platform
list(TRANSFORM LF_PLATFORM_FILES PREPEND platform/)

# Add sources to the list for debug info
list(APPEND REACTORC_SOURCES ${LF_PLATFORM_FILES})
