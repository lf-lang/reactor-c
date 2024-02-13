cmake_minimum_required(VERSION 3.13)
project(ReactorC LANGUAGES C)

# Require C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Currently this build process is used for testing, not production.
set(DEFAULT_BUILD_TYPE Debug)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING "Choose the type of build." FORCE)
endif()

# Whether or not we are using THREADED or SINGLE-THREADED runtime is a global
# flag. If it is passed to cmake. Then add it as a global compile def which
# is inherited by all child nodes (add_subdirectory)
if(DEFINED LF_SINGLE_THREADED)
    add_compile_definitions(LF_SINGLE_THREADED=1)
endif()

# Warnings as errors
add_compile_options(-Werror)

set(Test test)
set(Lib lib)
set(CoreLibPath core)
set(CoreLib reactor-c)
set(PlatformLib platform)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/core)
include_directories(${CMAKE_SOURCE_DIR}/include/core/federated)
include_directories(${CMAKE_SOURCE_DIR}/include/core/federated/network)
include_directories(${CMAKE_SOURCE_DIR}/include/core/modal_models)
include_directories(${CMAKE_SOURCE_DIR}/include/core/platform)
include_directories(${CMAKE_SOURCE_DIR}/include/core/threaded)
include_directories(${CMAKE_SOURCE_DIR}/include/core/utils)
include_directories(${CMAKE_SOURCE_DIR}/include/api)

enable_testing()
add_subdirectory(${Test})
add_subdirectory(${Lib})
add_subdirectory(${CoreLibPath})

include(test/Tests.cmake)
