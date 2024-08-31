add_library(lf-low-level-platform-api INTERFACE)
target_include_directories(lf-low-level-platform-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::low-level-platform-api ALIAS lf-low-level-platform-api)
target_link_libraries(lf-low-level-platform-api INTERFACE lf::tag-api)

if(${CMAKE_SYSTEM_NAME} STREQUAL "nRF52")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_ZEPHYR)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Patmos")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_PATMOS)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_RP2040)
    target_link_libraries(lf-low-level-platform-api INTERFACE pico_stdlib)
    target_link_libraries(lf-low-level-platform-api INTERFACE pico_multicore)
    target_link_libraries(lf-low-level-platform-api INTERFACE pico_sync)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Stm32")
    set(STM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../STM_sdk)
    set(MCU_FAMILY STM32F4xx)
    set(MCU_MODEL STM32F446xx)
    set(CPU_PARAMETERS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=softfp)

    file(GLOB_RECURSE STM32CUBEMX_SOURCES ${STM_DIR}/Core/*.c ${STM_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver/*.c)
    set(CUBEMX_INCLUDE_DIRECTORIES
        ${STM_DIR}/Core/Inc
        ${STM_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver/Inc
        ${STM_DIR}/Drivers/CMSIS/Device/ST/${MCU_FAMILY}/Include
        ${STM_DIR}/Drivers/CMSIS/Include
    )
    target_compile_options(lf-low-level-platform-api INTERFACE
        ${CPU_PARAMETERS}
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        $<$<COMPILE_LANGUAGE:CXX>:
            -Wno-volatile
            -Wold-style-cast
            -Wuseless-cast
            -Wsuggest-override>
        $<$<CONFIG:Debug>:-Og -g3 -ggdb>
        $<$<CONFIG:Release>:-Og -g0>)

    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_STM32F4)
    target_include_directories(lf-low-level-platform-api INTERFACE ${CUBEMX_INCLUDE_DIRECTORIES})
    target_compile_definitions(lf-low-level-platform-api INTERFACE ${MCU_MODEL} USE_HAL_DRIVER)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FlexPRET")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_FLEXPRET)
    target_link_libraries(lf-low-level-platform-api INTERFACE fp-sdk)
endif()
