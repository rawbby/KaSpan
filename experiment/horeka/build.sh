#!/bin/bash

set -euo pipefail

source ~/workspace/KaSpan/experiment/horeka/env.sh

mkdir ~/workspace/KaSpan/cmake-build-release
cd ~/workspace/KaSpan/cmake-build-release

cmake -S .. -B . -G Ninja -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . -j 16
