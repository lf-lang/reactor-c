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
