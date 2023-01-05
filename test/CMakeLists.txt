add_library(test-lib STATIC src_gen_stub.c rand_utils.c)
target_compile_definitions(test-lib PUBLIC LF_UNTHREADED=1)
