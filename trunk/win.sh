#!/bin/sh

make distclean
./configure --host=i386-mingw32 CXXFLAGS="-Wall -W -g -ggdb -O0" CFLAGS="-Wall -W -g -ggdb -O0"
make
