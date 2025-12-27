#!/bin/bash
#SBATCH --nodes=27
#SBATCH --ntasks=2052
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_2052.out
#SBATCH -e kaspan_2052.err
#SBATCH -J kaspan_2052
#SBATCH --partition=cpuonly
#SBATCH --time=25:00
#SBATCH --export=ALL

set -euo pipefail

source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh


app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan


set +eu

np=2052
for local_n in 150000 300000 600000; do
  for d in 90 100 200 400; do
    n=$(( np * local_n ))
    m=$(( n * d / 100 ))
    [[ $m -gt 4000000000 ]] && continue
    kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
    output_file="${app_name}_rmat_a59_bc19_np${np}_n${local_n}_d${d}.json"
    run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$local_n" "$d" "rmat_a59_bc19" "--nodes=27 --ntasks=2052 --cpus-per-task=1 --hint=nomultithread"
  done
done