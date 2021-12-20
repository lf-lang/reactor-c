set(SOURCES ctarget.c)

add_library(lib ctarget.c)
target_include_directories(lib PRIVATE ${PROJECT_SOURCE_DIR}/include)
