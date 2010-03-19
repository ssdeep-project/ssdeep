#!/bin/sh

make distclean
./configure --host=i386-mingw32 CFLAGS="-Wall -W -O2"
make
