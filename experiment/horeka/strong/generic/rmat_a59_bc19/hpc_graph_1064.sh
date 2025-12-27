#!/bin/bash
#SBATCH --nodes=14
#SBATCH --ntasks=14
#SBATCH --cpus-per-task=76
#SBATCH --ntasks-per-socket=1
#SBATCH --ntasks-per-node=1
#SBATCH -o hpc_graph_1064.out
#SBATCH -e hpc_graph_1064.err
#SBATCH -J hpc_graph_1064
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
kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
output_file="${app_name}_rmat_a59_bc19_np${np}_n${n}_d${d}.json"
run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$n" "$d" "rmat_a59_bc19" "--nodes=14 --ntasks=14 --cpus-per-task=76" --threads 76
done
done
