add_test_dir(${CMAKE_CURRENT_LIST_DIR} TEST_FILES)

# Create executables for each test.
foreach(FILE ${TEST_FILES})
    string(REGEX REPLACE "[./]" "_" NAME ${FILE})
    message(STATUS "Creating test executable: ${NAME}")
    add_executable(${NAME} ${TEST_DIR}/${FILE})
    add_test(NAME ${NAME} COMMAND ${NAME})
    target_link_libraries(
        ${NAME} PUBLIC
        ${CoreLib} ${Lib} ${TestLib}
    )
    target_include_directories(${NAME} PRIVATE ${TEST_DIR})
endforeach(FILE ${TEST_FILES})
