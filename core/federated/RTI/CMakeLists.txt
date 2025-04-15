cmake_minimum_required(VERSION 3.12)
project(RTI VERSION 0.2.0 LANGUAGES C)

set(CoreLib ../../../core)
set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(IncludeDir ../../../include/core)
set(RTI_LIB rti_lib)
set(RTI_MAIN RTI)

# Add common RTI functionality to a static library. This is done to simplify
# the building of unit tests.
add_library(${RTI_LIB} STATIC
    rti_common.c
    rti_remote.c
    ${CoreLib}/tracepoint.c
    ${CoreLib}/utils/util.c
    ${CoreLib}/tag.c
    ${CoreLib}/clock.c
    ${CoreLib}/federated/network/net_util.c
    ${CoreLib}/federated/network/socket_common.c
    ${CoreLib}/utils/pqueue_base.c
    ${CoreLib}/utils/pqueue_tag.c
    ${CoreLib}/utils/pqueue.c
)

# Add the main target which will link with the library.
add_executable(${RTI_MAIN} main.c)

target_include_directories(${RTI_LIB} PUBLIC ../../../include)
target_include_directories(${RTI_LIB} PUBLIC ${IncludeDir})
target_include_directories(${RTI_LIB} PUBLIC ${IncludeDir}/federated)
target_include_directories(${RTI_LIB} PUBLIC ${IncludeDir}/federated/network)
target_include_directories(${RTI_LIB} PUBLIC ${IncludeDir}/modal_models)
target_include_directories(${RTI_LIB} PUBLIC ${IncludeDir}/utils)

if (NOT DEFINED LOG_LEVEL)
    set(LOG_LEVEL 2)
ENDIF(NOT DEFINED LOG_LEVEL)

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    # Set the LOG_LEVEL to 4 to get DEBUG messages
    message("-- Building RTI with DEBUG messages enabled")
    set(LOG_LEVEL 4)
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)
target_compile_definitions(${RTI_LIB} PUBLIC LOG_LEVEL=${LOG_LEVEL})

include(${LF_ROOT}/version/api/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC lf::version-api)

include(${LF_ROOT}/logging/api/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC lf::logging-api)

include(${LF_ROOT}/tag/api/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC lf::tag-api)

include(${LF_ROOT}/platform/api/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC lf::platform-api)

include(${LF_ROOT}/platform/impl/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC lf::platform-impl)

include(${LF_ROOT}/trace/api/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC lf::trace-api)

include(${LF_ROOT}/trace/impl/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC  lf::trace-impl)

include(${LF_ROOT}/low_level_platform/impl/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC  lf::low-level-platform-impl)

include(${LF_ROOT}/low_level_platform/api/CMakeLists.txt)
target_link_libraries(${RTI_LIB} PUBLIC  lf::low-level-platform-api)

# Set the STANDALONE_RTI flag to include the rti_remote and rti_common.
target_compile_definitions(${RTI_LIB} PUBLIC STANDALONE_RTI=1)

# Set FEDERATED to get federated compilation support
target_compile_definitions(${RTI_LIB} PUBLIC FEDERATED=1)
target_compile_definitions(${RTI_LIB} PUBLIC PLATFORM_${CMAKE_SYSTEM_NAME})

# Set RTI Tracing
target_compile_definitions(${RTI_LIB} PUBLIC RTI_TRACE)

# Warnings as errors
target_compile_options(${RTI_LIB} PUBLIC -Werror)

# Find threads and link to it
find_package(Threads REQUIRED)
target_link_libraries(${RTI_LIB} PUBLIC Threads::Threads)

# If AUTH is defined, then the RTI will be able to authenticate federates. By default, this is off.
if (NOT DEFINED AUTH)
    set(AUTH OFF)
ENDIF()

IF(AUTH MATCHES ON)
  target_compile_definitions(${RTI_LIB} PUBLIC __RTI_AUTH__)
  # Find OpenSSL and link to it
  find_package(OpenSSL REQUIRED)
  target_link_libraries(${RTI_LIB} PUBLIC OpenSSL::SSL)
ENDIF(AUTH MATCHES ON)

# Link the main target with the library.
target_link_libraries(${RTI_MAIN} PRIVATE ${RTI_LIB})

# Add semantic versioning
# Get the version from the project definition
set(RTI_VERSION "${PROJECT_VERSION}")

# Get the current commit hash using git
find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE RTI_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the current build date
string(TIMESTAMP RTI_BUILD_DATE "%Y-%m-%d")

# Add compile definitions for versioning
target_compile_definitions(${RTI_LIB} PUBLIC 
    RTI_VERSION="${RTI_VERSION}"
    RTI_COMMIT="${RTI_COMMIT}"
    RTI_BUILD_DATE="${RTI_BUILD_DATE}"
)


install(
    TARGETS RTI
    DESTINATION bin
)

# Build unit tests
enable_testing()
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_SRCS
    ${TEST_DIR}/rti_common_test.c
)
foreach(TEST_SRC ${TEST_SRCS})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    target_link_libraries(${TEST_NAME} PUBLIC ${RTI_LIB})
    target_include_directories(${TEST_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
