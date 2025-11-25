#!/bin/bash
#SBATCH --nodes=54
#SBATCH --ntasks=4104
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o 4096.out
#SBATCH -e 4096.err
#SBATCH -J 4096
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
    --mpi=pmix           \
    --nodes=54           \
    --exclusive          \
    --ntasks=4096        \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "${app_name}_${manifest_name}_np4096.json" \
      --manifest_file "$manifest"
done
