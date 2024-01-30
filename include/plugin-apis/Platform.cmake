# FIXME: this is a hack. We should be able to compile core as a library and link
# it against the platform support library without having the core depend at
# compile time (via compile definitions) on the platform support library.
# in this case this cmake file would be located here, not in the APIs directory.

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
