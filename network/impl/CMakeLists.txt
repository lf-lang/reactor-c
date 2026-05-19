set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${LF_ROOT}/core/lf_utils.cmake)

add_library(lf-network-impl STATIC ${LF_NETWORK_FILES})
add_library(lf::network-impl ALIAS lf-network-impl)
target_sources(lf-network-impl PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src/net_util.c
    ${CMAKE_CURRENT_LIST_DIR}/src/socket_common.c
)

if(COMM_TYPE MATCHES TCP)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/lf_socket_support.c)
elseif(COMM_TYPE MATCHES SST)
    find_package(OpenSSL REQUIRED)
    find_package(sst-lib QUIET)

    if(NOT sst-lib_FOUND)
        message(STATUS "sst-lib not found locally, falling back to FetchContent...")
        include(FetchContent)
        
        # Share the download cache across all LF builds to prevent re-downloading
        set(FETCHCONTENT_BASE_DIR "$ENV{HOME}/.lf/sst-cache")
        
        FetchContent_Declare(
            sst-lib
            GIT_REPOSITORY https://github.com/iotauth/sst-c-api.git
            GIT_TAG        master
        )
        FetchContent_MakeAvailable(sst-lib)

        # Create an ALIAS to match the find_package namespace
        if(TARGET sst-c-api AND NOT TARGET sst-lib::sst-c-api)
            add_library(sst-lib::sst-c-api ALIAS sst-c-api)
        endif()
    endif()

    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/lf_sst_support.c)
    target_link_libraries(lf-network-impl PUBLIC sst-lib::sst-c-api)
elseif(COMM_TYPE MATCHES TLS)
    find_package(OpenSSL REQUIRED)
    target_sources(lf-network-impl PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/lf_tls_support.c
    )
    target_link_libraries(lf-network-impl PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
    )
else()
    message(FATAL_ERROR "Your communication type is not supported! The C target supports TCP.")
endif()

# Link necessary libraries
target_link_libraries(lf-network-impl PUBLIC lf-logging-api)
target_link_libraries(lf-network-impl PUBLIC lf-low-level-platform-api)
target_link_libraries(lf-network-impl PUBLIC lf::tag-api)
target_link_libraries(lf-network-impl PRIVATE lf-network-api)

lf_enable_compiler_warnings(lf-network-impl)

target_compile_definitions(lf-network-impl PUBLIC COMM_TYPE_${COMM_TYPE})
