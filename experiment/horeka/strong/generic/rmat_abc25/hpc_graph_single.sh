#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=76
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o hpc_graph_single.out
#SBATCH -e hpc_graph_single.err
#SBATCH -J hpc_graph_single
#SBATCH --partition=cpuonly
#SBATCH --time=45:00
#SBATCH --export=ALL

set -euo pipefail

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

app_name=hpc_graph
app=~/workspace/KaSpan/cmake-build-release/bin/bench_hpc_graph


export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=5

set +eu

sizes=( "small" "medium" "huge" )
  kagen_strings=( "rmat;directed;n=2000000;m=20000000;a=0.25;b=0.25;c=0.25;seed=13" "rmat;directed;n=20000000;m=200000000;a=0.25;b=0.25;c=0.25;seed=13" "rmat;directed;n=100000000;m=1000000000;a=0.25;b=0.25;c=0.25;seed=13" )
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
        --time=5:00                    \
        --oom-kill-step=1              \
        --mpi=pmi2                     \
        --nodes=1                      \
        --exclusive                    \
        "--ntasks=1"                   \
        "--ntasks-per-socket=1"        \
        "--cpus-per-task=$np"          \
        --cpu-bind=cores               \
        "$app"                         \
          --output_file "$output_file" \
          --threads "$np"              \
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
