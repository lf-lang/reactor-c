include(Platform.cmake)

set(GENERAL_SOURCES ${LF_PLATFORM_FILE})
set(SINGLE_THREADED_SOURCES lf_os_single_threaded_support.c)
add_sources_to_parent(GENERAL_SOURCES "" SINGLE_THREADED_SOURCES)
