set(CORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_SYSTEM_VERSION 10.0)
    message("Using Windows SDK version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Nrf52")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_NRF52)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_ZEPHYR)
    set(PLATFORM_ZEPHYR true)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Rp2040")
    list(APPEND REACTORC_COMPILE_DEFS PLATFORM_RP2040)
endif()

# Get the general common sources for reactor-c
list(APPEND GENERAL_SOURCES tag.c clock.c port.c mixed_radix.c reactor_common.c lf_token.c environment.c)

# Add tracing support if requested
if (DEFINED LF_TRACE)
    message(STATUS "Including sources specific to tracing.")
    list(APPEND GENERAL_SOURCES tracepoint.c)
endif()

# Add the general sources to the list of REACTORC_SOURCES
list(APPEND REACTORC_SOURCES ${GENERAL_SOURCES})

# Add sources for either threaded or single-threaded runtime
if (DEFINED FEDERATED)
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
include(modal_models/CMakeLists.txt)

# Print sources used for compilation
list(JOIN REACTORC_SOURCES ", " PRINTABLE_SOURCE_LIST)
message(STATUS "Including the following sources: " ${PRINTABLE_SOURCE_LIST})

# Create the reactor-c library. If we are targeting Zephyr we have to use the
# Zephyr Cmake extension to create the library and add the sources.
if(PLATFORM_ZEPHYR)
    message("--- Building Zephyr library")
    zephyr_library_named(reactor-c)
    zephyr_library_sources(${REACTORC_SOURCES})
    zephyr_library_link_libraries(kernel)
else()
    add_library(reactor-c)
    target_sources(reactor-c PRIVATE ${REACTORC_SOURCES})
endif()

if (DEFINED LF_TRACE)
    include(${LF_ROOT}/trace/api/CMakeLists.txt)
    if(NOT LF_TRACE_PLUGIN)
        set(LF_TRACE_PLUGIN lf::trace-impl)
        include(${LF_ROOT}/trace/impl/CMakeLists.txt)
    endif()
    message(STATUS "linking trace plugin library ${LF_TRACE_PLUGIN}")
    target_link_libraries(reactor-c PUBLIC lf::trace-api)
    target_link_libraries(reactor-c PRIVATE "${LF_TRACE_PLUGIN}")
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

# Apply compile definitions to the reactor-c library.
target_compile_definitions(reactor-c PUBLIC ${REACTORC_COMPILE_DEFS})

target_include_directories(reactor-c PUBLIC ../include)
target_include_directories(reactor-c PUBLIC ../include/core)
target_include_directories(reactor-c PUBLIC ../include/core/federated)
target_include_directories(reactor-c PUBLIC ../include/core/federated/network)
target_include_directories(reactor-c PUBLIC ../include/core/platform)
target_include_directories(reactor-c PUBLIC ../include/core/modal_models)
target_include_directories(reactor-c PUBLIC ../include/core/threaded)
target_include_directories(reactor-c PUBLIC ../include/core/utils)
target_include_directories(reactor-c PUBLIC federated/RTI/)

if (APPLE)
    SET(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

# Link with OpenSSL library
if(DEFINED FEDERATED_AUTHENTICATED)
    if (APPLE)
        set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    endif()
    find_package(OpenSSL REQUIRED)
    target_link_libraries(reactor-c PUBLIC OpenSSL::SSL)
endif()

if(DEFINED _LF_CLOCK_SYNC_ON)
  find_library(MATH_LIBRARY m)
  if(MATH_LIBRARY)
    target_link_libraries(reactor-c PUBLIC ${MATH_LIBRARY})
  endif()
endif()

# Link with thread library, unless we are on the Zephyr platform.
if(NOT DEFINED LF_SINGLE_THREADED OR DEFINED LF_TRACE)
    if (NOT PLATFORM_ZEPHYR)
        find_package(Threads REQUIRED)
        target_link_libraries(reactor-c PUBLIC Threads::Threads)
    endif()
endif()

# Macro for translating a command-line argument into compile definition for
# reactor-c lib
macro(define X)
    if(DEFINED ${X})
        message(STATUS ${X}=${${X}})
        target_compile_definitions(reactor-c PUBLIC ${X}=${${X}})
    endif(DEFINED ${X})
endmacro()

# FIXME: May want these to be application dependent, hence passed as
# parameters to Cmake.
target_compile_definitions(reactor-c PRIVATE INITIAL_EVENT_QUEUE_SIZE=10)
target_compile_definitions(reactor-c PRIVATE INITIAL_REACT_QUEUE_SIZE=10)
target_compile_definitions(reactor-c PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})

# Search and apply all possible compile definitions
message(STATUS "Applying preprocessor definitions...")
define(_LF_CLOCK_SYNC_ATTENUATION)
define(_LF_CLOCK_SYNC_COLLECT_STATS)
define(_LF_CLOCK_SYNC_EXCHANGES_PER_INTERVAL)
define(_LF_CLOCK_SYNC_INITIAL)
define(_LF_CLOCK_SYNC_ON)
define(_LF_CLOCK_SYNC_PERIOD_NS)
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
