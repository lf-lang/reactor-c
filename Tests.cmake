# This adds all tests in the test directory.
include(CTest)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
file(
    GLOB_RECURSE TEST_FILES
    LIST_DIRECTORIES false
    RELATIVE ${TEST_DIR}
    ${TEST_DIR}/*.c
)

set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
    string(REGEX REPLACE "[./]" "_" NAME ${FILE})
    add_executable(${NAME} ${TEST_DIR}/${FILE})
    add_test(NAME ${NAME} COMMAND ${NAME})
    target_link_libraries(${NAME} PUBLIC ${CoreLib} ${PlatformLib} ${Lib})
    target_include_directories(
        ${NAME} PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}/${NAME}"
    )
endforeach(FILE ${TEST_FILES})
