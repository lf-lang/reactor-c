# This adds all tests in the test directory.
include(CTest)

set(TestLib test-lib)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_SUFFIX test.c)  # Files that are tests must have names ending with TEST_SUFFIX.

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
    target_link_libraries(
        ${NAME} PUBLIC
        ${CoreLib} ${Lib} ${TestLib}
    )
    target_include_directories(${NAME} PRIVATE ${TEST_DIR})
endforeach(FILE ${TEST_FILES})

# Build a test for the RTI.
# Check which system we are running on to select the correct platform support
# file and assign the file's path to LF_PLATFORM_FILE
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(LF_PLATFORM_FILE ${CoreLib}/platform/lf_linux_support.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(LF_PLATFORM_FILE ${CoreLib}/platform/lf_macos_support.c)
else()
  message(FATAL_ERROR "Your platform is not supported! RTI supports Linux and MacOS.")
endif()

set(IncludeDir include/core)

set(RTI_DIR ${CoreLib}/federated/RTI)
add_executable(
  EIMTCalculation_test
  ${TEST_DIR}/RTI/EIMTCalculation_test.c
  ${RTI_DIR}/rti_common.c
  ${RTI_DIR}/rti_remote.c
  ${CoreLib}/trace.c
  ${LF_PLATFORM_FILE}
  ${CoreLib}/platform/lf_unix_clock_support.c
  ${CoreLib}/utils/util.c
  ${CoreLib}/tag.c
  ${CoreLib}/federated/network/net_util.c
  ${CoreLib}/utils/pqueue_base.c
  ${CoreLib}/utils/pqueue_tag.c
  ${CoreLib}/utils/pqueue.c
  ${RTI_DIR}/message_record/message_record.c
)
add_test(NAME EIMTCalculation_test COMMAND EIMTCalculation_test)
target_include_directories(EIMTCalculation_test PUBLIC ${RTI_DIR})
target_include_directories(EIMTCalculation_test PUBLIC ${IncludeDir})
target_include_directories(EIMTCalculation_test PUBLIC ${IncludeDir}/federated)
target_include_directories(EIMTCalculation_test PUBLIC ${IncludeDir}/modal_models)
target_include_directories(EIMTCalculation_test PUBLIC ${IncludeDir}/platform)
target_include_directories(EIMTCalculation_test PUBLIC ${IncludeDir}/utils)
# Set the STANDALONE_RTI flag to include the rti_remote and rti_common.
target_compile_definitions(EIMTCalculation_test PUBLIC STANDALONE_RTI=1)

# Set FEDERATED to get federated compilation support
target_compile_definitions(EIMTCalculation_test PUBLIC FEDERATED=1)

target_compile_definitions(EIMTCalculation_test PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})

# Set RTI Tracing
# target_compile_definitions(EIMTCalculation_test PUBLIC RTI_TRACE)

# Find threads and link to it
find_package(Threads REQUIRED)
target_link_libraries(EIMTCalculation_test Threads::Threads)