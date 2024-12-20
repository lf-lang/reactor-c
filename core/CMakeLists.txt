set(CORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

include(${LF_ROOT}/core/lf_utils.cmake)

# Get the general common sources for reactor-c
list(APPEND GENERAL_SOURCES tag.c clock.c port.c mixed_radix.c reactor_common.c lf_token.c environment.c)

# Add tracing support if requested
if(DEFINED LF_TRACE)
    message(STATUS "Including sources specific to tracing.")
    list(APPEND GENERAL_SOURCES tracepoint.c)
endif()

# Add the general sources to the list of REACTORC_SOURCES
list(APPEND REACTORC_SOURCES ${GENERAL_SOURCES})

# Add sources for either threaded or single-threaded runtime
if(DEFINED FEDERATED)
    include(federated/CMakeLists.txt)
    include(federated/network/CMakeLists.txt)
endif()

# Add sources for either threaded or single-threaded runtime
if(DEFINED LF_SINGLE_THREADED)
    message(STATUS "Including sources for single-threaded runtime.")
    list(APPEND SINGLE_THREADED_SOURCES reactor.c)
    list(APPEND REACTORC_SOURCES ${SINGLE_THREADED_SOURCES})
else()
    message(STATUS "Including sources for threaded runtime with \
${NUMBER_OF_WORKERS} worker(s) with scheduler=${SCHEDULER} and \
tracing=${LF_TRACE}.")
    include(threaded/CMakeLists.txt)
endif()

# Add sources for the local RTI if we are using scheduling enclaves
if(DEFINED LF_ENCLAVES)
    include(federated/RTI/local_rti.cmake)
endif()

# Include sources from subdirectories
include(utils/CMakeLists.txt)

if(DEFINED MODAL_REACTORS)
    include(modal_models/CMakeLists.txt)
endif()

# Print sources used for compilation
list(JOIN REACTORC_SOURCES ", " PRINTABLE_SOURCE_LIST)
message(STATUS "Including the following sources: " ${PRINTABLE_SOURCE_LIST})

add_library(reactor-c)
target_sources(reactor-c PRIVATE ${REACTORC_SOURCES})
lf_enable_compiler_warnings(reactor-c)

if(DEFINED LF_TRACE)
    include(${LF_ROOT}/trace/api/CMakeLists.txt)
    target_link_libraries(reactor-c PUBLIC lf::trace-api)
    # If the user specified an external trace plugin. Find it and link with it
    if (LF_TRACE_PLUGIN)
        message(STATUS "Linking trace plugin library ${LF_TRACE_PLUGIN}")
        find_library(TRACE_LIB NAMES ${LF_TRACE_PLUGIN} HINTS "${LF_ROOT}")
        if (NOT TRACE_LIB)
            message(FATAL_ERROR "The trace plugin library ${LF_TRACE_PLUGIN} not found")
        endif()
        # We also link with libdl because it is needed for some platforms.
        # TODO: Figure out why this is the case and how to avoid it.
        target_link_libraries(reactor-c PRIVATE ${TRACE_LIB} dl)
    else()
    # If not, use the default implementation
        message(STATUS "Linking with default trace implementation")
        include(${LF_ROOT}/trace/impl/CMakeLists.txt)
        target_link_libraries(reactor-c PRIVATE lf::trace-impl)
    endif()
else()
    include(${LF_ROOT}/trace/api/types/CMakeLists.txt)
    target_link_libraries(reactor-c PUBLIC lf::trace-api-types)
endif()

include(${LF_ROOT}/version/api/CMakeLists.txt)
target_link_libraries(reactor-c PUBLIC lf::version-api)

include(${LF_ROOT}/logging/api/CMakeLists.txt)
target_link_libraries(reactor-c PUBLIC lf::logging-api)

include(${LF_ROOT}/tag/api/CMakeLists.txt)
target_link_libraries(reactor-c PUBLIC lf::tag-api)

include(${LF_ROOT}/low_level_platform/api/CMakeLists.txt)
include(${LF_ROOT}/low_level_platform/impl/CMakeLists.txt)
target_link_libraries(reactor-c PUBLIC lf::low-level-platform-api)
target_link_libraries(reactor-c PRIVATE lf::low-level-platform-impl)

include(${LF_ROOT}/platform/api/CMakeLists.txt)
include(${LF_ROOT}/platform/impl/CMakeLists.txt)
target_link_libraries(reactor-c PUBLIC lf::platform-api)
target_link_libraries(reactor-c PRIVATE lf::platform-impl)

target_include_directories(reactor-c PUBLIC ../include)
target_include_directories(reactor-c PUBLIC ../include/core)
target_include_directories(reactor-c PUBLIC ../include/core/federated)
target_include_directories(reactor-c PUBLIC ../include/core/federated/network)
target_include_directories(reactor-c PUBLIC ../include/core/platform)
target_include_directories(reactor-c PUBLIC ../include/core/modal_models)
target_include_directories(reactor-c PUBLIC ../include/core/threaded)
target_include_directories(reactor-c PUBLIC ../include/core/utils)
target_include_directories(reactor-c PUBLIC federated/RTI/)

if(APPLE)
    SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

# Link with OpenSSL library
if(DEFINED FEDERATED_AUTHENTICATED)
    if(APPLE)
        set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    endif()

    find_package(OpenSSL REQUIRED)
    target_link_libraries(reactor-c PUBLIC OpenSSL::SSL)
endif()

if(DEFINED FEDERATED)
  find_library(MATH_LIBRARY m)
  if(MATH_LIBRARY)
    target_link_libraries(reactor-c PUBLIC ${MATH_LIBRARY})
  endif()
endif()

# Unless specified otherwise initial event queue and reaction queue to size 10
if(NOT DEFINED INITIAL_EVENT_QUEUE_SIZE)
    set(INITIAL_EVENT_QUEUE_SIZE 10)
endif()

if(NOT DEFINED INITIAL_REACT_QUEUE_SIZE)
    set(INITIAL_REACT_QUEUE_SIZE 10)
endif()

target_compile_definitions(reactor-c PRIVATE INITIAL_EVENT_QUEUE_SIZE=${INITIAL_EVENT_QUEUE_SIZE})
target_compile_definitions(reactor-c PRIVATE INITIAL_REACT_QUEUE_SIZE=${INITIAL_REACT_QUEUE_SIZE})
target_compile_definitions(reactor-c PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})

