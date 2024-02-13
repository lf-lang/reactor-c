# This is a cmake build script providing a solution for compiling
# the RTI in this directory..
#
# Usage:
#
# To compile with cmake, run the following commands:
#
# $> mkdir build && cd build
# $> cmake ../
# $> make
# $> sudo make install
#
# This create a binary RTI in the current working directory. Please put this in
# a directory that is on the path.
#
# To enable DEBUG messages, use the following build commands instead:
#
# $> mkdir build && cd build
# $> cmake -DCMAKE_BUILD_TYPE=DEBUG ../
# $> make
# $> sudo make install
#
# If you would like to go back to non-DEBUG mode, you would have to remove all
# contents of the `build` folder.

# To enable simple HMAC-based authentication of federates,
# add `-DAUTH=ON` option to the cmake command as shown below:
# 
# $> mkdir build && cd build
# $> cmake -DAUTH=ON ../
# $> make
# $> sudo make install
#
# If you would like to go back to non-AUTH mode, you would have to remove all
# contents of the `build` folder.

cmake_minimum_required(VERSION 3.12)
project(RTI VERSION 1.0.0 LANGUAGES C)

set(CoreLib ../../../core)

# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(LF_PLATFORM_FILE ${CoreLib}/platform/lf_linux_support.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(LF_PLATFORM_FILE ${CoreLib}/platform/lf_macos_support.c)
else()
  message(FATAL_ERROR "Your platform is not supported! RTI supports Linux and MacOS.")
endif()

set(IncludeDir ../../../include/core)
include_directories(${IncludeDir})
include_directories(${IncludeDir}/federated)
include_directories(${IncludeDir}/federated/network)
include_directories(${IncludeDir}/modal_models)
include_directories(${IncludeDir}/platform)
include_directories(${IncludeDir}/utils)


# Declare a new executable target and list all its sources
add_executable(
    RTI
    main.c
    rti_common.c
    rti_remote.c
    ${CoreLib}/trace.c
    ${LF_PLATFORM_FILE}
    ${CoreLib}/platform/lf_atomic_gcc_clang.c
    ${CoreLib}/platform/lf_unix_clock_support.c
    ${CoreLib}/utils/util.c
    ${CoreLib}/tag.c
    ${CoreLib}/clock.c
    ${CoreLib}/federated/network/net_util.c
    ${CoreLib}/utils/pqueue_base.c
    ${CoreLib}/utils/pqueue_tag.c
    ${CoreLib}/utils/pqueue.c
)

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    # Set the LOG_LEVEL to 4 to get DEBUG messages
    message("-- Building RTI with DEBUG messages enabled")
    target_compile_definitions(RTI PUBLIC LOG_LEVEL=4)
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

# Set the STANDALONE_RTI flag to include the rti_remote and rti_common.
target_compile_definitions(RTI PUBLIC STANDALONE_RTI=1)

# Set FEDERATED to get federated compilation support
target_compile_definitions(RTI PUBLIC FEDERATED=1)

target_compile_definitions(RTI PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})

# Set RTI Tracing
target_compile_definitions(RTI PUBLIC RTI_TRACE)

# Warnings as errors
target_compile_options(RTI PUBLIC -Werror)
# Find threads and link to it
find_package(Threads REQUIRED)
target_link_libraries(RTI Threads::Threads)

# Option for enabling federate authentication by RTI.
option(AUTH "Federate authentication by RTI enabled." OFF)
IF(AUTH MATCHES ON)
  add_compile_definitions(__RTI_AUTH__)
  # Find OpenSSL and link to it
  find_package(OpenSSL REQUIRED)
  target_link_libraries(RTI OpenSSL::SSL)
ENDIF(AUTH MATCHES ON)

install(
    TARGETS RTI
    DESTINATION bin
)
