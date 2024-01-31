cmake_minimum_required(VERSION 3.13..3.22)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/lib")

project(DefaultTracing LANGUAGES C)

add_library(DefaultTracing STATIC default-trace.c)
target_include_directories(DefaultTracing PRIVATE ../../include/plugin-apis)

# make name platform-independent
set_target_properties(DefaultTracing PROPERTIES PREFIX "")
set_target_properties(DefaultTracing PROPERTIES OUTPUT_NAME "defaulttracing")
set_target_properties(DefaultTracing PROPERTIES SUFFIX ".a")
