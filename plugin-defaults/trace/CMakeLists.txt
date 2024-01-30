cmake_minimum_required(VERSION 3.13..3.22)

project(DefaultTracing LANGUAGES C)

add_library(DefaultTracing STATIC trace.c)
target_include_directories(DefaultTracing PRIVATE ../../include/plugin-apis)
# target_include_directories(DefaultTracing PRIVATE ../../include/)

# FIXME: The following is BAD BAD BAD, breaking encapsulation, the opposite of what we want but hopefully a stepping stone to something better
# target_include_directories(DefaultTracing PRIVATE ../../include/core)
# target_include_directories(DefaultTracing PRIVATE ../../include/core/platform)
