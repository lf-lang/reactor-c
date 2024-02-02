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
    ${CMAKE_CURRENT_LIST_DIR}/platform_internal.c
)
if(PLATFORM_ZEPHYR)
    message("--- Building Zephyr library")
    zephyr_library_named(LfPlatform)
    zephyr_library_sources(${LF_PLATFORM_FILES})
    zephyr_library_link_libraries(kernel)
else()
message("--- Building non-Zephyr library")
    add_library(LfPlatform STATIC ${LF_PLATFORM_FILES})
endif()
# add_library(LfPlatform STATIC ${LF_PLATFORM_FILES})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    target_compile_definitions(LfPlatform PUBLIC PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    target_compile_definitions(LfPlatform PUBLIC PLATFORM_ZEPHYR)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    target_compile_definitions(LfPlatform PUBLIC PLATFORM_RP2040)
endif()

target_compile_definitions(LfPlatform PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})
message(STATUS "Applying preprocessor definitions to platform...")
macro(platform_define X)
    if(DEFINED ${X})
        message(STATUS ${X}=${${X}})
        target_compile_definitions(LfPlatform PUBLIC ${X}=${${X}})
    endif(DEFINED ${X})
endmacro()
platform_define(LF_SINGLE_THREADED)
platform_define(LOG_LEVEL)
platform_define(MODAL_REACTORS)
platform_define(USER_THREADS)
platform_define(NUMBER_OF_WORKERS)

target_include_directories(LfPlatform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include)
target_include_directories(LfPlatform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include/core)
target_include_directories(LfPlatform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include/core/platform)
target_include_directories(LfPlatform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include/core/utils)

# # Add sources to core lib
# target_sources(react PRIVATE ${LF_PLATFORM_FILES})
