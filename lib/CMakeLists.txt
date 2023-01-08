add_library(lib schedule.c)
target_include_directories(lib PRIVATE ${PROJECT_SOURCE_DIR}/include)
# FIXME: What is this? Why is it public?
target_compile_definitions(lib PUBLIC LF_UNTHREADED=1)
