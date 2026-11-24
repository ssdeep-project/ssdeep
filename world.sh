#!/bin/sh
set -eu

./win.sh
cmake --build build-win --target package package_source
