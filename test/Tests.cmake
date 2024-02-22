# This adds all tests in the test directory.
include(CTest)

set(TestLib test-lib)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_SUFFIX test.c)  # Files that are tests must have names ending with TEST_SUFFIX.
set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

# Add the test files found in DIR to TEST_FILES.
function(add_test_dir DIR)
    file(
        GLOB_RECURSE TEST_FILES_FOR_DIR
        LIST_DIRECTORIES false
        RELATIVE ${TEST_DIR}
        ${DIR}/*${TEST_SUFFIX}
    )
    list(APPEND TEST_FILES ${TEST_FILES_FOR_DIR})
    set(TEST_FILES ${TEST_FILES} PARENT_SCOPE)
endfunction()

# Add the appropriate directories for the provided build parameters.
add_test_dir(${TEST_DIR}/general)
if(NUMBER_OF_WORKERS)
    add_test_dir(${TEST_DIR}/multithreaded)
else()
    add_test_dir(${TEST_DIR}/single-threaded)
endif(NUMBER_OF_WORKERS)

# Create executables for each test.
foreach(FILE ${TEST_FILES})
    string(REGEX REPLACE "[./]" "_" NAME ${FILE})
    add_executable(${NAME} ${TEST_DIR}/${FILE})
    add_test(NAME ${NAME} COMMAND ${NAME})
    # include(${LF_ROOT}/low_level_platform/api/CMakeLists.txt)
    target_link_libraries(
        ${NAME} PUBLIC
        ${CoreLib} ${Lib} ${TestLib}
    )
    target_include_directories(${NAME} PRIVATE ${TEST_DIR})
endforeach(FILE ${TEST_FILES})

# Add the test for the RTI.
if (NOT DEFINED LF_SINGLE_THREADED)
    # Check which system we are running on to select the correct platform support
    # file and assign the file's path to LF_PLATFORM_FILE
    # FIXME: This is effectively a second build script for the RTI that we have to maintain. This is code duplication.
    # FIXME: We should not be reaching into the platform directory and bypassing its CMake build.
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
      set(LF_PLATFORM_FILE ${LF_ROOT}/low_level_platform/impl/src/lf_linux_support.c)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      set(LF_PLATFORM_FILE ${LF_ROOT}/low_level_platform/impl/src/lf_macos_support.c)
    else()
      message(FATAL_ERROR "Your platform is not supported! RTI supports Linux and MacOS.")
    endif()

    set(IncludeDir include/core)

    set(RTI_DIR ${CoreLibPath}/federated/RTI)
    add_executable(
      rti_common_test
      ${TEST_DIR}/RTI/rti_common_test.c
      ${RTI_DIR}/rti_common.c
      ${RTI_DIR}/rti_remote.c
      ${CoreLibPath}/trace.c
      ${LF_PLATFORM_FILE}
      ${LF_ROOT}/low_level_platform/impl/src/lf_atomic_gcc_clang.c
      ${LF_ROOT}/low_level_platform/impl/src/lf_unix_clock_support.c
      ${CoreLibPath}/utils/util.c
      ${CoreLibPath}/tag.c
      ${CoreLibPath}/clock.c
      ${CoreLibPath}/federated/network/net_util.c
      ${CoreLibPath}/utils/pqueue_base.c
      ${CoreLibPath}/utils/pqueue_tag.c
      ${CoreLibPath}/utils/pqueue.c
    )
    add_test(NAME rti_common_test COMMAND rti_common_test)
    target_include_directories(rti_common_test PUBLIC ${RTI_DIR})
    target_include_directories(rti_common_test PUBLIC ${IncludeDir})
    target_include_directories(rti_common_test PUBLIC ${IncludeDir}/federated)
    target_include_directories(rti_common_test PUBLIC ${IncludeDir}/modal_models)
    target_include_directories(rti_common_test PUBLIC ${IncludeDir}/platform)
    target_include_directories(rti_common_test PUBLIC ${IncludeDir}/utils)
    # Set the STANDALONE_RTI flag to include the rti_remote and rti_common.
    target_compile_definitions(rti_common_test PUBLIC STANDALONE_RTI=1)

    # Set FEDERATED to get federated compilation support
    target_compile_definitions(rti_common_test PUBLIC FEDERATED=1)

    target_compile_definitions(rti_common_test PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})

    # Find threads and link to it
    find_package(Threads REQUIRED)
    target_link_libraries(rti_common_test Threads::Threads)
endif()
