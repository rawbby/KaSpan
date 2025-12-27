#!/bin/bash

set -euo pipefail

source /opt/intel/oneapi/setvars.sh

mkdir ~/workspace/KaSpan/cmake-build-valgrind
cd ~/workspace/KaSpan/cmake-build-valgrind

cmake -S .. -B . -G Ninja -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Debug -DKASPAN_VALGRIND=ON
cmake --build . -j 7
