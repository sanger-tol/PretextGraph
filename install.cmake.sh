#!/bin/bash









git submodule update --init --recursive

cd libdeflate
make clean
make -j 4 || exit 1
cd ../


rm -rf build_cmake && mkdir build_cmake && cd build_cmake && 
cmake  -DCMAKE_BUILD_TYPE=Release -S.. -B. && cmake --build . --target all --config Release -j 4 # && make package_source && cd ..