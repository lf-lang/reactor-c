add_library(lf-network-impl STATIC)
add_library(lf::network-impl ALIAS lf-network-impl)
target_link_libraries(lf-network-impl PUBLIC lf-logging-api)
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
    # $ apt-get install libssl-dev
    # find_package(OpenSSL REQUIRED)
    # target_link_libraries(lf-network-impl PUBLIC OpenSSL::SSL)
else()
    target_link_libraries(lf-network-impl PRIVATE lf::network-api)
endif()

include_directories(/usr/local/include)

target_sources(lf-network-impl PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/net_util.c
    ${CMAKE_CURRENT_LIST_DIR}/netdriver.c
    ${CMAKE_CURRENT_LIST_DIR}/socket_common.c
)
IF(COMM_TYPE MATCHES TCP)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_socket_support.c)
ELSEIF(COMM_TYPE MATCHES MQTT)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_mqtt_support.c)
ELSEIF(COMM_TYPE MATCHES SST)
    target_link_libraries(lf-network-impl PUBLIC SSTLIB)
    target_sources(lf-network-impl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lf_sst_support.c)
    # add_compile_definitions(OPENSSL_REQUIRED)
ENDIF()

target_compile_definitions(lf-network-impl PUBLIC COMM_TYPE_${COMM_TYPE})
