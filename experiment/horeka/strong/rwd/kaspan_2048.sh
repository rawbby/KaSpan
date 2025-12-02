#!/bin/bash
#SBATCH --nodes=27
#SBATCH --ntasks=2052
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_2048.out
#SBATCH -e kaspan_2048.err
#SBATCH -J kaspan_2048
#SBATCH --partition=cpuonly
#SBATCH --time=25:00
#SBATCH --export=ALL

set -euo pipefail

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan

rwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )

export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=3

set +eu

for manifest in "${rwd[@]}"; do
  manifest_name="$(basename "${manifest%.manifest}")"
  output_file="${app_name}_${manifest_name}_np2048.json"
  if [[ -s "$output_file" ]]; then
    echo "[SKIPPING] ${app_name} NP=2048 Graph=${manifest_name}"
  else
    echo "[STARTING] ${app_name} NP=2048 Graph=${manifest_name}"
    srun                   \
      --time=3:00          \
      --oom-kill-step=1    \
      --mpi=pmi2           \
      --nodes=27           \
      --exclusive          \
      --ntasks=2048        \
      --cpus-per-task=1    \
      --hint=nomultithread \
      --cpu-bind=cores     \
      "$app"               \
        --output_file "$output_file" \
        --manifest_file "$manifest"; ec=$?
    if [[ $ec -ne 0 ]]; then
      echo "[FAILURE] ${app_name} NP=2048 Graph=${manifest_name} ec=${ec}"
    else
      echo "[SUCCESS] ${app_name} NP=2048 Graph=${manifest_name}"
    fi
  fi
done
