add_library(test-lib STATIC src_gen_stub.c rand_utils.c)
target_link_libraries(test-lib PRIVATE lf::low-level-platform-api)
