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
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh
source ~/workspace/KaSpan/experiment/horeka/run_parallel.sh
app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan
set +eu
for n in 1000000000 100000000 10000000; do
for d in 90 100 200 400; do
m=$(( n * d / 100 ))
kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
run_async run_generic "$app" "${app_name}_rmat_a59_bc19_np152_n${n}_d${d}.json" "$kagen_string" "$app_name" 152 "$n" "$d" "rmat_a59_bc19" "--nodes=2 --ntasks=152 --cpus-per-task=1 --hint=nomultithread"
run_async run_generic "$app" "${app_name}_rmat_a59_bc19_np304_n${n}_d${d}.json" "$kagen_string" "$app_name" 304 "$n" "$d" "rmat_a59_bc19" "--nodes=4 --ntasks=304 --cpus-per-task=1 --hint=nomultithread"
wait_all
done
done
# np=532
for n in 1000000000 100000000 10000000; do
for d in 90 100 200 400; do
m=$(( n * d / 100 ))
kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
run_generic "$app" "${app_name}_rmat_a59_bc19_np532_n${n}_d${d}.json" "$kagen_string" "$app_name" 532 "$n" "$d" "rmat_a59_bc19" "--nodes=7 --ntasks=532 --cpus-per-task=1 --hint=nomultithread"
done
done
