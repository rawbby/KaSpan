#!/bin/bash

set -euo pipefail

export SETVARS_ARGS="--force" && source /opt/intel/oneapi/setvars.sh >/dev/null 2>&1 || true

mkdir -p ~/workspace/KaSpan/cmake-build-valgrind
cd ~/workspace/KaSpan/cmake-build-valgrind

cmake -S .. -B . -G Ninja -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Debug -DKASPAN_VALGRIND=ON
cmake --build . -j 7
