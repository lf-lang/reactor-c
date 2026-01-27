#!/bin/bash

set -euo pipefail

# `trace/impl` is a standalone CMake project (and can also be included from the
# reactor-c top-level build). This script builds the standalone project and
# produces `lib/lf-trace-impl.a`.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DLOG_LEVEL=4
cmake --build "${BUILD_DIR}" --target lf-trace-impl
