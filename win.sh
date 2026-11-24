#!/bin/sh
set -eu

cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw32.cmake \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-win --parallel 4
