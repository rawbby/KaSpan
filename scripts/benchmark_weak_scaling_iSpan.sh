#!/usr/bin/env bash
set -euo pipefail

HOST=$(hostname)
CORES=$(nproc)   # detect logical cores
echo "Running on machine: $HOST"
echo "=== Weak scaling on $HOST ($CORES cores) ==="

#--- CONFIG ----------------------------------------------------
# Prepare ./graphs/${code}_wk${r}/… for weak-scaling sets
GRAPHS=(DB)                     # or add LJ, TW…
NP=(1 2 4 8 16 32)              # # ranks
PARAMS="30 200 10 0.01 1"
BIN=./bin/iSpan
RESULT_DIR=./results/weak/$HOST
#----------------------------------------------------------------

export I_MPI_PIN=1           # enable automatic pinning
export I_MPI_PIN_DOMAIN=core # pin at the core granularity

mkdir -p "$RESULT_DIR"

for code in "${GRAPHS[@]}"; do
  for r in "${NP[@]}"; do
    dir="./graphs/${code}_wk${r}"
    out="$RESULT_DIR/${code}_np${r}.log"
    echo "--- $(date): $code wk @ ${r} ranks ---" | tee "$out"
    mpirun                             \
        -np $r                         \
        "$BIN"                         \
        "$dir/${code}_fw_begin.bin"    \
        "$dir/${code}_fw_adjacent.bin" \
        "$dir/${code}_bw_begin.bin"    \
        "$dir/${code}_bw_adjacent.bin" \
        1 $PARAMS                      \
      &>> "$out"
  done
done
echo "Results in $RESULT_DIR"
