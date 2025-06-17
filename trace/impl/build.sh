#!/bin/bash

cmake -S . -B build -DLOG_LEVEL=4
cmake --build build
