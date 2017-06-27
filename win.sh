#!/bin/sh

make distclean
./configure --host=i686-w64-mingw32 CXXFLAGS="-Wall -W -g -ggdb -O0 -static" CFLAGS="-Wall -W -g -ggdb -O0 -static" LIBS="-Wl,-Bstatic -lstdc++ -lpthread -static-libgcc"
make
