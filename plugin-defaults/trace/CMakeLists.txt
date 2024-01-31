cmake_minimum_required(VERSION 3.13..3.22)

project(DefaultTracing LANGUAGES C)

add_library(DefaultTracing STATIC trace.c)
target_include_directories(DefaultTracing PRIVATE ../../include/plugin-apis)

# make name platform-independent
set_target_properties(DefaultTracing PROPERTIES PREFIX "")
set_target_properties(DefaultTracing PROPERTIES OUTPUT_NAME "defaulttracing")
set_target_properties(DefaultTracing PROPERTIES SUFFIX ".a")
