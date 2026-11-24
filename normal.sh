#!/bin/sh
set -eu

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