# If variable X is defined in cMake (set using SET()) or passed in as a command-line
# argument using -DX=<value>, then make it a compiler flag for reactor-c so that X
# is also defined in the C code for reactor-c.
macro(define X)
    if(DEFINED ${X})
        message(STATUS ${X}=${${X}})
        target_compile_definitions(reactor-c PUBLIC ${X}=${${X}})
    endif(DEFINED ${X})
endmacro()

# Search and apply all possible compile definitions
message(STATUS "Applying preprocessor definitions...")
define(_LF_CLOCK_SYNC_ATTENUATION)
define(_LF_CLOCK_SYNC_COLLECT_STATS)
define(_LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL)
define(LF_CLOCK_SYNC) # 1 for OFF, 2 for INIT and 3 for ON.
define(_LF_CLOCK_SYNC_PERIOD_NS)
define(_LF_FEDERATE_NAMES_COMMA_SEPARATED)
define(ADVANCE_MESSAGE_INTERVAL)
define(EXECUTABLE_PREAMBLE)
define(FEDERATED_CENTRALIZED)
define(FEDERATED_DECENTRALIZED)
define(FEDERATED)
define(FEDERATED_AUTHENTICATED)
define(FEDERATE_ID)
define(LF_REACTION_GRAPH_BREADTH)
define(LF_TRACE)
define(LF_SINGLE_THREADED)
define(LOG_LEVEL)
define(MODAL_REACTORS)
define(NUMBER_OF_FEDERATES)
define(NUMBER_OF_WORKERS)
define(NUMBER_OF_WATCHDOGS)
define(USER_THREADS)
define(SCHEDULER)
define(LF_SOURCE_DIRECTORY)
define(LF_SOURCE_GEN_DIRECTORY)
define(LF_PACKAGE_DIRECTORY)
define(LF_FILE_SEPARATOR)
define(WORKERS_NEEDED_FOR_FEDERATE)
define(LF_ENCLAVES)
