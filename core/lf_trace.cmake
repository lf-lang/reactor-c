if(DEFINED LF_TRACE)
    include(${LF_ROOT}/trace/api/CMakeLists.txt)
    target_link_libraries(reactor-c PUBLIC lf::trace-api)
    # If LF_TRACE_PLUGIN is set, treat it as a CMake package name and try to locate it via find_package().
    # If that fails (or the target isn't available), do not link anything here and assume the user supplies a
    # cmake-include file to link against the trace plugin and its dependencies.
    # To set LF_TRACE_PLUGIN, the user specifies a "package" field under "trace-plugin".
    # Example: See Option 1 at https://github.com/lf-lang/lf-trace-xronos/blob/8cd2ee82d624f6cc6a2788b0378e25df33be7347/example/src/CPS.lf
    if(DEFINED LF_TRACE_PLUGIN AND NOT LF_TRACE_PLUGIN STREQUAL "")
        # Optional: allow users to point CMake at a custom installation prefix or config dir(s).
        # To set LF_TRACE_PLUGIN_PATHS, the user specifies a "path" field under "trace-plugin".
        # Example: See Option 2 at https://github.com/lf-lang/lf-trace-xronos/blob/8cd2ee82d624f6cc6a2788b0378e25df33be7347/example/src/CPS.lf
        if(DEFINED LF_TRACE_PLUGIN_PATHS AND NOT LF_TRACE_PLUGIN_PATHS STREQUAL "")
            message(STATUS "Trying to find package ${LF_TRACE_PLUGIN} in: ${LF_TRACE_PLUGIN_PATHS}")
            find_package(${LF_TRACE_PLUGIN} QUIET CONFIG
                NO_DEFAULT_PATH
                PATHS ${LF_TRACE_PLUGIN_PATHS}
                PATH_SUFFIXES lib/cmake/${LF_TRACE_PLUGIN} share/${LF_TRACE_PLUGIN}/cmake
            )
        else()
            message(STATUS "Trying to find package ${LF_TRACE_PLUGIN} in the default system path")
            find_package(${LF_TRACE_PLUGIN} QUIET CONFIG)
        endif()

        if(DEFINED LF_TRACE_PLUGIN_LIBRARY AND NOT LF_TRACE_PLUGIN_LIBRARY STREQUAL "" AND TARGET "${LF_TRACE_PLUGIN_LIBRARY}")
            # To set LF_TRACE_PLUGIN_LIBRARY, the user specifies a "library" field under "trace-plugin".
            # Example: See Option 1 & 2 at https://github.com/lf-lang/lf-trace-xronos/blob/8cd2ee82d624f6cc6a2788b0378e25df33be7347/example/src/CPS.lf
            message(STATUS "Package ${LF_TRACE_PLUGIN} found. Linking trace plugin target: ${LF_TRACE_PLUGIN_LIBRARY}")
            target_link_libraries(reactor-c PRIVATE "${LF_TRACE_PLUGIN_LIBRARY}")
        else()
            # Example: See Option 3 at https://github.com/lf-lang/lf-trace-xronos/blob/8cd2ee82d624f6cc6a2788b0378e25df33be7347/example/src/CPS.lf
            message(STATUS "Trace plugin package/target not found (or LF_TRACE_PLUGIN_LIBRARY not set). Expecting user cmake-include to link the plugin.")
        endif()
    else()
        # If not, use the default implementation.
        message(STATUS "Linking with default trace implementation")
        include(${LF_ROOT}/trace/impl/CMakeLists.txt)
        target_link_libraries(reactor-c PRIVATE lf::trace-impl)
    endif()
else()
    include(${LF_ROOT}/trace/api/types/CMakeLists.txt)
    target_link_libraries(reactor-c PUBLIC lf::trace-api-types)
endif()