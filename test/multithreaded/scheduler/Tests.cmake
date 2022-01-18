set(SCHEDULER_PREFIX scheduler_)
set(SCHEDULER_PATH ${CMAKE_SOURCE_DIR}/${CoreLib}/threaded)
# Find all the implemented schedulers
file(
    GLOB IMPLEMENTED_SCHEDULERS
    RELATIVE ${CMAKE_SOURCE_DIR}/${CoreLib}/threaded
    ${SCHEDULER_PATH}/${SCHEDULER_PREFIX}*)

# For each scheduler
foreach(SCHED ${IMPLEMENTED_SCHEDULERS})
    # Create a executable for each scheduelr.
    string(REGEX REPLACE "[.${SCHEDULER_PREFIX}]" "" SCHED_NAME ${SCHED})
    string(CONCAT NAME "scheduler_api_test_c_" ${SCHED_NAME})
    message(STATUS "${NAME}")
    add_executable(${NAME} ${TEST_DIR}/${FILE} ${SCHEDULER_PATH}/${SCHED})
    add_test(NAME ${NAME} COMMAND ${NAME})
    target_link_libraries(
        ${NAME} PUBLIC
        ${CoreLib} ${Lib} ${TestLib}
    )
    target_include_directories(${NAME} PRIVATE ${TEST_DIR})
endforeach(SCHED ${IMPLEMENTED_SCHEDULERS})


