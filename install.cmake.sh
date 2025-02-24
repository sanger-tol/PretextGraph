#!/bin/bash

rm -rf build_cmake && mkdir build_cmake && cd build_cmake && cmake  -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
cd ../