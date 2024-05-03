add_library(lf-low-level-platform-api INTERFACE)
target_include_directories(lf-low-level-platform-api INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(lf::low-level-platform-api ALIAS lf-low-level-platform-api)
target_link_libraries(lf-low-level-platform-api INTERFACE lf::tag-api)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_ZEPHYR)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_RP2040)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FlexPRET")
    target_link_libraries(lf-low-level-platform-api INTERFACE fp-sdk-if)
    target_compile_definitions(lf-low-level-platform-api INTERFACE PLATFORM_FLEXPRET)

    # This is explained in issue https://github.com/lf-lang/reactor-c/issues/418
    # The stubs are found in `../impl/src/lf_flexpret_stubs.c`.
    target_link_options(lf-low-level-platform-api INTERFACE
        "-Wl,--wrap=_vfprintf_r" "-Wl,--wrap=__ssvfiscanf_r" "-Wl,--wrap=_svfiprintf_r"
    )

    # This is related to issue https://github.com/lf-lang/reactor-c/issues/418
    # 
    # The ordering of libraries passed to `gcc` matters
    # (https://stackoverflow.com/a/2487723/16358883)
    # Since we want `newlib` to link into our wrapper functions, our library
    # needs to come after `newlib` on the command line. But we cannot choose
    # where `newlib` is placed on the command line - and it is most likely at 
    # the very end (because everything is expected to link into it).
    #
    # Therefore, just adding these three functions normally into the library
    # will yield `undefined reference` errors. 
    #
    # The solution is to instead link in our three functions as an object file.
    # The ordering of object files does not matter. To do this, we add the source
    # as an interface, which means it will be passed onto the main target and
    # compiled as an object file there.
    target_sources(lf-low-level-platform-api INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../impl/src/lf_flexpret_stubs.c
    )
endif()
