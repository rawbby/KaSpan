#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=76
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o single_node.out
#SBATCH -e single_node.err
#SBATCH -J ispan_single_node
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL

set -euo pipefail

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

app_name=ispan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_ispan

rwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )

export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=4

set +eu

for manifest in "${rwd[@]}"; do
  manifest_name="$(basename "${manifest%.manifest}")"
  output_file="${app_name}_${manifest_name}_np64.json"

  if [[ -s "$output_file" ]]; then
    echo "[SKIPPING] ${app_name} NP=64 Graph=${manifest_name}"

  else
    echo "[STARTING] ${app_name} NP=64 Graph=${manifest_name}"

    srun                     \
      --time=2:00            \
      --oom-kill-step=1      \
      --mpi=pmi2             \
      --nodes=1              \
      --exclusive            \
      --ntasks=64            \
      --ntasks-per-socket=38 \
      --cpus-per-task=1      \
      --hint=nomultithread   \
      --cpu-bind=cores       \
      "$app"                 \
        --output_file "${app_name}_${manifest_name}_np64.json" \
        --manifest_file "$manifest"
  fi
done

max_jobs=2 # one per socket
running_jobs=0

for manifest in "${rwd[@]}"; do
  for np in 32 16 8 4 2 1; do
    manifest_name="$(basename "${manifest%.manifest}")"
    output_file="${app_name}_${manifest_name}_np${np}.json"

    if [[ -s "$output_file" ]]; then
      echo "[SKIPPING] ${app_name} NP=${np} Graph=${manifest_name}"

    else
      echo "[STARTING] ${app_name} NP=${np} Graph=${manifest_name}"

      srun                          \
        --time=2:00                 \
        --oom-kill-step=1           \
        --mpi=pmi2                  \
        --nodes=1                   \
        --exclusive                 \
        --ntasks="${np}"            \
        --ntasks-per-socket="${np}" \
        --cpus-per-task=1           \
        --hint=nomultithread        \
        --cpu-bind=cores            \
        "$app"                      \
          --output_file "$output_file" \
          --manifest_file "$manifest" &

      ((running_jobs++))
      if (( running_jobs >= max_jobs )); then
        wait -n
        ((running_jobs--))
      fi
    fi
  done
done
wait
