#!/bin/sh

make distclean
./configure --host=i386-mingw32 CPPFLAGS="-Wall -W -O2"
make
