#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

code="DB"

export I_MPI_PIN=1           # enable automatic pinning
export I_MPI_PIN_DOMAIN=core # pin at the core granularity

# -np <np>                         | spawn <np> ranks
# --map-by core:PE=1               | 1 process per core
# --bind-to core                   | pin each rank to its core
# --report-bindings                | print binding info
# ./graphs/${code}_fw_begin.bin    | <fw_beg_file>
# ./graphs/${code}_fw_adjacent.bin | <fw_csr_file>
# ./graphs/${code}_bw_begin.bin    | <bw_beg_file>
# ./graphs/${code}_bw_adjacent.bin | <bw_csr_file>
# 1 30 200 10 0.01 1               | <thread_count> <alpha> <beta> <gamma> <theta> <run_times>

mpirun                             \
  -envall                          \
  -np 4                            \
                                   \
  valgrind                         \
    --tool=memcheck                \
    --leak-check=full              \
    --show-leak-kinds=all          \
    --track-origins=yes            \
    --error-exitcode=1             \
    --log-file=memcheck.%p.log     \
                                   \
  ./bin/iSpan                      \
  ./graph/${code}_fw_begin.bin     \
  ./graph/${code}_fw_adjacent.bin  \
  ./graph/${code}_bw_begin.bin     \
  ./graph/${code}_bw_adjacent.bin  \
  1 30 200 10 0.01 1

deactivate  # deactivate env.sh
