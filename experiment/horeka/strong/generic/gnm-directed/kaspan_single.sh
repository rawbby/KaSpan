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

app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan


export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=3

set +eu

sizes=( "small" "medium" "huge" )
  kagen_strings=( "gnm-directed;n=2000000;m=20000000;seed=13" "gnm-directed;n=20000000;m=200000000;seed=13" "gnm-directed;n=100000000;m=1000000000;seed=13" )
  for i in "${!kagen_strings[@]}"; do
    manifest_name="${sizes[$i]}"
    kagen_string="${kagen_strings[$i]}"
  for np in 76 38 16 8 4 2 1; do
    output_file="${app_name}_${manifest_name}_np${np}.json"

    if [[ -s "$output_file" ]]; then
      echo "[SKIPPING] ${app_name} NP=${np} Graph=${manifest_name}"
    else
      echo "[STARTING] ${app_name} NP=${np} Graph=${manifest_name}"
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
        echo "[FAILURE] ${app_name} NP=${np} Graph=${manifest_name} ec=${ec}"
      else
        echo "[SUCCESS] ${app_name} NP=${np} Graph=${manifest_name}"
      fi
    fi
  done
done
