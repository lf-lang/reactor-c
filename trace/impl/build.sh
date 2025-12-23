#!/bin/bash

set -euo pipefail

# This directory does not contain a standalone CMake project. `trace/impl/CMakeLists.txt`
# is meant to be included from the reactor-c top-level build where the trace API include
# paths (for "trace.h") are defined.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REACTOR_C_ROOT="${SCRIPT_DIR}/../.."
BUILD_DIR="${SCRIPT_DIR}/build"

cmake -S "${REACTOR_C_ROOT}" -B "${BUILD_DIR}" -DLOG_LEVEL=4 -DLF_TRACE=1
cmake --build "${BUILD_DIR}" --target lf-trace-impl
