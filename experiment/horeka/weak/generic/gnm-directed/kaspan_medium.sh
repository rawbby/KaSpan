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

app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan

export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=3

set +eu

# np=152
np=152
kagen_string="gnm-directed;n=PLACEHOLDER_N;m=PLACEHOLDER_M;seed=13"
kagen_string="${kagen_string/PLACEHOLDER_N/$((np * 150000))}"
kagen_string="${kagen_string/PLACEHOLDER_M/$((np * 1500000))}"
output_file="${app_name}_gnm-directed_np152.json"
pid152=
np=152
for n in 150000 300000 600000; do
  for d in 90 100 200 400; do
    total_n=$(( np * n ))
    total_m=$(( total_n * d / 100 ))
    if [[ $total_m -gt 4000000000 ]]; then
      continue
    fi
    kagen_string="gnm-directed;n=${total_n};m=${total_m};seed=13"
    output_file="${app_name}_gnm-directed_np152_n${n}_d${d}.json"
    if [[ -s "$output_file" ]]; then
      echo "[SKIPPING] ${app_name} np=152 n=${n} d=${d} graph=gnm-directed"
    else
      echo "[STARTING] ${app_name} np=152 n=${n} d=${d} graph=gnm-directed"
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
      --kagen_option_string "$kagen_string"
    fi
  done
done & pid152=$!

# np=304
np=304
kagen_string="gnm-directed;n=PLACEHOLDER_N;m=PLACEHOLDER_M;seed=13"
kagen_string="${kagen_string/PLACEHOLDER_N/$((np * 150000))}"
kagen_string="${kagen_string/PLACEHOLDER_M/$((np * 1500000))}"
output_file="${app_name}_gnm-directed_np304.json"
pid304=
np=304
for n in 150000 300000 600000; do
  for d in 90 100 200 400; do
    total_n=$(( np * n ))
    total_m=$(( total_n * d / 100 ))
    if [[ $total_m -gt 4000000000 ]]; then
      continue
    fi
    kagen_string="gnm-directed;n=${total_n};m=${total_m};seed=13"
    output_file="${app_name}_gnm-directed_np304_n${n}_d${d}.json"
    if [[ -s "$output_file" ]]; then
      echo "[SKIPPING] ${app_name} np=304 n=${n} d=${d} graph=gnm-directed"
    else
      echo "[STARTING] ${app_name} np=304 n=${n} d=${d} graph=gnm-directed"
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
      --kagen_option_string "$kagen_string"
    fi
  done
done & pid304=$!

if [[ $pid152 ]]; then
  wait "$pid152"; ec=$?
  if [[ $ec -ne 0 ]]; then
    [[ $ec -eq 137 ]] && ec="${ec} (oom)"
    echo "[FAILURE] ${app_name} np=152 graph=gnm-directed"
  else
    echo "[SUCCESS] ${app_name} np=152 graph=gnm-directed"
  fi
fi

if [[ $pid304 ]]; then
  wait "$pid304"; ec=$?
  if [[ $ec -ne 0 ]]; then
    [[ $ec -eq 137 ]] && ec="${ec} (oom)"
    echo "[FAILURE] ${app_name} np=304 graph=gnm-directed"
  else
    echo "[SUCCESS] ${app_name} np=304 graph=gnm-directed"
  fi
fi

# np=532
np=532
kagen_string="gnm-directed;n=PLACEHOLDER_N;m=PLACEHOLDER_M;seed=13"
kagen_string="${kagen_string/PLACEHOLDER_N/$((np * 150000))}"
kagen_string="${kagen_string/PLACEHOLDER_M/$((np * 1500000))}"
output_file="${app_name}_gnm-directed_np532.json"
if [[ -s "$output_file" ]]; then
  echo "[SKIPPING] ${app_name} np=532 graph=gnm-directed"
else
  echo "[STARTING] ${app_name} np=532 graph=gnm-directed"
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
    echo "[FAILURE] ${app_name} np=532 graph=gnm-directed ec=${ec}"
  else
    echo "[SUCCESS] ${app_name} np=532 graph=gnm-directed"
  fi
fi
