set(CoreLib ../core)

# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LF_PLATFORM_FILE ${CoreLib}/platform/lf_linux_support.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(LF_PLATFORM_FILE ${CoreLib}/platform/lf_macos_support.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(LF_PLATFORM_FILE ${CoreLib}/platform/lf_windows_support.c)
    set(CMAKE_SYSTEM_VERSION 10.0)
    message("Using Windows SDK version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
else()
    message(FATAL_ERROR "Your platform is not supported! The C target supports Linux, MacOS and Windows.")
endif()

include_directories(../include)
include_directories(${CoreLib})
include_directories(${CoreLib}/platform)
include_directories(${CoreLib}/federated)



# Compile C++ tests (here only one, but it will grab whatever cxx is in the test directory)
file(GLOB_RECURSE TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c)

# Iterate over all tests found. For each, declare an executable and add it to the tests list.
set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
    string(REGEX REPLACE "[./]" "_" NAME ${FILE}) # create exe name
    add_executable(${NAME} ${FILE} ${LF_PLATFORM_FILE} ../core/tag.c ../core/utils/util.c ../core/reactor.c ../core/reactor_common.c ../lib/ctarget.c)
    # target_link_libraries(${NAME} ${PROJECT_NAME}) # ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}) # link against our lib and boost test 
    add_test(NAME ${NAME} COMMAND ${NAME})                              # this is how to add tests to CMake
endforeach(FILE ${TEST_FILES})

include(CTest)

#include(Catch)
#catch_discover_tests(submodule_1_test)

