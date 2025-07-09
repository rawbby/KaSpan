#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

export I_MPI_PIN=1           # enable automatic pinning
export I_MPI_PIN_DOMAIN=core # pin at the core granularity

NODES=4
THREADS_PER_NODE=4
ALPHA=30
BETA=200
GAMMA=10
THETA=0.01
RUNS=1
CODE="DB"

while [ $# -gt 0 ]; do
  if [ $# -eq 1 ]; then
    printf "Error: User passed uneven number of arguments but only argument pairs are expected!\n" >&2
    exit 1
  fi

  case "$1" in
    -np|-n|--nodes) NODES="$2"            ;;
    -t|--threads)   THREADS_PER_NODE="$2" ;;
    --alpha)        ALPHA="$2"            ;;
    --beta)         BETA="$2"             ;;
    --gamma)        GAMMA="$2"            ;;
    --theta)        THETA="$2"            ;;
    -r|--runs)      RUNS="$2"             ;;
    --code)         CODE="$2"             ;;
    *)              printf "Error: User passed an unrecognized argument pair '%s' '%s'\n" "$1" "$2" >&2; exit 1 ;;
  esac

  shift
  shift
done

# -np <np>                         | spawn <np> ranks
# --map-by core:PE=1               | 1 process per core
# --bind-to core                   | pin each rank to its core
# --report-bindings                | print binding info
# ./graph/${CODE}_fw_begin.bin    | <fw_beg_file>
# ./graph/${CODE}_fw_adjacent.bin | <fw_csr_file>
# ./graph/${CODE}_bw_begin.bin    | <bw_beg_file>
# ./graph/${CODE}_bw_adjacent.bin | <bw_csr_file>
# 1 30 200 10 0.01 1               | <thread_count> <alpha> <beta> <gamma> <theta> <run_times>

RESULT_DIR="./result/$(date +%Y%m%d_%H%M%S)_$(printf "%03d" $((RANDOM%1000)))"
mkdir -p "${RESULT_DIR}"
touch "${RESULT_DIR}/output"

exec 3>&1 4>&2
exec > "${RESULT_DIR}/output" 2>&1

mpirun                            \
  -np "$NODES"                    \
  ./bin/iSpan                     \
  ./graph/${CODE}_fw_begin.bin    \
  ./graph/${CODE}_fw_adjacent.bin \
  ./graph/${CODE}_bw_begin.bin    \
  ./graph/${CODE}_bw_adjacent.bin \
  "$THREADS_PER_NODE"  "$ALPHA" "$BETA" "$GAMMA" "$THETA" "$RUNS"

exec 1>&3 2>&4

grep 'total time' "${RESULT_DIR}/output" | awk -F', ' '{print $2}' > "${RESULT_DIR}/total_time_ms"

echo $NODES            > "${RESULT_DIR}/nodes"
echo $THREADS_PER_NODE > "${RESULT_DIR}/threads_per_node"
echo $ALPHA            > "${RESULT_DIR}/alpha"
echo $BETA             > "${RESULT_DIR}/beta"
echo $GAMMA            > "${RESULT_DIR}/gamma"
echo $THETA            > "${RESULT_DIR}/theta"
echo $RUNS             > "${RESULT_DIR}/runs"
echo $CODE             > "${RESULT_DIR}/code"
"$(dirname "$0")/hw_info.sh" "${RESULT_DIR}"

echo "Benchmark successfully run! (${RESULT_DIR})"

deactivate  # deactivate env.sh
