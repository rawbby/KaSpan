#!/usr/bin/env bash
set +e

cmake -S . -B cmake-build-release -G Ninja -DKAVAL_MACHINE=horeka -Wno-dev -DCMAKE_BUILD_TYPE=Release
source .venv/bin/activate

cd cmake-build-release
cmake --build .

BUILD_DIR=.                                                                                               \
python _deps/kaval-src/run-experiments.py                                                                 \
  --machine shared                                                                                        \
  --sbatch-template _deps/kaval-src/sbatch-templates/generic_job_files.txt                                \
  --command-template _deps/kaval-src/command-templates/command_template_shared_intelmpi_multithreaded.txt \
  --suite-files ../experiment/local/weak/kaspan_weak.suite.yaml

BUILD_DIR=.                                                                                               \
python _deps/kaval-src/run-experiments.py                                                                 \
  --machine shared                                                                                        \
  --sbatch-template _deps/kaval-src/sbatch-templates/generic_job_files.txt                                \
  --command-template _deps/kaval-src/command-templates/command_template_shared_intelmpi_multithreaded.txt \
  --suite-files ../experiment/local/weak/ispan_weak.suite.yaml
