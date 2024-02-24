# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE
set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(LF_LOW_LEVEL_PLATFORM_FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_unix_clock_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_unix_syscall_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_linux_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_macos_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_windows_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_nrf52_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_zephyr_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_zephyr_clock_counter.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_zephyr_clock_kernel.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_rp2040_support.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_atomic_windows.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_atomic_gcc_clang.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lf_atomic_irq.c
    ${CMAKE_CURRENT_LIST_DIR}/src/platform_internal.c
)
if(PLATFORM_ZEPHYR)
    message("--- Building Zephyr library")
    zephyr_library_named(lf-low-level-platform-impl)
    zephyr_library_sources(${LF_LOW_LEVEL_PLATFORM_FILES})
    zephyr_library_link_libraries(kernel)
else()
message("--- Building non-Zephyr library")
    add_library(lf-low-level-platform-impl STATIC ${LF_LOW_LEVEL_PLATFORM_FILES})
endif()
add_library(lf::low-level-platform-impl ALIAS lf-low-level-platform-impl)

target_link_libraries(lf-low-level-platform-impl PRIVATE lf::low-level-platform-api)
target_link_libraries(lf-low-level-platform-impl PUBLIC lf-logging-api)

target_compile_definitions(lf-low-level-platform-impl PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})
message(STATUS "Applying preprocessor definitions to platform...")
macro(low_level_platform_define X)
    if(DEFINED ${X})
        message(STATUS ${X}=${${X}})
        target_compile_definitions(lf-low-level-platform-impl PUBLIC ${X}=${${X}})
    endif(DEFINED ${X})
endmacro()
low_level_platform_define(LF_SINGLE_THREADED)
low_level_platform_define(LOG_LEVEL)
low_level_platform_define(MODAL_REACTORS)
low_level_platform_define(USER_THREADS)
low_level_platform_define(NUMBER_OF_WORKERS)
low_level_platform_define(NUMBER_OF_WATCHDOGS)
