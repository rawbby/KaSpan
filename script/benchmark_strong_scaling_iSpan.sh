#!/usr/bin/env bash
set -euo pipefail
set -x
trap 'echo "Error on or near line $LINENO. Exiting." >&2' ERR

spack env activate --create .

HOST=$(hostname)
CORES=$(nproc)   # detect logical cores
echo "Running on machine: $HOST"
echo "=== Strong scaling on $HOST ($CORES cores) ==="

#--- CONFIG ------------------------------------------
GRAPHS=(DB)                  # basename prefixes in ./graphs
NP=(4)                       # number of MPI ranks = cores
PARAMS="30 200 10 0.01 1"    # alpha beta gamma theta run_times
BIN=./bin/iSpan
RESULT_DIR=./results/strong/$HOST
#-----------------------------------------------------

export I_MPI_PIN=1           # enable automatic pinning
export I_MPI_PIN_DOMAIN=core # pin at the core granularity

mkdir -p "$RESULT_DIR"

for code in "${GRAPHS[@]}"; do
  for r in "${NP[@]}"; do

    out="$RESULT_DIR/${code}_np${r}.log"
    echo "--- $(date): $code @ ${r} ranks ---" | tee "$out"

    mpirun                                 \
        -np $r                             \
        "$BIN"                             \
        "./graphs/${code}_fw_begin.bin"    \
        "./graphs/${code}_fw_adjacent.bin" \
        "./graphs/${code}_bw_begin.bin"    \
        "./graphs/${code}_bw_adjacent.bin" \
        1 $PARAMS                          \
      &>> "$out"

  done
done
echo "Results in $RESULT_DIR"
