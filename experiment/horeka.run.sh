#!/bin/bash

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

cmake -S . -B cmake-build-release -G Ninja -Wno-dev -DCMAKE_BUILD_TYPE=Release
source .venv/bin/activate

cd cmake-build-release
cmake --build .

export NODES=2
export NTASKS=32
export NTASKS_PER_NODE=16
export OUT_FILE="job.%j.out"
export ERR_FILE="job.%j.err"
export JOB_NAME="myjob"
export PARTITION="debug"
export TIME="02:00:00"
export MEM="230gb"

# command to run (quote if contains spaces/specials)
export CMD="mpiexec.hydra -n ${NTASKS} ./my_program arg1 arg2"

TMP_SCRIPT=$(mktemp --suffix=.sh horeka.job.XXXXXX)
envsubst < horeka.job.sh > "${TMP_SCRIPT}"

echo "${TMP_SCRIPT}"
cat "${TMP_SCRIPT}"
