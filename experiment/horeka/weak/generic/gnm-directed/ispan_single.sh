#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=76
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_single.out
#SBATCH -e kaspan_single.err
#SBATCH -J kaspan_single
#SBATCH --partition=cpuonly
#SBATCH --time=45:00
#SBATCH --export=ALL

set -euo pipefail

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

app_name=ispan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_ispan

export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=3

set +eu

for np in 76 38 16 8 4 2 1; do
  kagen_string="gnm-directed;n=PLACEHOLDER_N;m=PLACEHOLDER_M;seed=13"
  kagen_string="${kagen_string/PLACEHOLDER_N/$((np * 150000))}"
  kagen_string="${kagen_string/PLACEHOLDER_M/$((np * 1500000))}"
  
  output_file="${app_name}_gnm-directed_np${np}.json"

  if [[ -s "$output_file" ]]; then
    echo "[SKIPPING] ${app_name} NP=${np} Graph=gnm-directed"
  else
    echo "[STARTING] ${app_name} NP=${np} Graph=gnm-directed"
    srun                             \
      --time=3:00                    \
      --oom-kill-step=1              \
      --mpi=pmi2                     \
      --nodes=1                      \
      --exclusive                    \
      "--ntasks=$np"                 \
      --cpus-per-task=1              \
      --hint=nomultithread           \
      --cpu-bind=cores               \
      "$app"                         \
        --output_file "$output_file" \
        --kagen_option_string "$kagen_string"; ec=$?
    if [[ $ec -ne 0 ]]; then
      [[ $ec -eq 137 ]] && ec="${ec} (oom)"
      echo "[FAILURE] ${app_name} NP=${np} Graph=gnm-directed ec=${ec}"
    else
      echo "[SUCCESS] ${app_name} NP=${np} Graph=gnm-directed"
    fi
  fi
done
