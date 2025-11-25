#!/bin/bash
#SBATCH --nodes=7
#SBATCH --ntasks=532
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o medium.out
#SBATCH -e medium.err
#SBATCH -J medium
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
export I_MPI_JOB_TIMEOUT=4

set +eu

for manifest in "${rwd[@]}"; do
  manifest_name="$(basename "${manifest%.manifest}")"

  srun                   \
    --mpi=pmi2           \
    --nodes=1            \
    --exclusive          \
    --ntasks=64          \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "${app_name}_${manifest_name}_np64.json" \
      --manifest_file "$manifest" &

  srun                   \
    --mpi=pmi2           \
    --nodes=2            \
    --exclusive          \
    --ntasks=128         \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "${app_name}_${manifest_name}_np128.json" \
      --manifest_file "$manifest" &

  srun                   \
    --mpi=pmi2           \
    --nodes=4            \
    --exclusive          \
    --ntasks=256         \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "${app_name}_${manifest_name}_np256.json" \
      --manifest_file "$manifest" &

  wait

  srun                   \
    --mpi=pmi2           \
    --nodes=7            \
    --exclusive          \
    --ntasks=512         \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "${app_name}_${manifest_name}_np512.json" \
      --manifest_file "$manifest"
done
