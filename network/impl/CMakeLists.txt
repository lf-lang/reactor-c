set(LF_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${LF_ROOT}/core/lf_utils.cmake)

add_library(lf-network-impl STATIC)
add_library(lf::network-impl ALIAS lf-network-impl)

# Link necessary libraries.
target_link_libraries(lf-network-impl PUBLIC lf-logging-api)
target_link_libraries(lf-network-impl PRIVATE lf::network-api)
lf_enable_compiler_warnings(lf-network-impl)

target_sources(lf-network-impl PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/net_util.c
    ${CMAKE_CURRENT_LIST_DIR}/netdriver.c
    ${CMAKE_CURRENT_LIST_DIR}/socket_common.c
)

if(COMM_TYPE MATCHES TCP)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/lf_socket_support.c)
# elseif(COMM_TYPE MATCHES MQTT)
    # $ git clone https://github.com/eclipse/paho.mqtt.c.git
    # $ cd paho.mqtt.c.git
    # $ mkdir build.paho; cd build.paho
    # $ cmake ../
    # $ make
    # $ sudo make install
    # It will be installed in /usr/local/lib
    find_package(eclipse-paho-mqtt-c REQUIRED)
    target_link_libraries(lf-network-impl PRIVATE eclipse-paho-mqtt-c::paho-mqtt3a eclipse-paho-mqtt-c::paho-mqtt3c)
    # $ apt-get install libssl-dev
    # find_package(OpenSSL REQUIRED)

    # target_link_libraries(lf-network-impl PUBLIC OpenSSL::SSL)
#     target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/lf_mqtt_support.c)
# elseif(COMM_TYPE MATCHES SST)
#     target_link_libraries(lf-network-impl PUBLIC SSTLIB)
#     target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/lf_sst_support.c)
#     # add_compile_definitions(OPENSSL_REQUIRED)
else()
    message(FATAL_ERROR "Your communication type is not supported! The C target supports TCP, MQTT and SST.")
endif()

target_compile_definitions(lf-network-impl PUBLIC COMM_TYPE_${COMM_TYPE})



##########################


# if(COMM_TYPE MATCHES MQTT)
#     # $ git clone https://github.com/eclipse/paho.mqtt.c.git
#     # $ cd paho.mqtt.c.git
#     # $ mkdir build.paho; cd build.paho
#     # $ cmake ../
#     # $ make
#     # $ sudo make install
#     # It will be installed in /usr/local/lib
#     find_package(eclipse-paho-mqtt-c REQUIRED)
#     target_link_libraries(lf-network-impl PRIVATE lf::network-api eclipse-paho-mqtt-c::paho-mqtt3a eclipse-paho-mqtt-c::paho-mqtt3c)
#     # $ apt-get install libssl-dev
#     # find_package(OpenSSL REQUIRED)
#     # target_link_libraries(lf-network-impl PUBLIC OpenSSL::SSL)
# else()
#     target_link_libraries(lf-network-impl PRIVATE lf::network-api)
# endif()

# include_directories(/usr/local/include)

# target_sources(lf-network-impl PUBLIC
#     ${CMAKE_CURRENT_LIST_DIR}/net_util.c
#     ${CMAKE_CURRENT_LIST_DIR}/netdriver.c
#     ${CMAKE_CURRENT_LIST_DIR}/socket_common.c
# )
# # IF(COMM_TYPE MATCHES TCP)
# #     target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_socket_support.c)
# # ELSEIF(COMM_TYPE MATCHES MQTT)
# #     target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_mqtt_support.c)
# # ELSEIF(COMM_TYPE MATCHES SST)
# #     target_link_libraries(lf-network-impl PUBLIC SSTLIB)
# #     target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_sst_support.c)
# #     # add_compile_definitions(OPENSSL_REQUIRED)
# # ENDIF()

# target_compile_definitions(lf-network-impl PUBLIC COMM_TYPE_${COMM_TYPE})
