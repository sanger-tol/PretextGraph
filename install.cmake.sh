#!/bin/bash

git submodule update --init --recursive

cd libdeflate
make -j 4 || exit 1
cd ../


cmake_bin=cmake
ctest_bin=ctest

rm -rf build_cmake && mkdir build_cmake && cd build_cmake && ${cmake_bin}  -DCMAKE_BUILD_TYPE=Release -S.. -B. && ${cmake_bin}  --build . --target all --config Release -j 4 # && ${ctest_bin} -j4 --verbose

Echo "Done"