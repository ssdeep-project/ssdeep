#!/bin/sh

make distclean
./configure CXXFLAGS="-Wall -W -g -ggdb -O0" CFLAGS="-Wall -W -g -ggdb -O0"
make -j4

