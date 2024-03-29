add_library(lf-network-impl STATIC)
add_library(lf::network-impl ALIAS lf-network-impl)
if(COMM_TYPE MATCHES MQTT)
    # $ git clone https://github.com/eclipse/paho.mqtt.c.git
    # $ cd paho.mqtt.c.git
    # $ mkdir build.paho; cd build.paho
    # $ cmake ../
    # $ make
    # $ sudo make install
    # It will be installed in /usr/local/lib
    find_package(eclipse-paho-mqtt-c REQUIRED)
    target_link_libraries(lf-network-impl PRIVATE lf::network-api eclipse-paho-mqtt-c::paho-mqtt3a eclipse-paho-mqtt-c::paho-mqtt3c)
    target_link_libraries(lf-network-impl PUBLIC lf-logging-api)
else()
    target_link_libraries(lf-network-impl PRIVATE lf::network-api)
    target_link_libraries(lf-network-impl PUBLIC lf-logging-api)
endif()

target_compile_definitions(lf-network-impl PUBLIC COMM_TYPE_${COMM_TYPE})
target_compile_definitions(lf-network-impl PUBLIC FEDERATED=1)

target_sources(lf-network-impl PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/net_util.c
)

IF(COMM_TYPE MATCHES TCP)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_socket_support.c ${CMAKE_CURRENT_LIST_DIR}/socket_common.c)
ELSEIF(COMM_TYPE MATCHES MQTT)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_mqtt_support.c)
ELSEIF(COMM_TYPE MATCHES SST)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_sst_support.c)
ENDIF()
