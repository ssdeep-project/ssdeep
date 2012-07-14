#!/bin/sh

make distclean
./configure CPPFLAGS="-Wall -W -g -ggdb -O0"
make

