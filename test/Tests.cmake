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

# Add the appropriate directories FIXME: Find a way to automaticall find all
# .cmake files.
include(${TEST_DIR}/general/Tests.cmake)

if(NUMBER_OF_WORKERS)
    include(${TEST_DIR}/multithreaded/scheduler/Tests.cmake)
endif(NUMBER_OF_WORKERS)