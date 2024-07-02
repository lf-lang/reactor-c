# This adds all tests in the test directory.
include(CTest)

set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_SUFFIX test.c)  # Files that are tests must have names ending with TEST_SUFFIX.
set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(TEST_MOCK_SRCS ${TEST_DIR}/src_gen_stub.c ${TEST_DIR}/rand_utils.c)

include(${LF_ROOT}/core/lf_utils.cmake)

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
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
      add_test_dir(${TEST_DIR}/scheduling)
    endif()
endif(NUMBER_OF_WORKERS)

# Create executables for each test.
foreach(FILE ${TEST_FILES})
    string(REGEX REPLACE "[./]" "_" NAME ${FILE})
    add_executable(${NAME} ${TEST_DIR}/${FILE} ${TEST_MOCK_SRCS})
    add_test(NAME ${NAME} COMMAND ${NAME})
    # This is needed for the tests to use the threading API declared in
    # low_level_platform.h. Ideally this would not be needed.
    target_link_libraries(${NAME} PRIVATE lf::low-level-platform-impl)
    target_link_libraries(
        ${NAME} PRIVATE
        ${CoreLib} ${Lib} m
    )
    target_include_directories(${NAME} PRIVATE ${TEST_DIR})
    # Warnings as errors
    lf_enable_compiler_warnings(${NAME})
endforeach(FILE ${TEST_FILES})
