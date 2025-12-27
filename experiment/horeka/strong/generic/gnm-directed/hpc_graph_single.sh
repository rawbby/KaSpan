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
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh
app_name=hpc_graph
app=~/workspace/KaSpan/cmake-build-release/bin/bench_hpc_graph
set +eu
for n in 1000000000 100000000 10000000; do
for d in 90 100 200 400; do
m=$(( n * d / 100 ))
kagen_string="gnm-directed;n=${n};m=${m};seed=13"
output_file="${app_name}_gnm-directed_np${np}_n${n}_d${d}.json"
run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$n" "$d" "gnm-directed" "--nodes=1 --ntasks=1 --ntasks-per-socket=1 --cpus-per-task=$np" --threads $np
done
done
