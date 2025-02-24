

#!/bin/bash


rm -rf builddir

if [ `uname` == Darwin ]; then
    meson setup --buildtype=release --unity on builddir
else
    env CC=clang CXX=clang++ meson setup --buildtype=release --unity on builddir
fi
cd builddir
meson compile
meson test

