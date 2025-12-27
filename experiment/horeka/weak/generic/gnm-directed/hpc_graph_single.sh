#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=76
#SBATCH --ntasks-per-socket=1
#SBATCH --ntasks-per-node=1
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

for np in 76 38 16 8 4 2 1; do
  for n in 150000 300000 600000; do
    for d in 90 100 200 400; do
      total_n=$(( np * n ))
    total_m=$(( total_n * d / 100 ))
      if [[ $total_m -gt 4000000000 ]]; then
        continue
      fi
      kagen_string="gnm-directed;n=${total_n};m=${total_m};seed=13"
      output_file="${app_name}_gnm-directed_np${np}_n${n}_d${d}.json"
      if [[ -s "$output_file" ]]; then
        echo "[SKIPPING] ${app_name} np=${np} n=${n} d=${d} graph=gnm-directed"
      else
        echo "[STARTING] ${app_name} np=${np} n=${n} d=${d} graph=gnm-directed"
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
        --kagen_option_string "$kagen_string"; ec=$
        if [[ $ec -ne 0 ]]; then
          [[ $ec -eq 137 ]] && ec="${ec} (oom)"
          echo "[FAILURE] ${app_name} np=${np} n=${n} d=${d} graph=gnm-directed ec=${ec}"
        else
          echo "[SUCCESS] ${app_name} np=${np} n=${n} d=${d} graph=gnm-directed"
        fi
      fi
    done
  done
done
