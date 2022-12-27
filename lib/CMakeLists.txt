add_library(lib schedule.c)
target_include_directories(lib PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(lib PUBLIC LF_UNTHREADED=1)
