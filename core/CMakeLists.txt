add_subdirectory(platform)
add_subdirectory(utils)

add_library(core reactor.c)
target_link_libraries(core utils)
