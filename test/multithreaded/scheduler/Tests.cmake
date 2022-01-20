set(SCHEDULER_PREFIX scheduler_)
set(SCHEDULER_PATH ${CMAKE_SOURCE_DIR}/${CoreLib}/threaded)
# Find all the implemented schedulers
file(
    GLOB IMPLEMENTED_SCHEDULERS
    RELATIVE ${CMAKE_SOURCE_DIR}/${CoreLib}/threaded
    ${SCHEDULER_PATH}/${SCHEDULER_PREFIX}*)

# Add all the test files in the current folder with a suffix of _test.c to be tested
add_test_dir(${CMAKE_CURRENT_LIST_DIR} SCHED_TEST_FILES)

# For each scheduler
foreach(SCHED ${IMPLEMENTED_SCHEDULERS})
    # For each test file in this folder
    foreach(TEST ${SCHED_TEST_FILES})
        # Create a executable for each scheduelr.
        string(REGEX REPLACE "[.${SCHEDULER_PREFIX}]" "" SCHED_NAME ${SCHED})
        string(REGEX REPLACE "[./]" "_" NEW_TEST ${TEST})
        string(CONCAT NAME "${NEW_TEST}_" ${SCHED_NAME})
        message(STATUS "${NAME}")
        add_executable(${NAME} ${TEST_DIR}/${TEST} ${SCHEDULER_PATH}/${SCHED})
        add_test(NAME ${NAME} COMMAND ${NAME})
        target_link_libraries(
            ${NAME} PUBLIC
            ${CoreLib} ${Lib} ${TestLib}
        )
        target_include_directories(${NAME} PRIVATE ${TEST_DIR})
    endforeach(TEST ${SCHED_TEST_FILES})
endforeach(SCHED ${IMPLEMENTED_SCHEDULERS})


