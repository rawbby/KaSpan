#!/usr/bin/env bash
set +e

cmake -S . -B cmake-build-release -G Ninja -DKAVAL_MACHINE=horeka -Wno-dev -DCMAKE_BUILD_TYPE=Release
source .venv/bin/activate

cd cmake-build-release
cmake --build .

module unload compiler/intel/2025.1_llvm
module unload devel/cuda/12.4
module unload mpi/openmpi/5.0

module load devel/cmake/3.30
module load compiler/gnu/14
module load devel/gdb/13.1
module load mpi/impi/2021.11

BUILD_DIR=.                                                                \
MACHINE=horeka                                                             \
python _deps/kaval-src/run-experiments.py                                  \
  --machine horeka                                                         \
  --sbatch-template _deps/kaval-src/sbatch-templates/horeka.txt            \
  --command-template _deps/kaval-src/command-templates/horeka-IntelMPI.txt \
  --suite-files ../experiment/horeka/strong/ispan_strong.suite.yaml

BUILD_DIR=.                                                                \
MACHINE=horeka                                                             \
python _deps/kaval-src/run-experiments.py                                  \
  --machine horeka                                                         \
  --sbatch-template _deps/kaval-src/sbatch-templates/horeka.txt            \
  --command-template _deps/kaval-src/command-templates/horeka-IntelMPI.txt \
  --suite-files ../experiment/horeka/weak/ispan_weak.suite.yaml
