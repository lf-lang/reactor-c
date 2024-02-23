set(LF_PLATFORM_FILES ${CMAKE_CURRENT_LIST_DIR}/platform.c)


if(PLATFORM_ZEPHYR)
    message("--- Building Zephyr library")
    zephyr_library_named(lf-platform-impl)
    zephyr_library_sources(${LF_PLATFORM_FILES})
    zephyr_library_link_libraries(kernel)
else()
message("--- Building non-Zephyr library")
    add_library(lf-platform-impl STATIC)
    target_sources(lf-platform-impl PUBLIC ${LF_PLATFORM_FILES})
endif()

add_library(lf::platform-impl ALIAS lf-platform-impl)
target_link_libraries(lf-platform-impl PRIVATE lf::low-level-platform-api)
target_link_libraries(lf-platform-impl PRIVATE lf::platform-api)
