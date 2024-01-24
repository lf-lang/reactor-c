cmake_minimum_required(VERSION 3.13..3.22)

project(DefaultTracing LANGUAGES C)

add_library(DefaultTracing STATIC trace.c)
target_include_directories(DefaultTracing PRIVATE ../../include/plugin-apis)
# target_include_directories(DefaultTracing PRIVATE ../../include/)
