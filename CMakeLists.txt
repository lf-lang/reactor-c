cmake_minimum_required(VERSION 3.13)
project(ReactorC LANGUAGES C)

# Require C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEFAULT_BUILD_TYPE Release)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING "Choose the type of build." FORCE)
endif()

set(Lib lib)
set(CoreLib core)
set(PlatformLib platform)

enable_testing()
add_subdirectory(test)
add_subdirectory(${Lib})
add_subdirectory(${CoreLib})

include(Tests.cmake)
