set(SOURCES schedule.c tag.c time.c)

add_library(lib schedule.c tag.c time.c)
target_include_directories(lib PRIVATE ${PROJECT_SOURCE_DIR}/include)
