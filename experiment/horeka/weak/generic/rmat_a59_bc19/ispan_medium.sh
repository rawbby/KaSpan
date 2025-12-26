#!/bin/bash
#SBATCH --nodes=7
#SBATCH --ntasks=532
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_medium.out
#SBATCH -e kaspan_medium.err
#SBATCH -J kaspan_medium
#SBATCH --partition=cpuonly
#SBATCH --time=35:00
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

# NP=152
np=152
kagen_string="rmat;directed;n=PLACEHOLDER_N;m=PLACEHOLDER_M;a=0.59;b=0.19;c=0.19;seed=13"
kagen_string="${kagen_string/PLACEHOLDER_N/$((np * 150000))}"
kagen_string="${kagen_string/PLACEHOLDER_M/$((np * 1500000))}"
output_file="${app_name}_rmat_a59_bc19_np152.json"
pid152=
if [[ -s "$output_file" ]]; then
  echo "[SKIPPING] ${app_name} NP=152 Graph=rmat_a59_bc19"
else
  echo "[STARTING] ${app_name} NP=152 Graph=rmat_a59_bc19"
  srun                   \
    --time=3:00          \
    --oom-kill-step=1    \
    --mpi=pmi2           \
    --nodes=2            \
    --exclusive          \
    --ntasks=152         \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "$output_file" \
      --kagen_option_string "$kagen_string" & pid152=$!
fi

# NP=304
np=304
kagen_string="rmat;directed;n=PLACEHOLDER_N;m=PLACEHOLDER_M;a=0.59;b=0.19;c=0.19;seed=13"
kagen_string="${kagen_string/PLACEHOLDER_N/$((np * 150000))}"
kagen_string="${kagen_string/PLACEHOLDER_M/$((np * 1500000))}"
output_file="${app_name}_rmat_a59_bc19_np304.json"
pid304=
if [[ -s "$output_file" ]]; then
  echo "[SKIPPING] ${app_name} NP=304 Graph=rmat_a59_bc19"
else
  echo "[STARTING] ${app_name} NP=304 Graph=rmat_a59_bc19"
  srun                   \
    --time=3:00          \
    --oom-kill-step=1    \
    --mpi=pmi2           \
    --nodes=4            \
    --exclusive          \
    --ntasks=304         \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "$output_file" \
      --kagen_option_string "$kagen_string" & pid304=$!
fi

if [[ $pid152 ]]; then
  wait "$pid152"; ec=$?
  if [[ $ec -ne 0 ]]; then
    [[ $ec -eq 137 ]] && ec="${ec} (oom)"
    echo "[FAILURE] ${app_name} NP=152 Graph=rmat_a59_bc19"
  else
    echo "[SUCCESS] ${app_name} NP=152 Graph=rmat_a59_bc19"
  fi
fi

if [[ $pid304 ]]; then
  wait "$pid304"; ec=$?
  if [[ $ec -ne 0 ]]; then
    [[ $ec -eq 137 ]] && ec="${ec} (oom)"
    echo "[FAILURE] ${app_name} NP=304 Graph=rmat_a59_bc19"
  else
    echo "[SUCCESS] ${app_name} NP=304 Graph=rmat_a59_bc19"
  fi
fi

# NP=532
np=532
kagen_string="rmat;directed;n=PLACEHOLDER_N;m=PLACEHOLDER_M;a=0.59;b=0.19;c=0.19;seed=13"
kagen_string="${kagen_string/PLACEHOLDER_N/$((np * 150000))}"
kagen_string="${kagen_string/PLACEHOLDER_M/$((np * 1500000))}"
output_file="${app_name}_rmat_a59_bc19_np532.json"
if [[ -s "$output_file" ]]; then
  echo "[SKIPPING] ${app_name} NP=532 Graph=rmat_a59_bc19"
else
  echo "[STARTING] ${app_name} NP=532 Graph=rmat_a59_bc19"
  srun                   \
    --time=3:00          \
    --oom-kill-step=1    \
    --mpi=pmi2           \
    --nodes=7            \
    --exclusive          \
    --ntasks=532         \
    --cpus-per-task=1    \
    --hint=nomultithread \
    --cpu-bind=cores     \
    "$app"               \
      --output_file "$output_file" \
      --kagen_option_string "$kagen_string"; ec=$?
  if [[ $ec -ne 0 ]]; then
    [[ $ec -eq 137 ]] && ec="${ec} (oom)"
    echo "[FAILURE] ${app_name} NP=532 Graph=rmat_a59_bc19 ec=${ec}"
  else
    echo "[SUCCESS] ${app_name} NP=532 Graph=rmat_a59_bc19"
  fi
fi
