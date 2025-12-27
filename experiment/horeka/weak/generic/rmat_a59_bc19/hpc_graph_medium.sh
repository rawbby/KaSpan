#!/bin/bash
#SBATCH --nodes=7
#SBATCH --ntasks=7
#SBATCH --cpus-per-task=76
#SBATCH --ntasks-per-socket=1
#SBATCH --ntasks-per-node=1
#SBATCH -o hpc_graph_medium.out
#SBATCH -e hpc_graph_medium.err
#SBATCH -J hpc_graph_medium
#SBATCH --partition=cpuonly
#SBATCH --time=35:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh
source ~/workspace/KaSpan/experiment/horeka/run_parallel.sh
app_name=hpc_graph
app=~/workspace/KaSpan/cmake-build-release/bin/bench_hpc_graph
set +eu
for local_n in 150000 300000 600000; do
for d in 90 100 200 400; do
n=$(( 152 * local_n ))
m=$(( n * d / 100 ))
kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
run_async run_generic "$app" "${app_name}_rmat_a59_bc19_np152_n${local_n}_d${d}.json" "$kagen_string" "$app_name" 152 "$local_n" "$d" "rmat_a59_bc19" "--nodes=2 --ntasks=2 --cpus-per-task=76" --threads 76
n=$(( 304 * local_n ))
m=$(( n * d / 100 ))
kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
run_async run_generic "$app" "${app_name}_rmat_a59_bc19_np304_n${local_n}_d${d}.json" "$kagen_string" "$app_name" 304 "$local_n" "$d" "rmat_a59_bc19" "--nodes=4 --ntasks=4 --cpus-per-task=76" --threads 76
wait_all
done
done
# np=532
for local_n in 150000 300000 600000; do
for d in 90 100 200 400; do
n=$(( 532 * local_n ))
m=$(( n * d / 100 ))
kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
run_generic "$app" "${app_name}_rmat_a59_bc19_np532_n${local_n}_d${d}.json" "$kagen_string" "$app_name" 532 "$local_n" "$d" "rmat_a59_bc19" "--nodes=7 --ntasks=7 --cpus-per-task=76" --threads 76
done
done
