set(CORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Appends the provided sources to the SOURCES variable of the parent.
macro(add_sources_to_parent GENERAL_SOURCES MULTITHREADED_SOURCES SINGLE_THREADED_SOURCES)
    set(TEMP ${${GENERAL_SOURCES}})
    if(DEFINED NUMBER_OF_WORKERS)
        list(APPEND TEMP ${${MULTITHREADED_SOURCES}})
    else()
        list(APPEND TEMP ${${SINGLE_THREADED_SOURCES}})
    endif()
    foreach(FILE ${TEMP})
        file(RELATIVE_PATH RELATIVE_TO_ROOT ${CORE_ROOT} ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        list(APPEND SOURCES ${RELATIVE_TO_ROOT})
    endforeach()
    set(SOURCES ${SOURCES} PARENT_SCOPE)
endmacro()

# Applies the variable whose name is X as a preprocessor definition,
#  if that variable is defined.
macro(define X)
    if(DEFINED ${X})
        message(STATUS ${X}=${${X}})
        add_compile_definitions(${X}=${${X}})
    endif(DEFINED ${X})
endmacro()

# FIXME: May want these to be application dependent, hence passed as
# parameters to Cmake.
add_compile_definitions(INITIAL_EVENT_QUEUE_SIZE=10)
add_compile_definitions(INITIAL_REACT_QUEUE_SIZE=10)

message(STATUS "Applying preprocessor definitions...")
define(_LF_CLOCK_SYNC_ATTENUATION)
define(_LF_CLOCK_SYNC_COLLECT_STATS)
define(_LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL)
define(_LF_CLOCK_SYNC_INITIAL)
define(_LF_CLOCK_SYNC_ON)
define(_LF_CLOCK_SYNC_PERIOD_NS)
define(ADVANCE_MESSAGE_INTERVAL)
define(FEDERATED_CENTRALIZED)
define(FEDERATED_DECENTRALIZED)
define(FEDERATED)
define(LF_REACTION_GRAPH_BREADTH)
define(LINGUA_FRANCA_TRACE)
define(LOG_LEVEL)
define(MODAL_REACTORS)
define(NUMBER_OF_FEDERATES)
define(NUMBER_OF_WORKERS)
define(SCHEDULER)
define(TARGET_FILES_DIRECTORY)
define(WORKERS_NEEDED_FOR_FEDERATE)
message(STATUS "")

# List sources in this directory.
list(APPEND SINGLE_THREADED_SOURCES reactor.c)
list(APPEND GENERAL_SOURCES tag.c port.c mixed_radix.c reactor_common.c lf_token.c)
if (DEFINED LINGUA_FRANCA_TRACE)
    message(STATUS "Including sources specific to tracing.")
    list(APPEND GENERAL_SOURCES trace.c)
endif()


# Add sources from subdirectories.
add_subdirectory(platform)
add_subdirectory(utils)
add_subdirectory(utils/hashset)
add_subdirectory(modal_models)
if(DEFINED NUMBER_OF_WORKERS)
    message(STATUS "Including sources for threaded runtime with \
${NUMBER_OF_WORKERS} worker(s) with scheduler=${SCHEDULER} and \
tracing=${LINGUA_FRANCA_TRACE}.")
    add_subdirectory(threaded)
else()
    message(STATUS "Including sources for unthreaded runtime.")
    list(APPEND SOURCES ${SINGLE_THREADED_SOURCES})
endif()

list(APPEND SOURCES ${GENERAL_SOURCES})

# Create the library "core".
list(JOIN SOURCES ", " PRINTABLE_SOURCE_LIST)
message(STATUS "Including the following sources: " ${PRINTABLE_SOURCE_LIST})
add_library(core ${SOURCES})

target_include_directories(core PUBLIC ../include)
target_include_directories(core PUBLIC ../include/core)
target_include_directories(core PUBLIC ../include/core/federated)
target_include_directories(core PUBLIC ../include/core/platform)
target_include_directories(core PUBLIC ../include/core/modal_models)
target_include_directories(core PUBLIC ../include/core/threaded)
target_include_directories(core PUBLIC ../include/core/utils)

if(DEFINED NUMBER_OF_WORKERS OR DEFINED LINGUA_FRANCA_TRACE)
    find_package(Threads REQUIRED)
    target_link_libraries(core PUBLIC Threads::Threads)
endif()
