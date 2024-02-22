add_library(test-lib STATIC src_gen_stub.c rand_utils.c)
# include(${CMAKE_CURRENT_LIST_DIR}/../low_level_platform/api/CMakeLists.txt)
target_link_libraries(test-lib PRIVATE lf-low-level-platform-api)
