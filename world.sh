#!/bin/sh

make distclean
./configure CFLAGS="-Wall -W -g -ggdb -O0"
make world

