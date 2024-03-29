# set(LF_NETWORK_FILES net_util.c lf_socket_support.c socket_common.c)

# list(TRANSFORM LF_NETWORK_FILES PREPEND federated/network/)
# list(APPEND REACTORC_SOURCES ${LF_NETWORK_FILES})


add_library(lf-network-impl STATIC)
add_library(lf::network-impl ALIAS lf-network-impl)

target_link_libraries(lf-network-impl PRIVATE lf::network-api)
target_link_libraries(lf-network-impl PUBLIC lf-logging-api)
target_compile_definitions(lf-network-impl PUBLIC COMM_TYPE_${COMM_TYPE})
target_compile_definitions(lf-network-impl PUBLIC FEDERATED=1)

target_sources(lf-network-impl PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/net_util.c
    ${CMAKE_CURRENT_LIST_DIR}/socket_common.c
)

IF(COMM_TYPE MATCHES TCP)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_socket_support.c)
ELSEIF(COMM_TYPE MATCHES MQTT)
    # $ git clone https://github.com/eclipse/paho.mqtt.c.git
    # $ cd paho.mqtt.c.git
    # $ mkdir build.paho; cd build.paho
    # $ cmake -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=TRUE \
    #     -DPAHO_BUILD_SAMPLES=TRUE ../
    # $ make
    # $ sudo make install
    # It will be installed in /usr/local/lib
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_mqtt_support.c)
    find_package(eclipse-paho-mqtt-c REQUIRED)
    target_link_libraries(lf-network-impl eclipse-paho-mqtt-c::paho-mqtt3as)
ELSEIF(COMM_TYPE MATCHES SST)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_sst_support.c)
ENDIF()


