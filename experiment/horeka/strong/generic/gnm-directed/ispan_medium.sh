#!/bin/bash
#SBATCH --nodes=7
#SBATCH --ntasks=532
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o ispan_medium.out
#SBATCH -e ispan_medium.err
#SBATCH -J ispan_medium
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

sizes=( "small" "medium" "huge" )
  kagen_strings=( "gnm-directed;n=2000000;m=20000000;seed=13" "gnm-directed;n=20000000;m=200000000;seed=13" "gnm-directed;n=100000000;m=1000000000;seed=13" )
  for i in "${!kagen_strings[@]}"; do
    manifest_name="${sizes[$i]}"
    kagen_string="${kagen_strings[$i]}"

  output_file="${app_name}_${manifest_name}_np152.json"
  pid152=
  if [[ -s "$output_file" ]]; then
    echo "[SKIPPING] ${app_name} NP=152 Graph=${manifest_name}"
  else
    echo "[STARTING] ${app_name} NP=152 Graph=${manifest_name}"
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

  output_file="${app_name}_${manifest_name}_np304.json"
  pid304=
  if [[ -s "$output_file" ]]; then
    echo "[SKIPPING] ${app_name} NP=304 Graph=${manifest_name}"
  else
    echo "[STARTING] ${app_name} NP=304 Graph=${manifest_name}"
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
      echo "[FAILURE] ${app_name} NP=152 Graph=${manifest_name}"
    else
      echo "[SUCCESS] ${app_name} NP=152 Graph=${manifest_name}"
    fi
  fi

  if [[ $pid304 ]]; then
    wait "$pid304"; ec=$?
    if [[ $ec -ne 0 ]]; then
      [[ $ec -eq 137 ]] && ec="${ec} (oom)"
      echo "[FAILURE] ${app_name} NP=304 Graph=${manifest_name}"
    else
      echo "[SUCCESS] ${app_name} NP=304 Graph=${manifest_name}"
    fi
  fi

  output_file="${app_name}_${manifest_name}_np532.json"
  if [[ -s "$output_file" ]]; then
    echo "[SKIPPING] ${app_name} NP=532 Graph=${manifest_name}"
  else
    echo "[STARTING] ${app_name} NP=532 Graph=${manifest_name}"
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
      echo "[FAILURE] ${app_name} NP=532 Graph=${manifest_name} ec=${ec}"
    else
      echo "[SUCCESS] ${app_name} NP=532 Graph=${manifest_name}"
    fi
  fi
done
