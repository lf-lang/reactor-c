if(DEFINED LF_TRACE)
    include(${LF_ROOT}/trace/api/CMakeLists.txt)
    target_link_libraries(reactor-c PUBLIC lf::trace-api)
    # If LF_TRACE_PLUGIN is set, treat it as a CMake package name and try to locate it via find_package().
    # If that fails (or the target isn't available), do not link anything here and assume the user supplies a
    # cmake-include file to link against the trace plugin and its dependencies.
    if(DEFINED LF_TRACE_PLUGIN AND NOT LF_TRACE_PLUGIN STREQUAL "")
        if(DEFINED LF_TRACE_PLUGIN_PATHS AND NOT LF_TRACE_PLUGIN_PATHS STREQUAL "")
            # Case A: LF_TRACE_PLUGIN is set and LF_TRACE_PLUGIN_PATHS is set,
            # when the user specifies a "package" field, a "library" field, and a "path" field under "trace-plugin".
            # Example: See https://github.com/lf-lang/lf-trace-xronos/blob/53e77a6b072f6b25d4fdfd53a4a3700fc199f938/tests/src/TracePluginUserPath.lf
            message(STATUS "Trying to find package ${LF_TRACE_PLUGIN} in: ${LF_TRACE_PLUGIN_PATHS}")
            find_package(${LF_TRACE_PLUGIN} QUIET CONFIG
                NO_DEFAULT_PATH
                PATHS ${LF_TRACE_PLUGIN_PATHS}
                PATH_SUFFIXES lib/cmake/${LF_TRACE_PLUGIN} share/${LF_TRACE_PLUGIN}/cmake
            )
        else()
            # Case B: LF_TRACE_PLUGIN is set but LF_TRACE_PLUGIN_PATHS is not set,
            # when the user specifies a "package" field and a "library" field under "trace-plugin".
            # Example: See https://github.com/lf-lang/lf-trace-xronos/blob/53e77a6b072f6b25d4fdfd53a4a3700fc199f938/tests/src/TracePluginSystemPath.lf
            message(STATUS "Trying to find package ${LF_TRACE_PLUGIN} in the default system path")
            find_package(${LF_TRACE_PLUGIN} QUIET CONFIG)
        endif()

        if(DEFINED LF_TRACE_PLUGIN_LIBRARY AND NOT LF_TRACE_PLUGIN_LIBRARY STREQUAL "" AND TARGET "${LF_TRACE_PLUGIN_LIBRARY}")
            # In case A & B, the "library" field determines what gets linked.
            message(STATUS "Package ${LF_TRACE_PLUGIN} found. Linking trace plugin target: ${LF_TRACE_PLUGIN_LIBRARY}")
            target_link_libraries(reactor-c PRIVATE "${LF_TRACE_PLUGIN_LIBRARY}")
        else()
            # Case C: Neither LF_TRACE_PLUGIN_LIBRARY nor LF_TRACE_PLUGIN_PATHS is set. LF_TRACE_PLUGIN is set via cmake-args.
            # This case happens when the user does not use "trace-plugin" but proceeds with a custom integration via cmake-include and cmake-args.
            # Example: See https://github.com/lf-lang/lf-trace-xronos/blob/53e77a6b072f6b25d4fdfd53a4a3700fc199f938/tests/src/TracePluginCustomCmake.lf
            message(STATUS "Trace plugin package or library not found. Expecting user cmake-include to link the plugin.")
        endif()
    else()
        # If LF_TRACE_PLUGIN not set, use the default trace plugin implementation.
        message(STATUS "Linking with default trace implementation")
        include(${LF_ROOT}/trace/impl/CMakeLists.txt)
        target_link_libraries(reactor-c PRIVATE lf::trace-impl)
    endif()
else()
    include(${LF_ROOT}/trace/api/types/CMakeLists.txt)
    target_link_libraries(reactor-c PUBLIC lf::trace-api-types)
endif()