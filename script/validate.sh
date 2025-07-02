#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

run_networkx() {
  mkdir -p "$1"
  cd "$1"

  python                             \
    "${SOLVER_DIR}/networkx/main.py" \
    "${GRAPH_DIR}/TOY_edgelist.txt"  \
    "TOY_assignment.txt"
}

run_ispan() {
  mkdir -p "$1"
  cd "$1"

  export I_MPI_PIN=1
  export I_MPI_PIN_DOMAIN=core

  mpirun                                   \
    -np 4                                  \
    "${BINARY_DIR}/iSpan"                  \
    "${GRAPH_DIR}/TOY_fw_begin.bin"    \
    "${GRAPH_DIR}/TOY_fw_adjacent.bin" \
    "${GRAPH_DIR}/TOY_bw_begin.bin"    \
    "${GRAPH_DIR}/TOY_bw_adjacent.bin" \
    1 30 200 10 0.01 1
}

THIS_RESULT_DIR="${RESULT_DIR}/$(date +%Y%m%d_%H%M%S)_$(printf "%03d" $((RANDOM%1000)))"
run_networkx "${THIS_RESULT_DIR}/networkx"
run_ispan "${THIS_RESULT_DIR}/iSpan"

deactivate
