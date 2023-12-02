set(LF_NETWORK_FILES net_util.c lf_socket_support.c)
list(APPEND INFO_SOURCES ${LF_NETWORK_FILES})

list(TRANSFORM LF_NETWORK_FILES PREPEND federated/network/)
target_sources(core PRIVATE ${LF_NETWORK_FILES})
