add_subdirectory(platform)
add_subdirectory(threaded)
add_subdirectory(utils)

# Determine which sources to include.
set(UNTHREADED_SOURCES reactor.c)
if(NUMBER_OF_WORKERS)
    message(STATUS "Including sources for threaded runtime with ${NUMBER_OF_WORKERS} worker(s).")
    add_compile_definitions(NUMBER_OF_WORKERS=${NUMBER_OF_WORKERS})
else()
    message(STATUS "Including sources for unthreaded runtime.")
    remove_definitions(-DNUMBER_OF_WORKERS)
    list(APPEND SOURCES ${UNTHREADED_SOURCES})
endif(NUMBER_OF_WORKERS)

# Create the library "core".
add_library(core ${SOURCES})

# Link threading library if necessary.
if(NUMBER_OF_WORKERS)
    find_package(Threads REQUIRED)
    target_link_libraries(${CoreLib} Threads::Threads)
endif(NUMBER_OF_WORKERS)
