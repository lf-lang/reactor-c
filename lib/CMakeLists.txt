set(SOURCES ctarget_schedule.c)

add_library(lib ctarget_schedule.c)
target_include_directories(lib PRIVATE ${PROJECT_SOURCE_DIR}/include)
