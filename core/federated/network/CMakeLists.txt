set(LF_NETWORK_FILES net_util.c lf_socket_support.c)

list(TRANSFORM LF_NETWORK_FILES PREPEND federated/network/)
list(APPEND REACTORC_SOURCES ${LF_NETWORK_FILES})
