#!/bin/bash
#SBATCH --nodes=14
#SBATCH --ntasks=1064
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_1064.out
#SBATCH -e kaspan_1064.err
#SBATCH -J kaspan_1064
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

export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=3

set +eu

np=1064
for n in 150000 300000 600000; do
  for d in 90 100 200 400; do
    total_n=$(( np * n ))
    total_m=$(( total_n * d / 100 ))
    if [[ $total_m -gt 4000000000 ]]; then
      continue
    fi
    kagen_string="rmat;directed;n=${total_n};m=${total_m};a=0.59;b=0.19;c=0.19;seed=13"
    output_file="${app_name}_rmat_a59_bc19_np${np}_n${n}_d${d}.json"
    if [[ -s "$output_file" ]]; then
      echo "[SKIPPING] ${app_name} np=${np} n=${n} d=${d} graph=rmat_a59_bc19"
    else
      echo "[STARTING] ${app_name} np=${np} n=${n} d=${d} graph=rmat_a59_bc19"
      srun                   \
    --time=3:00          \
    --oom-kill-step=1    \
    --mpi=pmi2           \
    --nodes=14           \
    --exclusive          \
    --ntasks=1064        \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "$output_file" \
      --kagen_option_string "$kagen_string"; ec=$
      if [[ $ec -ne 0 ]]; then
        [[ $ec -eq 137 ]] && ec="${ec} (oom)"
        echo "[FAILURE] ${app_name} np=${np} n=${n} d=${d} graph=rmat_a59_bc19 ec=${ec}"
      else
        echo "[SUCCESS] ${app_name} np=${np} n=${n} d=${d} graph=rmat_a59_bc19"
      fi
    fi
  done
done
fi
