#!/usr/bin/env bash
set +e

mkdir cmake-build-release
cat > cmake-build-release/spack.yaml <<'EOF'
spack:
  specs:
  - gmake
  - intel-oneapi-mpi
  - ninja
  - cmake
  - gcc@14
  - python@3
  - python-venv
  - valgrind
  - iperf3
  - linux-perf
  view: true
  concretizer:
    unify: true
EOF
spack env activate --dir cmake-build-release

cmake -S . -B cmake-build-release -G Ninja -DKAVAL_MACHINE=horeka -Wno-dev -DCMAKE_BUILD_TYPE=Release
source .venv/bin/activate

cd cmake-build-release
cmake --build .
BUILD_DIR=. python _deps/kaval-src/run-experiments.py --machine shared --sbatch-template _deps/kaval-src/sbatch-templates/generic_job_files.txt --command-template _deps/kaval-src/command-templates/command_template_shared_intelmpi_multithreaded.txt --search-dirs ../experiment/iti/weak
