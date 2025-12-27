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
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh
source ~/workspace/KaSpan/experiment/horeka/run_parallel.sh
app_name=ispan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_ispan
set +eu
for n in 1000000000 100000000 10000000; do
for d in 90 100 200 400; do
m=$(( n * d / 100 ))
kagen_string="gnm-directed;n=${n};m=${m};seed=13"
run_async run_generic "$app" "${app_name}_gnm-directed_np152_n${n}_d${d}.json" "$kagen_string" "$app_name" 152 "$n" "$d" "gnm-directed" "--nodes=2 --ntasks=152 --cpus-per-task=1 --hint=nomultithread"
run_async run_generic "$app" "${app_name}_gnm-directed_np304_n${n}_d${d}.json" "$kagen_string" "$app_name" 304 "$n" "$d" "gnm-directed" "--nodes=4 --ntasks=304 --cpus-per-task=1 --hint=nomultithread"
wait_all
done
done
# np=532
for n in 1000000000 100000000 10000000; do
for d in 90 100 200 400; do
m=$(( n * d / 100 ))
kagen_string="gnm-directed;n=${n};m=${m};seed=13"
run_generic "$app" "${app_name}_gnm-directed_np532_n${n}_d${d}.json" "$kagen_string" "$app_name" 532 "$n" "$d" "gnm-directed" "--nodes=7 --ntasks=532 --cpus-per-task=1 --hint=nomultithread"
done
done
